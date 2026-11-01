#!/usr/bin/env python3
"""Fetch encyclopedic illustrations via Wikipedia and Commons with license metadata.

Requires Pillow; SVG conversion additionally requires CairoSVG. No unlicensed or
fair-use images are accepted. Download once; runtime uses only local RGBA assets.
"""
import argparse
import hashlib
import html
import io
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def request(url):
    req = urllib.request.Request(url, headers={'User-Agent': 'SolarisHistory/0.2 (educational game asset importer)'})
    with urllib.request.urlopen(req, timeout=30) as response:
        data = response.read(16 * 1024 * 1024 + 1)
        if len(data) > 16 * 1024 * 1024: raise ValueError('image exceeds 16 MiB')
        return data

def query(host, **params):
    return json.loads(request('https://' + host + '/w/api.php?' + urllib.parse.urlencode(dict(action='query',format='json',**params))))

def plain(text): return html.unescape(re.sub('<[^>]+>', '', text))

def fetch(item, dest):
    from PIL import Image
    if item.get('file'):
        title = 'File:' + item['file']
    else:
        pages = query('en.wikipedia.org',titles=item['article'],redirects=1,prop='pageimages',piprop='name',pilicense='free')['query']['pages']
        page = next(iter(pages.values()))
        if 'pageimage' not in page: raise ValueError('no free illustration for ' + item['article'])
        title = 'File:' + page['pageimage']
    pages = query('commons.wikimedia.org',titles=title,prop='imageinfo',iiprop='url|extmetadata')['query']['pages']
    info = next(iter(pages.values())).get('imageinfo', [None])[0]
    if not info: raise ValueError('file is not hosted on Wikimedia Commons')
    meta = info.get('extmetadata', {})
    def val(k): return plain(meta.get(k,{}).get('value',''))
    license_name = val('LicenseShortName')
    if not (license_name.startswith(('CC BY', 'CC0', 'Public domain', 'PD'))):
        raise ValueError('license requires manual review: ' + license_name)
    url = info['url']
    parsed=urllib.parse.urlparse(url)
    if parsed.scheme != 'https' or parsed.hostname != 'upload.wikimedia.org': raise ValueError('unexpected image host')
    raw = request(url)
    if parsed.path.lower().endswith('.svg'):
        import cairosvg
        # Reject external references before rasterization; unsafe SVG features stay disabled.
        if re.search(rb'(?:href\s*=\s*[\'"](?:https?:|file:|//)|<!ENTITY|<!DOCTYPE)',raw,re.I):
            raise ValueError('SVG external references are not supported')
        from defusedxml import ElementTree
        tree = ElementTree.fromstring(raw)
        for element in tree.iter():
            for key, value in element.attrib.items():
                if key.split('}')[-1] == 'href' and not value.startswith('#'):
                    raise ValueError('SVG external references are not supported')
                if ('url(' in value.lower() and not re.fullmatch(r'url\(#[^)]+\)', value.strip())):
                    raise ValueError('SVG external URLs are not supported')
            if element.tag.split('}')[-1] == 'style' and element.text and ('url(' in element.text.lower() or '@import' in element.text.lower()):
                raise ValueError('SVG external styles are not supported')
        raw = cairosvg.svg2png(bytestring=raw,output_width=128,output_height=128)
    image = Image.open(io.BytesIO(raw)).convert('RGBA')
    image.thumbnail((128,128), Image.Resampling.LANCZOS)
    canvas = Image.new('RGBA',(128,128),(0,0,0,0))
    canvas.alpha_composite(image,((128-image.width)//2,(128-image.height)//2))
    (dest/(item['id']+'.rgba')).write_bytes(canvas.tobytes())
    return dict(id=item['id'], article=item.get('article'),file=title,source=info['descriptionurl'],download=url,
                author=val('Artist'),license=license_name,license_url=val('LicenseUrl'),
                sha256=hashlib.sha256(canvas.tobytes()).hexdigest(),
                note=item.get('note','Encyclopedic illustration; not necessarily a contemporary flag.'))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only',help='fetch one catalogue ID')
    args=parser.parse_args()
    dest=ROOT/'content/media';dest.mkdir(exist_ok=True)
    items=json.loads((ROOT/'content/media_catalog.json').read_text())
    if args.only:
        items=[i for i in items if i['id']==args.only]
        if not items: parser.error('unknown catalogue ID')
    old=json.loads((dest/'attributions.json').read_text()) if (dest/'attributions.json').exists() else []
    records={r['id']:r for r in old};failed=[]
    for item in items:
        try:
            record=fetch(item,dest);records[item['id']]=record;print('OK',item['id'])
        except Exception as exc:
            failed.append(item['id']);print('FAILED',item['id'],str(exc),file=sys.stderr)
    (dest/'attributions.json').write_text(json.dumps(list(records.values()),ensure_ascii=False,indent=2)+'\n')
    (dest/'index.json').write_text(json.dumps([dict(id=r) for r in records],indent=2)+'\n')
    return bool(failed)

if __name__=='__main__': sys.exit(main())

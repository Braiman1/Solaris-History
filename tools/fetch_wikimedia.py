#!/usr/bin/env python3
"""Fetch encyclopedic illustrations via Wikipedia and Commons with license metadata.

Requires Pillow; SVG conversion additionally requires CairoSVG. No unlicensed or
fair-use images are accepted. Download once; runtime uses only local RGBA assets.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import html
import io
import json
import re
import sys
import urllib.parse
import urllib.request
import urllib.error
import time
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

_rate_lock = threading.Lock()
_next_request = {}
_query_cache = {}

def request(url):
    host=urllib.parse.urlparse(url).hostname
    req = urllib.request.Request(url, headers={'User-Agent': 'SolarisHistory/0.2 (educational game asset importer; https://github.com/Braiman1/Solaris-History)'})
    for attempt in range(6):
        with _rate_lock:
            now=time.monotonic()
            when=max(now,_next_request.get(host,now))
            _next_request[host]=when+1.25
        if when>now: time.sleep(when-now)
        try:
            with urllib.request.urlopen(req, timeout=45) as response:
                data=response.read(64*1024*1024+1)
                if len(data)>64*1024*1024: raise ValueError('image exceeds 64 MiB')
                return data
        except urllib.error.HTTPError as exc:
            if exc.code not in (429,503) or attempt==5: raise
            retry=exc.headers.get('Retry-After','')
            delay=min(60,float(retry)) if retry.isdigit() else min(30,5*2**attempt)
            print('Waiting',int(delay),'seconds after HTTP',exc.code,'from',host,flush=True)
            with _rate_lock:
                _next_request[host]=max(_next_request.get(host,0),time.monotonic()+delay)
    raise RuntimeError('request retries exhausted')

def query(host, **params):
    key=(host,tuple(sorted(params.items())))
    if key not in _query_cache:
        _query_cache[key]=json.loads(request('https://' + host + '/w/api.php?' + urllib.parse.urlencode(dict(action='query',format='json',**params))))
    return _query_cache[key]

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
    author=val('Artist') or item.get('author_override','')
    if not author and license_name.startswith('CC BY'): raise ValueError('missing author attribution; review the Commons source')
    author=author or 'Author not specified in the Commons metadata; see source.'
    url = info['url']
    parsed=urllib.parse.urlparse(url)
    if parsed.scheme != 'https' or parsed.hostname != 'upload.wikimedia.org': raise ValueError('unexpected image host')
    raw = request(url)
    if parsed.path.lower().endswith('.svg'):
        import cairosvg
        # Reject external references before rasterization; unsafe SVG features stay disabled.
        if re.search(rb'(?:href\s*=\s*[\'"](?:https?:|file:|//)|<!ENTITY)',raw,re.I):
            raise ValueError('SVG external references are not supported')
        from defusedxml import ElementTree
        tree = ElementTree.fromstring(raw)
        for element in tree.iter():
            for key, value in element.attrib.items():
                if key.split('}')[-1] == 'href' and not value.startswith('#'):
                    raise ValueError('SVG external references are not supported')
                for reference in re.findall(r'url\(([^)]+)\)',value,re.I):
                    if not reference.strip(" \"'").startswith('#'): raise ValueError('SVG external URLs are not supported')
            if element.tag.split('}')[-1] == 'style' and element.text:
                if '@import' in element.text.lower(): raise ValueError('SVG external styles are not supported')
                for reference in re.findall(r'url\(([^)]+)\)',element.text,re.I):
                    if not reference.strip(" \"'").startswith('#'): raise ValueError('SVG external styles are not supported')
        raw = cairosvg.svg2png(bytestring=raw,output_width=128,output_height=128)
    image = Image.open(io.BytesIO(raw)).convert('RGBA')
    surface={}
    if item.get('spherical'):
        pixels=image.resize((512,256),Image.Resampling.LANCZOS).tobytes()
        (dest/(item['id']+'_surface.rgba')).write_bytes(pixels)
        surface=dict(surface_sha256=hashlib.sha256(pixels).hexdigest(),surface_width=512,surface_height=256)
    image.thumbnail((128,128), Image.Resampling.LANCZOS)
    canvas = Image.new('RGBA',(128,128),(0,0,0,0))
    canvas.alpha_composite(image,((128-image.width)//2,(128-image.height)//2))
    (dest/(item['id']+'.rgba')).write_bytes(canvas.tobytes())
    return dict(id=item['id'], article=item.get('article'),file=title,source=info['descriptionurl'],download=url,
                author=author,license=license_name,license_url=val('LicenseUrl'),
                sha256=hashlib.sha256(canvas.tobytes()).hexdigest(),
                note=item.get('note','Encyclopedic illustration; not necessarily a contemporary flag.'),
                note_es=item.get('note_es','Ilustracion enciclopedica, no necesariamente una bandera historica.'),
                kind=item.get('kind','illustration'),original=info['url'],
                author_source=item.get('author_source',info['descriptionurl']),**surface)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--only',help='fetch one catalogue ID')
    parser.add_argument('--refresh',action='store_true',help='redownload cached assets')
    parser.add_argument('--workers',type=int,default=3,help='concurrent Wikimedia requests (1-4)')
    args=parser.parse_args()
    dest=ROOT/'content/media';dest.mkdir(exist_ok=True)
    items=json.loads((ROOT/'content/media_catalog.json').read_text())
    if args.only:
        items=[i for i in items if i['id']==args.only]
        if not items: parser.error('unknown catalogue ID')
    old=json.loads((dest/'attributions.json').read_text()) if (dest/'attributions.json').exists() else []
    records={r['id']:r for r in old};failed=[]
    def save_records():
        ordered=[records[key] for key in sorted(records)]
        for name,data in [('attributions',ordered),('index',[dict(id=r['id']) for r in ordered])]:
            tmp=dest/(name+'.json.tmp')
            tmp.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
            tmp.replace(dest/(name+'.json'))
    def cached(item):
        record=records.get(item['id'])
        path=dest/(item['id']+'.rgba')
        return (not args.refresh and record and path.exists()
                and hashlib.sha256(path.read_bytes()).hexdigest()==record.get('sha256')
                and (not item.get('file') or record.get('file')=='File:'+item['file'])
                and record.get('article')==item.get('article')
                and (not item.get('spherical') or ((dest/(item['id']+'_surface.rgba')).exists() and hashlib.sha256((dest/(item['id']+'_surface.rgba')).read_bytes()).hexdigest()==record.get('surface_sha256'))))
    pending=[]
    for item in items:
        if cached(item):
            if item.get('author_override') and not records[item['id']].get('author'): records[item['id']]['author']=item['author_override']
            for key in ('note','note_es','kind','author_source'):
                if key in item: records[item['id']][key]=item[key]
            print('CACHED',item['id'],flush=True)
        else: pending.append(item)
    with ThreadPoolExecutor(max_workers=max(1,min(4,args.workers))) as pool:
        futures={pool.submit(fetch,item,dest):item for item in pending}
        for future in as_completed(futures):
            item=futures[future]
            try:
                record=future.result();records[item['id']]=record
                save_records()
                print('OK',item['id'],flush=True)
            except Exception as exc:
                failed.append(item['id']);print('FAILED',item['id'],str(exc),file=sys.stderr,flush=True)
    save_records()
    return bool(failed)

if __name__=='__main__': sys.exit(main())

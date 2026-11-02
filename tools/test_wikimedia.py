"""Offline contract checks for the importer; real network access is verified separately."""
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from PIL import Image
import fetch_wikimedia as media

class ImporterTest(unittest.TestCase):
    def test_free_image_produces_rgba_and_attribution(self):
        data=io.BytesIO();Image.new('RGBA',(20,10),(20,40,80,255)).save(data,format='PNG')
        info={'query':{'pages':{'1':{'imageinfo':[{'url':'https://upload.wikimedia.org/example.png','descriptionurl':'https://commons.wikimedia.org/wiki/File:Example.png','extmetadata':{'LicenseShortName':{'value':'CC BY-SA 4.0'},'Artist':{'value':'Test author'},'LicenseUrl':{'value':'https://creativecommons.org/licenses/by-sa/4.0/'}}}]}}}}
        with tempfile.TemporaryDirectory() as out, patch.object(media,'query',return_value=info), patch.object(media,'request',return_value=data.getvalue()):
            result=media.fetch({'id':'test','file':'Example.png'},Path(out))
            self.assertEqual(len((Path(out)/'test.rgba').read_bytes()),128*128*4)
            self.assertEqual(result['author'],'Test author')
            self.assertEqual(result['license'],'CC BY-SA 4.0')
            self.assertEqual(len(result['sha256']),64)
    def test_rejects_nonfree_art(self):
        info={'query':{'pages':{'1':{'imageinfo':[{'extmetadata':{'LicenseShortName':{'value':'Fair use'}}}]}}}}
        with tempfile.TemporaryDirectory() as out, patch.object(media,'query',return_value=info):
            with self.assertRaisesRegex(ValueError,'license requires manual review'):media.fetch({'id':'test','file':'Example.png'},Path(out))
            self.assertEqual(list(Path(out).iterdir()),[])
    def test_svg_internal_gradient_and_external_reference(self):
        info={'query':{'pages':{'1':{'imageinfo':[{'url':'https://upload.wikimedia.org/example.svg','descriptionurl':'https://commons.wikimedia.org/wiki/File:Example.svg','extmetadata':{'LicenseShortName':{'value':'CC0'}}}]}}}}
        svg=b'<svg xmlns="http://www.w3.org/2000/svg" width="40" height="40"><defs><linearGradient id="g"><stop stop-color="red"/><stop offset="1" stop-color="blue"/></linearGradient></defs><rect width="40" height="40" style="fill:url(#g);stroke:none"/></svg>'
        with tempfile.TemporaryDirectory() as out, patch.object(media,'query',return_value=info), patch.object(media,'request',return_value=svg):
            media.fetch({'id':'safe','file':'Example.svg'},Path(out))
            self.assertEqual(len((Path(out)/'safe.rgba').read_bytes()),128*128*4)
        external=b'<svg xmlns="http://www.w3.org/2000/svg"><image href="/tmp/private.png"/></svg>'
        with tempfile.TemporaryDirectory() as out, patch.object(media,'query',return_value=info), patch.object(media,'request',return_value=external):
            with self.assertRaisesRegex(ValueError,'external references'):media.fetch({'id':'bad','file':'Example.svg'},Path(out))
            self.assertEqual(list(Path(out).iterdir()),[])

    def test_bundled_assets_are_complete_and_attributed(self):
        import hashlib
        root=media.ROOT/'content'
        catalogue=json.loads((root/'media_catalog.json').read_text())
        credits={item['id']:item for item in json.loads((root/'media/attributions.json').read_text())}
        index={item['id'] for item in json.loads((root/'media/index.json').read_text())}
        self.assertEqual(index,{item['id'] for item in catalogue})
        self.assertEqual(set(credits),index)
        for item in catalogue:
            with self.subTest(asset=item['id']):
                raw=(root/'media'/(item['id']+'.rgba')).read_bytes()
                self.assertEqual(len(raw),128*128*4)
                self.assertEqual(hashlib.sha256(raw).hexdigest(),credits[item['id']]['sha256'])
                self.assertTrue(credits[item['id']]['source'].startswith('https://commons.wikimedia.org/'))
                self.assertTrue(credits[item['id']]['license'])
                self.assertTrue(credits[item['id']]['author'])
                if 'surface_sha256' in credits[item['id']]:
                    surface=(root/'media'/(item['id']+'_surface.rgba')).read_bytes()
                    self.assertEqual(len(surface),512*256*4)
                    self.assertEqual(hashlib.sha256(surface).hexdigest(),credits[item['id']]['surface_sha256'])
                if item.get('file'):self.assertEqual(credits[item['id']]['file'],'File:'+item['file'])
                if item['id'].startswith(('culture_','nation_')):
                    self.assertIn('note_es',credits[item['id']])
                    self.assertNotIn('provisional',credits[item['id']]['note_es'])

    def test_catalogue_covers_content(self):
        root=media.ROOT/'content';catalogue=json.loads((root/'media_catalog.json').read_text());ids=[i['id'] for i in catalogue]
        self.assertEqual(len(ids),len(set(ids)))
        w=json.loads((root/'world.json').read_text())
        for resource in w['resources']: self.assertIn('resource_'+resource,ids)
        for tech in json.loads((root/'technologies.json').read_text()):self.assertIn('tech_'+tech['id'],ids)
        for nation in json.loads((root/'nations.json').read_text()):self.assertIn('nation_'+nation['id'],ids)

if __name__=='__main__':unittest.main()

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
    def test_catalogue_covers_content(self):
        root=media.ROOT/'content';catalogue=json.loads((root/'media_catalog.json').read_text());ids=[i['id'] for i in catalogue]
        self.assertEqual(len(ids),len(set(ids)))
        w=json.loads((root/'world.json').read_text())
        for resource in w['resources']: self.assertIn('resource_'+resource,ids)
        for tech in json.loads((root/'technologies.json').read_text()):self.assertIn('tech_'+tech['id'],ids)
        for nation in json.loads((root/'nations.json').read_text()):self.assertIn('nation_'+nation['id'],ids)

if __name__=='__main__':unittest.main()

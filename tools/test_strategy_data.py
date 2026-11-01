import json
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent/'content'
def read(path):return json.loads((ROOT/path).read_text())
class StrategyDataTest(unittest.TestCase):
    def test_country_references(self):
        tags=read('countries/index.json');self.assertEqual(len(tags),len(set(tags)))
        cultures={c['tag'] for c in read('cultures.json')};religions={r['tag'] for r in read('religions.json')}
        for tag in tags:
            country=read('countries/'+tag+'.json');self.assertEqual(tag,country['tag']);self.assertIn(country['culture_tag'],cultures);self.assertIn(country['religion_tag'],religions)
        self.assertEqual(read('countries/rom.json')['id'],'roma')
        for religion in read('religions.json'):
            values=[religion[k] for k in ['farm','military','stability']]
            self.assertTrue(any(v>0 for v in values));self.assertTrue(any(v<0 for v in values))
    def test_tree_dependencies_and_dates(self):
        techs={t['id']:t for t in read('technologies.json')}
        self.assertEqual({t['branch'] for t in techs.values()},{'militar','economica','social'})
        self.assertEqual(max(t['year'] for t in techs.values()),0)
        for key in techs:
            seen=set();current=key
            while current:
                self.assertNotIn(current,seen);seen.add(current);self.assertIn(current,techs);current=techs[current]['requires']
    def test_evolution_references(self):
        units={u['id']:u for u in read('units.json')};techs={t['id'] for t in read('technologies.json')}
        for key in units:
            seen=set();current=key
            while current:
                self.assertNotIn(current,seen);seen.add(current);self.assertIn(current,units)
                if units[current].get('requires_tech'):self.assertIn(units[current]['requires_tech'],techs)
                current=units[current].get('upgrade','')
        for culture,names in read('strategy.json')['unit_names'].items():self.assertEqual(set(names),set(units),culture)
if __name__=='__main__':unittest.main()

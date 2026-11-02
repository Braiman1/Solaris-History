"""Geographic and biome contracts; no network or data regeneration required."""
import json
import math
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent/'content'
def read(path):return json.loads((ROOT/path).read_text())
def km(a,b):return 6371*math.hypot(math.radians(b['lon']-a['lon'])*math.cos(math.radians((a['lat']+b['lat'])/2)),math.radians(b['lat']-a['lat']))
class ExplorationDataTest(unittest.TestCase):
    def test_biomes_and_continents(self):
        biomes={b['id']:b for b in read('biomes.json')}
        self.assertEqual(len(biomes),9)
        for biome in biomes.values():
            for factor in ('food','defense','move'):self.assertGreater(biome[factor],0)
        provinces=read('world.json')['provinces']
        self.assertEqual({p['biome'] for p in provinces},set(biomes))
        for p in provinces:self.assertIn(p['continent'],{'Europa','Africa','Asia'})
    def test_routes_preserve_colonization_limits(self):
        provinces=read('world.json')['provinces']
        self.assertEqual([p['id'] for p in provinces],list(range(len(provinces))))
        for p in provinces:
            for key,maximum in [('adj',225),('sea_adj',150),('explore_sea_adj',450)]:
                self.assertEqual(len(p[key]),len(set(p[key])))
                for target in p[key]:
                    self.assertNotEqual(p['id'],target);other=provinces[target]
                    self.assertIn(p['id'],other[key]);self.assertLessEqual(km(p,other),maximum+.01)
                    if key!='adj':self.assertTrue(p['coastal'] and other['coastal'])
    def test_tribes_have_room_to_form(self):
        world=read('world.json');provinces=world['provinces']
        cultures={c['id'] for c in read('cultures.json')}
        capitals=set()
        for tribe in world['city_states']:
            self.assertNotIn(tribe['capital'],capitals);capitals.add(tribe['capital'])
            self.assertIn(tribe['culture'],cultures)
            start=provinces[tribe['capital']];seen={start['id']};pending=[start['id']]
            while pending:
                for target in provinces[pending.pop()]['adj']:
                    if target not in seen and km(start,provinces[target])<=900:
                        seen.add(target);pending.append(target)
            minimum=4 if start['region'] in {'Egipto','Mesopotamia','Levante'} else 5
            self.assertGreaterEqual(len(seen),minimum,tribe['name'])
    def test_regions_and_culture_references(self):
        world=read('world.json');provinces=world['provinces']
        self.assertGreaterEqual(sum(p['region']=='Iberia' for p in provinces),40)
        self.assertGreaterEqual(sum(p['region'] in {'Germania','Escandinavia','Baltico'} for p in provinces),60)
        for culture in ('dilmun','namazga','kelteminar','botai','afanasievo','narva'):
            self.assertTrue(any(t['culture']==culture for t in world['city_states']),culture)
        names={p['name'] for p in provinces}
        for tag in read('countries/index.json'):
            country=read('countries/'+tag+'.json')
            if country.get('required_province'):self.assertIn(country['required_province'],names)
if __name__=='__main__':unittest.main()

#!/usr/bin/env python3
"""Idempotent expansion; preserves existing province IDs and reciprocal routes."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent

def apply():
    content = ROOT / 'content'
    def load(name): return json.loads((content / name).read_text())
    def save(name, value): (content / name).write_text(json.dumps(value, ensure_ascii=False, indent=1) + '\n')
    w = load('world.json')
    extra = '''Delta Oriental|32.0|30.9|egipcio|Egipto|Heliopolis|papiro
Naqada|32.72|25.90|egipcio|Egipto|Tebas|ceramica
El Badari|31.40|26.99|egipcio|Egipto|Abidos|lino
Merimde|30.85|30.32|egipcio|Egipto|Sais|trigo
Tell Brak|41.06|36.67|semita|Mesopotamia|Ninive|lana
Tell Hamoukar|41.95|36.81|semita|Mesopotamia|Ninive|ceramica
Nippur|45.23|32.13|sumerio|Mesopotamia|Uruk|arcilla
Shuruppak|45.50|31.78|sumerio|Mesopotamia|Uruk|cebada
Adab|45.62|31.93|sumerio|Mesopotamia|Uruk|arcilla
Tepe Sialk|51.37|33.97|iranio|Iran|Susa|cobre
Tepe Hissar|54.08|35.70|iranio|Iran|Susa|lapislazuli
Arslantepe|38.36|38.38|anatolio|Anatolia|Hattusa|cobre
Hacilar|30.10|37.57|anatolio|Anatolia|Catalhoyuk|ceramica
Beycesultan|29.70|38.26|anatolio|Anatolia|Troya|lana
Poliochni|25.34|39.85|egeo|Egeo|Troya|pescado
Dimini|22.75|39.36|egeo|Egeo|Sesklo|cebada
Lerna|22.75|37.55|egeo|Egeo|Micenas|aceite
Aricia|12.67|41.72|italico|Italia|Lacio|madera
Lavinium|12.48|41.65|italico|Italia|Lacio|sal
Tusculum|12.71|41.80|italico|Italia|Lacio|piedra
Alba Longa|12.65|41.75|italico|Italia|Lacio|trigo
Cerveteri|12.10|42.00|italico|Italia|Etruria|cobre
Skara Brae|-3.34|59.05|megalitico|Britania|Caledonia|pescado
Newgrange|-6.48|53.69|megalitico|Irlanda|Irlanda|piedra
Colinas del Tiber|12.50|41.90|italico|Italia|Lavinium|arcilla'''
    # Use named parent where present; nearest same-culture settlement otherwise.
    for line in extra.splitlines():
        name, lon, lat, culture, region, parent, resource = line.split('|')
        if any(p['name'] == name for p in w['provinces']): continue
        candidates = [p for p in w['provinces'] if p['culture'] == culture]
        neighbor = next((p for p in candidates if p['name'] == parent), None)
        if neighbor is None: neighbor = min(candidates, key=lambda p: (p['lon']-float(lon))**2+(p['lat']-float(lat))**2)
        pid = len(w['provinces'])
        p = dict(id=pid, name=name, lon=float(lon), lat=float(lat), culture=culture, region=region,
                 terrain='llanura', resource=resource, pop=3.0, dev=1, coastal=False, owner=0,
                 adj=[neighbor['id']], sea_adj=[], buildings=[], loyalty=60.0, occupied=False)
        neighbor['adj'].append(pid)
        w['provinces'].append(p)
    # Fix inconsistent tin ID used by the original generator/loader.
    if 'estaño' in w['resources']: w['resources']['estano'] = w['resources'].pop('estaño')
    for p in w['provinces']:
        if p['resource'] == 'estaño': p['resource'] = 'estano'
    resources = {
        'arcilla': ('Arcilla','stone',1.1,[170,110,80]),
        'ceramica': ('Ceramica','trade',1.5,[186,112,70]),
        'lana': ('Lana','trade',1.2,[220,212,190]),
        'papiro': ('Papiro','trade',1.7,[180,190,105]),
        'lapislazuli': ('Lapislazuli','trade',2.8,[65,90,180]),
    }
    for key,(name,kind,value,color) in resources.items(): w['resources'][key] = dict(name=name,kind=kind,value=value,color=color)
    # The starting factions are cultural groups, not states from later centuries.
    labels = {'egipcio':'Los Egipcios', 'sumerio':'Los Sumerios','anatolio':'Los Anatolios','semita':'Los Semitas','egeo':'Los Egeos'}
    for i, cs in enumerate(w['city_states']):
        if i < 6: cs['name'] = labels[cs['culture']]
    if not any(cs['culture'] == 'italico' for cs in w['city_states']):
        cap = next(p for p in w['provinces'] if p['name'] == 'Lavinium')
        w['city_states'].append(dict(id=len(w['city_states'])+1,name='Los Latinos',prov=cap['name'],capital=cap['id'],culture='italico',gov='monarquia',color=[150,65,65],playable=True,is_city_state=True,desc='Comunidades prehistoricas del Lacio. El nombre Latinos es una etiqueta de juego; Roma aun no existe.'))
    nations = load('nations.json')
    nations = [n for n in nations if n['id'] != 'latinos']
    for key,name,capital,year in [('alba_longa','Alba Longa','Alba Longa',-1200),('lavinium','Lavinium','Lavinium',-1200),('tusculum','Tusculum','Tusculum',-1000),('aricia','Aricia','Aricia',-1000),('roma','Roma','Colinas del Tiber',-753)]:
        if any(n['id'] == key for n in nations): continue
        nations.append(dict(id=key,name=name,culture='italico',capital=capital,color=[155,65,65],gov='monarquia',adj='Latino',era=0,earliest_year=year,desc='Estado latino historico independiente. Disponible a partir de '+str(-year)+' a.C.; fecha aproximada o tradicional.'))
    for n in nations:
        if n['id'] == 'etruria': n['earliest_year'] = -900
    save('world.json',w); save('nations.json',nations)
    print(f"Expansion: {len(w['provinces'])} provincias, {len(nations)} naciones, {len(w['resources'])} recursos")

if __name__ == '__main__': apply()

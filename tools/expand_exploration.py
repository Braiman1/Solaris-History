"""Idempotent authored expansion. Run explicitly; never during environment setup.
Existing province IDs and tribal starts are preserved. Ancient/modern place names
identify locations; cultural groupings and formation dates are game abstractions.
"""
from pathlib import Path
import json, math, shutil
ROOT=Path(__file__).resolve().parent.parent/'content'
def read(path): return json.loads((ROOT/path).read_text())
def write(path, data): (ROOT/path).write_text(json.dumps(data,ensure_ascii=False,indent=1)+'\n')
def expand():
 w=read('world.json')
 # name|longitude|latitude; regional blocks make authoring and review straightforward.
 groups={
 ('Iberia','iberico'): '''Braga|-8.42|41.55
Porto|-8.61|41.15
Coimbra|-8.43|40.21
Evora|-7.91|38.57
Faro|-7.93|37.02
Huelva|-6.94|37.26
Cadiz|-6.29|36.53
Malaga|-4.42|36.72
Granada|-3.60|37.18
Jaen|-3.79|37.77
Cordoba|-4.78|37.88
Carmona|-5.64|37.47
Badajoz|-6.97|38.88
Caceres|-6.37|39.47
Toledo|-4.03|39.86
Talavera|-4.83|39.96
Numancia|-2.44|41.81
Segovia|-4.12|40.95
Salamanca|-5.66|40.97
Zamora|-5.75|41.50
Leon|-5.57|42.60
Lugo|-7.56|43.01
Castro de Barona|-9.03|42.69
Oviedo|-5.85|43.36
Santander|-3.81|43.46
Bilbao|-2.93|43.26
Pamplona|-1.64|42.82
Zaragoza|-0.88|41.65
Lleida|0.62|41.62
Tarragona|1.25|41.12
Girona|2.82|41.98
Tortosa|0.52|40.81
Valencia|-0.38|39.47
Sagunto|-0.28|39.68
Alicante|-0.48|38.35
Cartagena|-0.98|37.60
Albacete|-1.86|38.99
Teruel|-1.11|40.34
Cuenca|-2.13|40.07
Ciudad Real|-3.93|38.98
Palma|2.65|39.57
Menorca|4.26|39.89
Ibiza|1.43|38.91''',
 ('Germania','nordico'): '''Hamburgo|9.99|53.55
Bremen|8.80|53.08
Luneburgo|10.41|53.25
Hannover|9.73|52.37
Osnabruck|8.05|52.28
Colonia|6.96|50.94
Dortmund|7.47|51.51
Magdeburgo|11.63|52.12
Berlin|13.40|52.52
Rostock|12.14|54.09
Schwerin|11.41|53.63
Frankfurt|8.68|50.11
Erfurt|11.03|50.98
Leipzig|12.37|51.34
Dresde|13.74|51.05
Breslavia|17.04|51.11
Poznan|16.93|52.41
Gdansk|18.65|54.35
Torun|18.60|53.01
Varsovia|21.01|52.23
Cracovia|19.94|50.06
Groningen|6.57|53.22
Utrecht|5.12|52.09
Kiel|10.12|54.32''',
 ('Escandinavia','nordico'): '''Ribe|8.76|55.33
Jelling|9.42|55.75
Aarhus|10.20|56.16
Aalborg|9.92|57.05
Roskilde|12.08|55.64
Lund|13.19|55.70
Halmstad|12.86|56.67
Gotemburgo|11.97|57.71
Skara|13.44|58.39
Norrkoping|16.18|58.59
Uppsala|17.64|59.86
Orebro|15.21|59.27
Vasteras|16.54|59.61
Visby|18.29|57.63
Oslo|10.75|59.91
Sarpsborg|11.11|59.28
Kristiansand|8.00|58.15
Stavanger|5.73|58.97
Bergen|5.32|60.39
Hamar|11.07|60.79
Lillehammer|10.47|61.12
Trondheim|10.40|63.43
Bodo|14.40|67.28
Tromso|18.96|69.65
Namsos|11.49|64.47
Mosjoen|13.20|65.84
Mo i Rana|14.14|66.30
Narvik|17.43|68.44''',
 ('Baltico','narva'): '''Narva|28.19|59.38
Tallin|24.75|59.44
Tartu|26.73|58.38
Parnu|24.50|58.39
Riga|24.11|56.95
Liepaja|21.01|56.50
Daugavpils|26.54|55.87
Vilna|25.28|54.69
Kaunas|23.90|54.90
Klaipeda|21.14|55.71
Turku|22.27|60.45
Helsinki|24.94|60.17
Tampere|23.76|61.50
Lahti|25.66|60.98
Savonlinna|28.88|61.87
Oulu|25.47|65.01
Rovaniemi|25.73|66.50
Inari|27.03|68.91
Jyvaskyla|25.75|62.24
Kuopio|27.68|62.89
Kajaani|27.73|64.22
Sodankyla|26.59|67.42''',
 ('Caledonia','megalitico'): '''Stirling|-3.94|56.12
Dunkeld|-3.59|56.56
Inverness|-4.22|57.48
Elgin|-3.31|57.65
Aberdeen|-2.09|57.15
Thurso|-3.53|58.59
Vindolanda|-2.36|54.99
Carlisle|-2.94|54.90
Trimontium|-2.72|55.60''',
 ('Dacia','danubiano'): '''Drobeta|22.66|44.63
Oescus|24.47|43.70
Nicopolis|25.59|43.22
Durostorum|27.27|44.11''',
 ('Anatolia','anatolio'): '''Biga|27.24|40.23
Canakkale|26.40|40.15
Balikesir|27.88|39.65
Bursa|29.06|40.18''',
 ('Levante','semita'): '''Jerico|35.44|31.86
Hebron|35.10|31.53
Gaza|34.45|31.50
Ascalon|34.57|31.67
Megido|35.18|32.58
Hazor|35.57|33.02
Damasco|36.29|33.51
Bosra|36.48|32.52
Palmira|38.27|34.55
Hama|36.75|35.13
Qatna|36.87|34.84
Carquemis|38.00|36.83
Mari|40.89|34.55
Terqa|40.56|34.92''',
 ('Sinai','semita'): '''Pelusium|32.54|31.04
Qantara|32.31|30.85
Suez|32.55|29.97
Nakhl|33.74|29.90
Arish|33.80|31.13
Aqaba|35.00|29.53
Petra|35.44|30.33
Maan|35.73|30.20
Amman|35.91|31.95
Negev|34.80|30.85''',
 ('Mesopotamia','sumerio'): '''Nippur|45.23|32.13
Isin|45.28|31.92
Shuruppak|45.50|31.78
Eridu|45.99|30.82
Adab|45.97|31.93
Larsa|45.85|31.29
Eshnunna|44.72|33.48
Sippar|44.25|33.06
Samarra|43.88|34.20
Hatra|42.72|35.59
Nuzi|44.27|35.17
Tell Brak|41.09|36.67''',
 ('Golfo Persico','dilmun'): '''Qalat al Bahrain|50.52|26.23
Saar|50.48|26.20
Failaka|48.34|29.44
Tarut|50.08|26.57
Hofuf|49.59|25.36
Dhahran|50.10|26.29
Umm an Nar|54.50|24.44
Hili|55.76|24.29
Bat|56.74|23.27
Sohar|56.74|24.35''',
 ('Iran','iranio'): '''Qazvin|50.00|36.27
Hamadan|48.52|34.80
Kermanshah|47.07|34.31
Kashan|51.44|33.98
Isfahan|51.68|32.65
Yazd|54.37|31.90
Shiraz|52.58|29.59
Jiroft|57.74|28.68
Shahr-e Sukhteh|61.33|30.60
Zahedan|60.86|29.50
Nishapur|58.79|36.21
Sabzevar|57.68|36.21
Mashhad|59.62|36.30
Bojnurd|57.33|37.47
Damghan|54.35|36.17
Semnan|53.39|35.57''',
 ('Kopet Dag','namazga'): '''Anau|58.53|37.90
Namazga Tepe|59.15|37.92
Altyn Depe|60.47|36.86
Djeitun|58.33|38.12
Abiverd|59.58|37.28
Dushak|60.04|37.19
Serakhs|61.17|36.54
Tejen|60.51|37.38''',
 ('Margiana','namazga'): '''Merv|62.18|37.66
Gonur|62.02|38.21
Togolok|62.14|37.98
Mary|61.83|37.60
Yoloten|62.36|37.30
Bayramaly|62.17|37.62''',
 ('Bactria','namazga'): '''Balkh|66.90|36.76
Shortugai|69.41|37.30
Termez|67.28|37.22
Kunduz|68.87|36.73
Dushanbe|68.79|38.56
Hisor|68.55|38.53
Herat|62.20|34.35
Maymana|64.77|35.92
Andkhoy|65.12|36.95
Shibirghan|65.75|36.67''',
 ('Sogdiana','kelteminar'): '''Samarcanda|66.97|39.65
Bujara|64.42|39.77
Panjakent|67.61|39.50
Shahrisabz|66.83|39.06
Navoi|65.38|40.09
Jizzakh|67.84|40.12
Tashkent|69.24|41.30
Khujand|69.62|40.28
Kokand|70.94|40.53
Fergana|71.78|40.39''',
 ('Corasmia','kelteminar'): '''Khiva|60.36|41.38
Urgench|60.63|41.55
Nukus|59.61|42.46
Ayaz Kala|61.03|42.01
Kunya Urgench|59.16|42.32
Dargan|62.00|40.48
Turkmenabat|63.58|39.08''',
 ('Estepa Central','botai'): '''Botai|67.65|53.30
Kokshetau|69.38|53.28
Petropavl|69.16|54.88
Kostanay|63.62|53.22
Torgai|63.50|49.63
Astana|71.43|51.17
Karaganda|73.09|49.80
Zhezkazgan|67.71|47.80
Pavlodar|76.97|52.29
Ekibastuz|75.32|51.72
Turkestan|68.27|43.30
Shymkent|69.59|42.32
Taraz|71.37|42.90
Bishkek|74.61|42.87
Almaty|76.89|43.24
Issyk Kul|77.10|42.45
Atbasar|68.36|51.82
Akkol|70.94|52.00
Esil|66.40|51.95
Ereymentau|73.10|51.60
Kurchatov|78.54|50.76''',
 ('Altai','afanasievo'): '''Afanasievo|91.30|53.90
Abakan|91.44|53.72
Minusinsk|91.69|53.71
Gorno Altaysk|85.96|51.96
Biysk|85.21|52.54
Barnaul|83.77|53.35
Ust Kamenogorsk|82.61|49.95
Semey|80.23|50.41
Zaysan|84.87|47.47
Kosh Agach|88.67|50.00
Ridder|83.50|50.34
Ust Kan|84.76|50.93
Tashtagol|87.89|52.76
Abaza|90.10|52.65'''}
 for (region,culture),rows in groups.items():
  for row in rows.splitlines():
   name,lon,lat=row.split('|')
   if any(p['name']==name for p in w['provinces']):continue
   w['provinces'].append(dict(id=len(w['provinces']),name=name,lon=float(lon),lat=float(lat),region=region,culture=culture,terrain='llanura',resource='trigo',pop=2.5,dev=1,coastal=False,adj=[],sea_adj=[]))
 # Biomes are independent of relief: a mountain or coast may have different climates.
 biomes=[('tundra','Tundra',.40,1.15,1.45,[150,168,155]),('taiga','Taiga',.65,1.20,1.40,[72,113,92]),('bosque','Bosque templado',.95,1.15,1.20,[85,139,75]),('mediterraneo','Matorral mediterraneo',.95,1.05,1.05,[158,160,91]),('pradera','Pradera',1.10,1.,1.,[154,177,91]),('estepa','Estepa',.75,1.,1.15,[175,170,110]),('desierto','Desierto',.35,1.,1.45,[213,185,125]),('humedal','Humedal',1.05,1.20,1.35,[96,151,133]),('alpino','Alpino',.50,1.30,1.60,[162,163,154])]
 write('biomes.json',[dict(id=i,name=n,food=f,defense=d,move=m,color=c) for i,n,f,d,m,c in biomes])
 for p in w['provinces']:
  lat,lon=p['lat'],p['lon'];t=p['terrain']
  p['continent']='Africa' if p['region'] in ['Egipto','Nubia','Magreb','Libia','Sahara'] else 'Europa' if p['region'] in ['Iberia','Germania','Escandinavia','Baltico','Nordico','Estepa','Galia','Galias','Italia','Grecia','Egeo','Creta','Danubio','Balcanes','Dacia','Britania','Irlanda','Caledonia','Helvecia','Atlantico','Sardena'] else 'Asia'
  if lat>=66:b='tundra'
  elif lat>=59:b='taiga'
  elif t=='montana':b='alpino'
  elif t=='marisma':b='humedal'
  elif t in ['desierto','oasis'] or p['region'] in ['Arabia','Sahara','Golfo Persico','Corasmia']:b='desierto'
  elif p['region'] in ['Estepa','Estepa Central','Altai','Bactria','Sogdiana','Kopet Dag','Margiana'] or (lon>47 and lat<40):b='estepa'
  elif lat>47:b='bosque' if t!='llanura' or lat>52 else 'pradera'
  elif p['continent']=='Europa' or p['region'] in ['Levante','Anatolia']:b='mediterraneo'
  else:b='pradera'
  p['biome']=b
  if p['id']>=256:
   p['terrain']='colina' if p['id']%5==0 else 'bosque' if b in ['taiga','bosque'] else 'desierto' if b=='desierto' else 'estepa' if b=='estepa' else 'llanura'
   p['resource']={'tundra':'ganado','taiga':'madera','bosque':'madera','mediterraneo':'aceite','pradera':'trigo','estepa':'caballos','desierto':'sal','humedal':'pescado','alpino':'piedra'}[b]
   if p['id']%7==0:p['resource']='cobre'
   if p['id']%11==0:p['resource']='lana'
 raw=(ROOT/'textures/earth.rgba').read_bytes()
 def water(lon,lat):
  x=int((lon+180)/360*2048)%2048;y=min(1023,max(0,int((90-lat)/180*1024)));i=(y*2048+x)*4;red,green,blue=raw[i:i+3];return blue>red+22 and blue>green+22
 def km(a,b):
  return 6371*math.hypot(math.radians(b['lon']-a['lon'])*math.cos(math.radians((a['lat']+b['lat'])/2)),math.radians(b['lat']-a['lat']))
 for p in w['provinces']:
  p['adj']=[];p['sea_adj']=[];p['explore_sea_adj']=[]
  p['coastal']=any(water(p['lon']+dx,p['lat']+dy) for dx,dy in [(.2,0),(-.2,0),(0,.2),(0,-.2)])
 for i,a in enumerate(w['provinces']):
  for b in w['provinces'][i+1:]:
   d=km(a,b)
   if d>450:continue
   wet=sum(water(a['lon']+(b['lon']-a['lon'])*t/20,a['lat']+(b['lat']-a['lat'])*t/20) for t in range(2,19))
   if d<=225 and wet<=2:
    a['adj'].append(b['id']);b['adj'].append(a['id'])
   elif a['coastal'] and b['coastal'] and wet>=3:
    if wet>=9:a['explore_sea_adj'].append(b['id']);b['explore_sea_adj'].append(a['id'])
    if d<=150:a['sea_adj'].append(b['id']);b['sea_adj'].append(a['id'])
 cultures=read('cultures.json');strategy=read('strategy.json');leaders=read('leaders.json')
 newcult=[('narva','Narva y comunidades del Baltico','Balticos','nordico','nat'),('dilmun','Dilmun y comunidades del Golfo','Dilmunitas','arabigo','anc'),('namazga','Namazga y comunidades del Oxus','Oxus','iranio','anc'),('kelteminar','Kelteminar y comunidades del Aral','Aral','estepario','nat'),('botai','Botai y comunidades de la estepa central','Botai','estepario','nat'),('afanasievo','Afanasievo y comunidades del Altai','Altai','estepario','anc')]
 aliases=[]
 for tag,name,short,base,religion in newcult:
  if not any(c['id']==tag for c in cultures):
   c=json.loads(json.dumps(next(c for c in cultures if c['id']==base)));c.update(id=tag,tag=tag,name=name,short=short,desc='Agrupacion regional de comunidades antiguas; una abstraccion de juego, no una genealogia etnica unica.');cultures.append(c)
  strategy['religion_by_culture'][tag]=religion
  strategy['unit_names'][tag]={k:v+' ('+short+')' for k,v in strategy['unit_names'][base].items()}
  leaders['names'][tag]=leaders['names'][base]
  aliases.append(('culture_'+tag,'culture_'+base))
 starts=[('Tribus del noroeste iberico','Castro de Barona'),('Tribus de la Meseta','Numancia'),('Tribus del Tajo','Toledo'),('Tribus del levante iberico','Sagunto'),('Comunidades del Guadalquivir','Carmona'),('Tribus del bajo Rin','Utrecht'),('Tribus del Elba','Magdeburgo'),('Tribus del Vistula','Torun'),('Tribus de Jutlandia','Jelling'),('Tribus de Svealand','Uppsala'),('Tribus de Noruega','Hamar'),('Comunidades de Narva','Narva'),('Comunidades de Finlandia','Lahti'),('Comunidades de Dilmun','Qalat al Bahrain'),('Comunidades de Namazga','Namazga Tepe'),('Comunidades de Margiana','Gonur'),('Comunidades de Bactria','Balkh'),('Comunidades de Sogdiana','Samarcanda'),('Comunidades de Corasmia','Ayaz Kala'),('Tribus de Botai','Botai'),('Tribus del Altai','Afanasievo')]
 for name,capital in starts:
  if any(t['name']==name for t in w['city_states']):continue
  p=next(p for p in w['provinces'] if p['name']==capital);color=next(c['color'] for c in cultures if c['id']==p['culture'])
  w['city_states'].append(dict(id=len(w['city_states'])+1,name=name,capital=p['id'],prov=capital,culture=p['culture'],gov='monarquia',color=color,playable=True,desc='Comunidades regionales antiguas; las etiquetas geograficas no implican estados modernos en 3300 a.C.'))
 index=read('countries/index.json')
 countries=[('dil','dilmun','Dilmun','dilmun','Qalat al Bahrain',-3000,'culture_arabigo'),('mar','margiana','Margiana','namazga','Gonur',-2000,'culture_iranio'),('kho','corasmia','Corasmia','kelteminar','Ayaz Kala',-600,'culture_estepario'),('bac','bactria','Bactria','namazga','Balkh',-600,'culture_iranio'),('sog','sogdiana','Sogdiana','kelteminar','Samarcanda',-600,'culture_estepario'),('sue','suevos','Suevos','nordico','Magdeburgo',-100,'culture_nordico')]
 for tag,id,name,culture,capital,year,media in countries:
  if tag not in index or tag=='dil':
   color=next(c['color'] for c in cultures if c['id']==culture)
   write('countries/'+tag+'.json',dict(tag=tag,id=id,name=name,culture_tag=culture,religion_tag=strategy['religion_by_culture'][culture],capital=capital,required_province=capital,earliest_year=year,era=0,gov='monarquia',color=color,adj=name,desc='Formacion regional historica con fecha aproximada. Su cultura inicial representa continuidad geografica de juego, no descendencia etnica directa.'));index.append(tag) if tag not in index else None
  aliases.append(('nation_'+id,media))
 catalog=read('media_catalog.json');credits=read('media/attributions.json')
 for new,old in aliases:
  if any(c['id']==new for c in catalog):continue
  item=dict(next(c for c in catalog if c['id']==old));item['id']=new
  item['note_es']='Ilustracion regional compartida con '+old+'. No es una bandera ni prueba de descendencia directa.'
  record=dict(next(c for c in credits if c['id']==old));record['id']=new;record['note_es']=item['note_es']
  catalog.append(item);credits.append(record);shutil.copyfile(ROOT/'media'/(old+'.rgba'),ROOT/'media'/(new+'.rgba'))
 w['meta']['playable_regions']=['Europa','Africa del Norte','Oriente Proximo','Asia Central']
 for path,data in [('world.json',w),('cultures.json',cultures),('strategy.json',strategy),('leaders.json',leaders),('countries/index.json',index),('media_catalog.json',catalog),('media/attributions.json',credits),('media/index.json',[{'id':x['id']} for x in credits])]:write(path,data)
 print(len(w['provinces']),'provinces;',len(w['city_states']),'tribes;',len(cultures),'cultures;',len(index),'countries')
if __name__=='__main__':expand()

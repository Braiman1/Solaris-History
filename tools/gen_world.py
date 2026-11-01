#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Solaris History - Generador de contenido del mundo (3300 a.C.)
Genera content/world.json, cultures.json, nations.json, units.json,
buildings.json, eras.json y leaders.json.

Las provincias usan coordenadas reales (lat/lon) y las adyacencias se
calculan por distancia geodesica + rutas maritimas comprobadas contra la
mascara de tierra del mapa de la Tierra.
"""
import json, math, os, sys, random
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTENT = ROOT / "content"
CONTENT.mkdir(exist_ok=True)
random.seed(3300)

# ---------------------------------------------------------------------------
# PROVINCIAS: nombre|lon|lat|terreno|recurso|cultura|poblacion|desarrollo|region
# Poblacion en miles de habitantes. 165 provincias historicas reales.
# ---------------------------------------------------------------------------
RAW_PROVINCES = """
Buto|30.75|31.05|marisma|pescado|egipcio|6|1|Egipto
Sais|30.95|30.97|valle|lino|egipcio|7|1|Egipto
Menfis|31.25|29.85|valle|trigo|egipcio|16|3|Egipto
Heliopolis|31.35|30.13|valle|cebada|egipcio|9|2|Egipto
Fayum|30.60|29.30|oasis|trigo|egipcio|5|1|Egipto
Heracleopolis|30.90|28.80|valle|lino|egipcio|7|1|Egipto
Abidos|31.90|26.18|valle|cebada|egipcio|8|2|Egipto
Hieracompolis|32.78|25.10|valle|trigo|egipcio|9|2|Egipto
Tebas|32.66|25.70|valle|trigo|egipcio|10|2|Egipto
Edfu|32.88|24.98|valle|trigo|egipcio|6|1|Egipto
Elefantina|32.90|24.10|valle|piedra|egipcio|5|1|Egipto
Oasis de Jarga|30.50|25.40|oasis|sal|egipcio|1|0|Egipto
Desierto Occidental|28.00|27.00|desierto|piedra|egipcio|1|0|Egipto
Sinai|33.80|29.30|montana|cobre|egipcio|2|1|Egipto
Nubia|32.00|22.00|valle|oro|nubio|4|1|Nubia
Kerma|30.50|19.60|valle|ganado|nubio|5|1|Nubia
Cirenaica|21.80|32.80|costa|aceite|bereber|3|1|Libia
Marmarica|24.50|31.40|desierto|ganado|bereber|1|0|Libia
Biblos|35.65|34.12|costa|cedro|semita|9|2|Levante
Ugarit|35.80|35.60|costa|vino|semita|7|2|Levante
Jerico|35.44|31.87|oasis|trigo|semita|6|2|Levante
Siquem|35.28|32.20|colina|aceite|semita|6|1|Levante
Gaz|34.45|31.50|costa|vino|semita|5|1|Levante
Hazor|35.57|33.02|colina|trigo|semita|5|1|Levante
Damasco|36.30|33.50|oasis|trigo|semita|8|2|Levante
Amon|35.93|31.95|colina|ganado|semita|4|1|Levante
Ebla|36.80|35.80|llanura|aceite|semita|10|2|Levante
Hama|36.75|35.13|valle|trigo|semita|6|1|Levante
Alepo|37.15|36.20|llanura|trigo|semita|8|2|Levante
Tadmor|38.30|34.60|oasis|sal|semita|2|1|Levante
Arwad|35.85|34.85|isla|pescado|semita|3|1|Levante
Edom|35.20|30.30|montana|cobre|semita|3|1|Levante
Qadesh|36.50|34.50|valle|vino|semita|4|1|Levante
Eridu|45.99|30.81|marisma|pescado|sumerio|8|2|Mesopotamia
Ur|46.10|30.96|llanura|cebada|sumerio|14|3|Mesopotamia
Uruk|45.60|31.30|llanura|trigo|sumerio|18|3|Mesopotamia
Larsa|45.85|31.28|llanura|cebada|sumerio|9|2|Mesopotamia
Nippur|45.23|32.13|llanura|cebada|sumerio|10|2|Mesopotamia
Lagash|46.40|31.40|llanura|cebada|sumerio|8|2|Mesopotamia
Umma|45.90|31.60|llanura|cebada|sumerio|7|1|Mesopotamia
Shuruppak|45.60|31.85|llanura|lino|sumerio|6|1|Mesopotamia
Kish|44.90|32.55|llanura|trigo|sumerio|10|2|Mesopotamia
Babilonia|44.42|32.54|llanura|trigo|semita|7|1|Mesopotamia
Sippar|44.30|33.10|llanura|cebada|semita|6|1|Mesopotamia
Eshnunna|44.70|33.75|valle|trigo|semita|6|1|Mesopotamia
Mari|40.90|34.55|valle|trigo|semita|9|2|Mesopotamia
Assur|43.26|35.46|colina|trigo|semita|7|2|Mesopotamia
Ninive|43.15|36.36|valle|trigo|semita|8|2|Mesopotamia
Erbil|44.00|36.19|llanura|trigo|semita|5|1|Mesopotamia
Nagar|40.20|36.40|llanura|cebada|semita|6|1|Mesopotamia
Amida|40.20|37.90|valle|cobre|semita|4|1|Mesopotamia
Marismas del Sur|47.50|30.60|marisma|pescado|sumerio|3|1|Mesopotamia
Susa|48.25|32.19|valle|trigo|elamita|10|2|Elam
Anshan|52.90|29.90|montana|plata|elamita|5|1|Elam
Tepe Sialk|51.40|33.90|oasis|cobre|iranio|4|1|Iran
Tepe Hissar|54.50|36.20|colina|cobre|iranio|3|1|Iran
Gorgan|54.50|37.00|bosque|madera|iranio|2|1|Iran
Kerman|57.00|30.30|montana|piedra|iranio|2|1|Iran
Rayy|51.40|35.60|llanura|trigo|iranio|4|1|Iran
Tabriz|46.30|38.10|montana|cobre|iranio|3|1|Iran
Hasanlu|45.50|37.00|valle|ganado|iranio|3|1|Iran
Persepolis|53.00|30.00|colina|ganado|iranio|4|1|Iran
Catalhoyuk|32.80|37.60|llanura|trigo|anatolio|12|3|Anatolia
Hattusa|34.60|40.00|montana|cobre|anatolio|5|1|Anatolia
Troya|26.20|39.95|costa|pescado|anatolio|8|2|Anatolia
Mersin|34.60|36.80|costa|trigo|anatolio|5|1|Anatolia
Gobekli Tepe|38.90|37.20|colina|piedra|anatolio|4|2|Anatolia
Cayonu|39.60|38.20|valle|trigo|anatolio|4|1|Anatolia
Kanesh|35.50|38.80|llanura|cebada|anatolio|5|1|Anatolia
Arslantepe|38.30|38.35|valle|trigo|anatolio|6|2|Anatolia
Van|43.40|38.50|montana|obsidiana|anatolio|3|1|Anatolia
Antalya|30.70|36.90|costa|madera|anatolio|4|1|Anatolia
Efeso|27.30|37.90|costa|aceite|anatolio|5|1|Anatolia
Sesklo|22.90|39.40|colina|trigo|egeo|6|2|Grecia
Atenas|23.70|37.98|colina|aceite|egeo|7|2|Grecia
Micenas|22.75|37.73|colina|ganado|egeo|4|1|Grecia
Esparta|22.40|37.10|valle|aceite|egeo|4|1|Grecia
Corinto|22.90|37.90|costa|vino|egeo|4|1|Grecia
Tebas Griega|23.30|38.30|llanura|trigo|egeo|4|1|Grecia
Knossos|25.15|35.30|isla|aceite|egeo|10|3|Creta
Festos|24.90|35.05|isla|vino|egeo|5|1|Creta
Chania|24.00|35.50|isla|cedro|egeo|4|1|Creta
Lemnos|25.00|39.90|isla|vino|egeo|2|1|Egeo
Lesbos|26.50|39.20|isla|aceite|egeo|3|1|Egeo
Rodas|28.20|36.40|isla|pescado|egeo|3|1|Egeo
Kition|33.60|34.90|isla|cobre|chipriota|4|2|Chipre
Pafos|32.40|34.80|isla|vino|chipriota|3|1|Chipre
Vinca|20.50|44.80|valle|trigo|balcanico|8|2|Balcanes
Varna|27.90|43.20|costa|oro|balcanico|7|2|Balcanes
Plovdiv|24.75|42.15|valle|trigo|balcanico|4|1|Balcanes
Dalmacia|16.50|43.50|costa|madera|balcanico|3|1|Balcanes
Butrinto|20.00|39.70|costa|pescado|balcanico|3|1|Balcanes
Cucuteni|27.60|47.00|llanura|trigo|danubiano|8|2|Danubio
Tripilia|30.50|48.50|llanura|trigo|danubiano|7|2|Danubio
Moldavia|29.00|47.50|llanura|ganado|danubiano|4|1|Danubio
Alfold|19.20|47.00|llanura|ganado|danubiano|6|2|Danubio
Wachau|15.40|48.20|valle|vino|danubiano|4|1|Danubio
Bohemia|14.40|50.00|bosque|piedra|danubiano|4|1|Danubio
Moravia|17.00|49.40|valle|trigo|danubiano|4|1|Danubio
Silesia|17.00|51.00|llanura|trigo|danubiano|3|1|Danubio
Polonia|18.50|52.70|llanura|ganado|nordico|3|1|Nordico
Este de Polonia|22.00|52.00|bosque|madera|nordico|2|1|Nordico
Lituania|24.00|55.00|bosque|madera|nordico|2|1|Nordico
Letonia|24.50|57.00|bosque|madera|nordico|2|1|Nordico
Estonia|25.00|58.50|bosque|pescado|nordico|2|1|Nordico
Kiev|30.50|50.50|bosque|trigo|danubiano|4|1|Danubio
Estepa Pontica|35.00|47.50|estepa|caballos|estepario|3|1|Estepa
Crimea|34.50|45.00|estepa|trigo|estepario|3|1|Estepa
Volga|44.00|48.50|estepa|ganado|estepario|3|1|Estepa
Maykop|40.00|44.00|montana|cobre|caucasico|4|2|Caucaso
Colchis|42.00|42.00|bosque|madera|caucasico|3|1|Caucaso
Kura-Araxes|44.50|41.00|valle|cobre|caucasico|4|1|Caucaso
Lacio|12.50|41.80|colina|ganado|italico|4|1|Italia
Etruria|11.80|42.25|costa|cobre|italico|6|2|Italia
Cumas|14.10|40.85|costa|vino|italico|4|1|Italia
Padania|11.00|44.70|llanura|trigo|italico|5|1|Italia
Umbria|12.50|43.00|colina|trigo|italico|3|1|Italia
Apulia|16.50|41.00|llanura|aceite|italico|3|1|Italia
Calabria|16.50|39.00|montana|madera|italico|2|1|Italia
Venecia|12.40|45.40|marisma|pescado|italico|3|1|Italia
Siracusa|15.30|37.10|isla|trigo|italico|5|2|Italia
Segesta|12.80|37.90|isla|vino|italico|3|1|Italia
Cerdana|9.00|40.50|isla|obsidiana|sardino|4|2|Sardena
Corcega|9.20|42.10|isla|madera|sardino|3|1|Sardena
Los Millares|-2.50|36.95|costa|cobre|iberico|6|2|Iberia
Valencina|-6.00|37.50|llanura|trigo|iberico|4|1|Iberia
Lisboa|-9.10|38.70|costa|pescado|iberico|3|1|Iberia
Galicia|-8.40|42.90|costa|pescado|megalitico|2|1|Atlantico
Cantabria|-4.00|43.20|montana|ganado|megalitico|2|1|Atlantico
Cataluna|1.80|41.60|costa|vino|iberico|3|1|Iberia
Pirineos|0.50|42.60|montana|piedra|megalitico|1|1|Atlantico
Aquitania|0.00|44.80|valle|vino|megalitico|3|1|Galias
Bretana|-2.80|48.20|costa|piedra|megalitico|3|1|Galias
Normandia|0.20|49.20|costa|trigo|megalitico|3|1|Galias
Paris|2.30|48.85|valle|trigo|megalitico|4|1|Galias
Borgona|4.90|47.30|valle|vino|megalitico|3|1|Galias
Provenza|5.40|43.50|costa|aceite|megalitico|3|1|Galias
Limousin|1.30|45.50|bosque|ganado|megalitico|2|1|Galias
Alsacia|7.75|48.60|valle|trigo|danubiano|4|1|Danubio
Belgica|4.40|50.80|llanura|trigo|megalitico|3|1|Galias
Paises Bajos|5.30|52.10|marisma|pescado|nordico|3|1|Nordico
Alemania Norte|9.70|53.00|llanura|ganado|nordico|3|1|Nordico
Dinamarca|10.00|55.70|costa|pescado|nordico|3|1|Nordico
Escania|13.50|55.70|costa|pescado|nordico|2|1|Nordico
Noruega Sur|8.00|58.20|costa|madera|nordico|2|1|Nordico
Wessex|-1.80|51.20|llanura|trigo|megalitico|4|1|Britania
Anglia|0.90|52.60|llanura|trigo|megalitico|3|1|Britania
Gales|-3.80|52.40|montana|ganado|megalitico|2|1|Britania
Escocia|-3.50|56.50|montana|pescado|megalitico|2|1|Britania
Irlanda|-7.00|53.30|costa|ganado|megalitico|2|1|Britania
Orcadas|-3.00|59.00|isla|pescado|megalitico|1|1|Britania
Marruecos|-5.50|34.00|valle|trigo|bereber|4|1|Magreb
Argel|-3.00|36.70|costa|aceite|bereber|4|1|Magreb
Tunez|10.30|36.85|costa|aceite|bereber|6|2|Magreb
Numidia|6.50|35.50|llanura|caballos|bereber|3|1|Magreb
Leptis|13.20|32.60|costa|aceite|bereber|3|1|Magreb
Sahara Argelino|5.00|27.00|desierto|sal|bereber|1|0|Sahara
Fezzan|13.50|26.00|oasis|sal|bereber|2|1|Sahara
Sahara Egipcio|28.00|23.00|desierto|piedra|egipcio|1|0|Sahara
Hiyaz|39.50|24.00|desierto|ganado|arabigo|2|1|Arabia
Dilmun|50.50|26.00|isla|pescado|arabigo|3|1|Arabia
Magan|57.00|23.00|desierto|cobre|arabigo|2|1|Arabia
Yemen|44.50|15.50|montana|incienso|arabigo|4|2|Arabia
Kuwait|47.80|29.50|marisma|pescado|arabigo|2|1|Arabia
"""

TORN = {
    "valle":     dict(name="Valle fluvial",  food=1.35, defense=1.00, move=1.00, color=[154, 176, 92]),
    "llanura":   dict(name="Llanura",        food=1.10, defense=1.00, move=1.00, color=[178, 170, 96]),
    "colina":    dict(name="Colinas",        food=0.90, defense=1.25, move=1.20, color=[158, 150, 92]),
    "montana":   dict(name="Montana",        food=0.50, defense=1.60, move=1.70, color=[140, 132, 118]),
    "desierto":  dict(name="Desierto",       food=0.25, defense=1.10, move=1.40, color=[214, 190, 126]),
    "estepa":    dict(name="Estepa",         food=0.70, defense=1.00, move=1.05, color=[168, 174, 110]),
    "bosque":    dict(name="Bosque",         food=0.80, defense=1.20, move=1.35, color=[104, 138, 84]),
    "costa":     dict(name="Costa",          food=1.00, defense=1.00, move=1.00, color=[150, 178, 150]),
    "isla":      dict(name="Isla",           food=0.90, defense=1.15, move=1.10, color=[144, 176, 170]),
    "oasis":     dict(name="Oasis",          food=1.20, defense=1.00, move=1.05, color=[190, 184, 112]),
    "marisma":   dict(name="Marisma",        food=1.15, defense=1.15, move=1.35, color=[122, 150, 108]),
}

RESOURCES = {
    "trigo":     dict(name="Trigo",       kind="food",  value=1.0, color=[220, 192, 96]),
    "cebada":    dict(name="Cebada",      kind="food",  value=0.8, color=[205, 181, 110]),
    "pescado":   dict(name="Pescado",     kind="food",  value=0.9, color=[130, 176, 200]),
    "ganado":    dict(name="Ganado",      kind="food",  value=0.8, color=[170, 130, 92]),
    "madera":    dict(name="Madera",      kind="wood",  value=1.0, color=[112, 142, 88]),
    "cedro":     dict(name="Cedro",       kind="wood",  value=1.4, color=[96, 128, 80]),
    "piedra":    dict(name="Piedra",      kind="stone", value=1.0, color=[156, 152, 144]),
    "cobre":     dict(name="Cobre",       kind="copper",value=1.6, color=[196, 122, 74]),
    "estaño":    dict(name="Estano",      kind="copper",value=2.4, color=[176, 186, 196]),
    "oro":       dict(name="Oro",         kind="gold",  value=2.4, color=[228, 190, 72]),
    "plata":     dict(name="Plata",       kind="gold",  value=1.8, color=[198, 204, 214]),
    "sal":       dict(name="Sal",         kind="trade", value=1.6, color=[226, 226, 222]),
    "obsidiana": dict(name="Obsidiana",   kind="trade", value=2.0, color=[92, 84, 110]),
    "vino":      dict(name="Vino",        kind="trade", value=1.4, color=[146, 62, 86]),
    "aceite":    dict(name="Aceite",      kind="trade", value=1.4, color=[170, 164, 74]),
    "lino":      dict(name="Lino",        kind="trade", value=1.0, color=[208, 204, 170]),
    "incienso":  dict(name="Incienso",    kind="trade", value=2.6, color=[210, 188, 210]),
    "caballos":  dict(name="Caballos",    kind="military", value=1.8, color=[150, 116, 82]),
}

CULTURES = [
 dict(id="egipcio", name="Egipcio Predinastico", short="Egipcios", color=[201,162,39],
      desc="El Nilo dicta la vida: cosechas seguras, teocracia fuerte y un pueblo que venera a sus muertos.",
      mods=dict(farm=1.25,growth=1.15,tech=1.05,trade=1.00,mil=0.95,stab=1.10,naval=0.85,expansion=0.90)),
 dict(id="nubio", name="Nubio (Grupo A)", short="Nubios", color=[120,84,44],
      desc="Guerreros del alto Nilo y maestros del arco; ricos en oro y ganado, pero encajonados entre desiertos.",
      mods=dict(farm=0.95,growth=1.00,tech=0.90,trade=0.95,mil=1.15,stab=1.00,naval=0.70,expansion=1.00)),
 dict(id="sumerio", name="Sumerio", short="Sumerios", color=[46,139,139],
      desc="Inventores de la ciudad y la escritura. Canales, templos y contabilidad antes que nadie.",
      mods=dict(farm=1.10,growth=1.05,tech=1.20,trade=1.20,mil=1.00,stab=0.90,naval=0.80,expansion=1.05)),
 dict(id="semita", name="Semita Oriental", short="Semitas", color=[155,107,58],
      desc="Pastores y mercaderes de la media luna fertil; se adaptan a cualquier tierra y comercian con todos.",
      mods=dict(farm=0.95,growth=1.00,tech=1.05,trade=1.25,mil=1.05,stab=0.95,naval=0.90,expansion=1.10)),
 dict(id="elamita", name="Elamita", short="Elamitas", color=[122,79,160],
      desc="Montaneses de Susa y Anshan: arqueros temibles y una corte orgullosa de su independencia.",
      mods=dict(farm=0.90,growth=0.95,tech=1.00,trade=1.05,mil=1.15,stab=1.00,naval=0.60,expansion=1.00)),
 dict(id="anatolio", name="Anatolio Neolitico", short="Anatolios", color=[138,59,46],
      desc="Los primeros urbanistas del mundo. Obsidiana, trigo y ganado en las mesetas de Anatolia.",
      mods=dict(farm=1.10,growth=1.05,tech=1.15,trade=1.05,mil=1.00,stab=1.00,naval=0.80,expansion=1.00)),
 dict(id="egeo", name="Egeo Neolitico", short="Egeos", color=[47,111,158],
      desc="Islas, viento y comercio. Marinos natos con ciudades abiertas al mar y a las ideas.",
      mods=dict(farm=0.90,growth=1.00,tech=1.10,trade=1.25,mil=1.00,stab=1.00,naval=1.30,expansion=1.05)),
 dict(id="balcanico", name="Balcanico (Vinca)", short="Balcanicos", color=[138,90,43],
      desc="Metalurgicos pioneros del Danubio; ricos en cobre y oro, feroces defensores de sus valles.",
      mods=dict(farm=1.05,growth=1.00,tech=1.05,trade=0.95,mil=1.10,stab=0.95,naval=0.90,expansion=1.10)),
 dict(id="danubiano", name="Danubiano (LBK)", short="Danubianos", color=[78,122,58],
      desc="Agricultores de las llanuras centrales. Aldeas grandes, campos de trigo y crecimiento constante.",
      mods=dict(farm=1.15,growth=1.10,tech=1.00,trade=0.95,mil=0.95,stab=1.00,naval=0.70,expansion=1.05)),
 dict(id="megalitico", name="Megalitico Atlantico", short="Megaliticos", color=[90,110,122],
      desc="Constructores de piedra y navegantes de cabotaje. Sociedades rituales muy cohesionadas.",
      mods=dict(farm=0.95,growth=0.95,tech=0.90,trade=0.90,mil=1.05,stab=1.15,naval=1.00,expansion=1.00)),
 dict(id="nordico", name="Nordico Neolitico", short="Nordicos", color=[90,106,74],
      desc="Cazadores, pescadores y navegantes del Baltico y el Mar del Norte. Duros en invierno.",
      mods=dict(farm=0.90,growth=1.00,tech=0.95,trade=1.00,mil=1.05,stab=1.00,naval=1.20,expansion=1.05)),
 dict(id="estepario", name="Estepario (Yamnaya)", short="Esteparios", color=[110,127,62],
      desc="Jinetes y pastores de la estepa pontica. Movilidad brutal y hambre de horizontes.",
      mods=dict(farm=0.70,growth=1.00,tech=0.90,trade=0.90,mil=1.30,stab=0.90,naval=0.50,expansion=1.25)),
 dict(id="caucasico", name="Caucasico (Kura-Araxes)", short="Caucasicos", color=[74,90,138],
      desc="Metalurgicos de montana entre dos mares; artesanos del bronce y guardianes de los pasos.",
      mods=dict(farm=0.90,growth=1.00,tech=1.10,trade=1.00,mil=1.10,stab=1.00,naval=0.60,expansion=1.00)),
 dict(id="iranio", name="Iranio de la Meseta", short="Iranios", color=[90,74,138],
      desc="Gentes de oasis y mesetas altas; comercio caravanero y arqueros de montana.",
      mods=dict(farm=0.85,growth=0.95,tech=1.05,trade=1.10,mil=1.05,stab=1.00,naval=0.50,expansion=1.05)),
 dict(id="bereber", name="Bereber (Libio)", short="Bereberes", color=[160,138,74],
      desc="Senores del Magreb y del desierto; nomadas, jinetes y guias de caravanas.",
      mods=dict(farm=0.80,growth=0.95,tech=0.85,trade=1.10,mil=1.10,stab=1.00,naval=1.05,expansion=1.10)),
 dict(id="arabigo", name="Arabigo Nomada", short="Arabigos", color=[176,138,90],
      desc="Caravaneros y criadores del desierto arabigo. Conocen cada pozo y cada ruta.",
      mods=dict(farm=0.70,growth=1.00,tech=0.85,trade=1.20,mil=1.05,stab=0.90,naval=0.80,expansion=1.15)),
 dict(id="italico", name="Italico Neolitico", short="Italicos", color=[122,59,90],
      desc="Aldeanos de colinas y llanuras italicas; comunidades rurales solidas y laboriosas.",
      mods=dict(farm=1.05,growth=1.05,tech=1.00,trade=1.00,mil=1.05,stab=1.05,naval=1.00,expansion=1.00)),
 dict(id="iberico", name="Iberico (Los Millares)", short="Ibericos", color=[176,106,58],
      desc="Metalurgicos del sureste iberico; cobre, murallas ciclopeas y fuerte sentido del clan.",
      mods=dict(farm=1.00,growth=1.00,tech=1.00,trade=1.00,mil=1.05,stab=1.00,naval=1.00,expansion=1.05)),
 dict(id="sardino", name="Sardo (Ozieri)", short="Sardos", color=[90,122,138],
      desc="Islenos de la obsidiana y los nuragas; artesanos y pastores difíciles de someter.",
      mods=dict(farm=0.90,growth=0.95,tech=1.00,trade=1.00,mil=1.00,stab=1.10,naval=1.15,expansion=0.95)),
 dict(id="chipriota", name="Chipriota", short="Chipriotas", color=[79,111,168],
      desc="Puente entre oriente y occidente; cobre, vino y puertos abiertos a todas las flotas.",
      mods=dict(farm=0.90,growth=1.00,tech=1.00,trade=1.20,mil=0.95,stab=1.00,naval=1.15,expansion=1.00)),
]

NAME_POOLS = {
 "egipcio": (["Narmer","Hor-Aha","Djer","Den","Semerkhet","Qaa","Iry-Hor","Escorpion"],
             ["Neithhotep","Merneith","Khenthap","Betrest","Nimaethap","Seshemetka"]),
 "nubio":   (["Aman","Sesostris","Kerma","Taharqa","Amaniastabarqa","Piye","Kashta","Sabrakamani"],
             ["Makeda","Amanirenas","Shanakdakhete","Nawidemak","Amanishakheto","Kandake"]),
 "sumerio": (["Enmerkar","Lugalbanda","Gilgamesh","Dumuzid","Enmebaragesi","Urukagina","Meskiaggasher","Lugalzagesi"],
             ["Kubaba","Puabi","Baranamtarra","Shagshag","Ninbanda","Enheduanna"]),
 "semita":  (["Sargon","Rimush","Manishtusu","Naram-Sin","Shamshi-Adad","Ibbi-Sin","Yasmah-Adad","Zimri-Lim"],
             ["Tashlultum","Sammuramat","Beltak","Iltani","Sibtu","Zakutu"]),
 "elamita": (["Kindattu","Humban","Shutruk","Untash","Puzur-Inshushinak","Tan-Ruhuratir","Kutir-Nahhunte","Shilhak"],
             ["Napir-Asu","Itti","Rishatum","Shala","Anunitu","Urtaku"]),
 "anatolio":(["Anitta","Pithana","Labarna","Hattusili","Mursili","Tudhaliya","Arnuwanda","Suppiluliuma"],
             ["Tawananna","Asmunikal","Puduhepa","Danuhepa","Gassulawiya","Malnigal"]),
 "egeo":    (["Minos","Radamantis","Sarpedon","Idomeneo","Agamenon","Menelao","Odiseo","Teseo"],
             ["Ariadna","Helena","Clitemnestra","Penelope","Electra","Andromaca"]),
 "balcanico":(["Zalmoxis","Sitalces","Cotys","Dromichaites","Oroles","Burebista","Deceneo","Moskon"],
             ["Bendis","Meda","Kotysa","Sarmis","Drypetis","Amadoka"]),
 "danubiano":(["Vindex","Marbod","Zbelut","Moget","Bato","Pannon","Sarmus","Tauriscus"],
             ["Sava","Dora","Liba","Mira","Tisa","Dunja"]),
 "megalitico":(["Belenos","Cernunnos","Lugus","Taranis","Diviciaco","Bituito","Ambicatus","Segovesus"],
             ["Epona","Rosmerta","Brigantia","Sulis","Nantosuelta","Andarta"]),
 "nordico": (["Haldan","Skjold","Frodi","Halfdan","Ragnar","Bjorn","Ingjald","Godfred"],
             ["Sigrid","Thyra","Gudrun","Astrid","Skadi","Hilda"]),
 "estepario":(["Wiro","Deiwos","Perkuno","Aryo","Skandho","Yemo","Manu","Kurgan"],
             ["Dana","Saraswati","Priya","Usha","Anahita","Arma"]),
 "caucasico":(["Tushpa","Sarduri","Menuas","Argishti","Rusa","Sargon","Erimena","Haldi"],
             ["Nwari","Anahit","Nane","Astghik","Varduhi","Tarhunna"]),
 "iranio":  (["Achaemenes","Teispes","Cambyses","Darius","Arsames","Cyrus","Phraortes","Deioces"],
             ["Atossa","Cassandane","Roxana","Artystone","Amestris","Parysatis"]),
 "bereber": (["Syphax","Masinissa","Juba","Bocchus","Gaia","Massyli","Hiempsal","Jugurtha"],
             ["Sophonisba","Eunoe","Dido","Tingis","Yuba","Kalis"]),
 "arabigo": (["Yatha","Karib","Shamir","Abkarib","Amir","Hassan","Wahb","Ilum"],
             ["Bilqis","Shams","Layla","Hind","Zabibe","Ishara"]),
 "italico": (["Romulo","Remo","Numitor","Aventino","Tarquino","Servio","Turno","Latinus"],
             ["Rea Silvia","Hersilia","Lucrecia","Tanaquil","Lavinia","Camila"]),
 "iberico": (["Indibil","Mandonio","Istolacio","Indortes","Arganthonios","Gargoris","Corribilo","Avaros"],
             ["Habidis","Nuk","Sos","Iberia","Astan","Nertha"]),
 "sardino": (["Sardus","Norax","Iolaus","Arbata","Ospitone","Hostio","Balaro","Amsicora"],
             ["Sardinia","Pira","Nora","Tharros","Sulci","Bosa"]),
 "chipriota":(["Kinyras","Teucro","Pygmalion","Onesilos","Salamis","Belos","Adonis","Kiniras"],
             ["Myrrha","Smyrna","Kinyra","Europa","Ledra","Paphos"]),
}

TRAITS = [
 dict(id="administrador", name="Administrador", desc="+8% ingresos", mods=dict(income=0.08)),
 dict(id="general", name="General", desc="+8% fuerza militar", mods=dict(mil=0.08)),
 dict(id="diplomatico", name="Diplomatico", desc="+10% estabilidad", mods=dict(stab=0.10)),
 dict(id="piadoso", name="Piadoso", desc="+6 legitimidad y +5% estabilidad", mods=dict(legit=6,stab=0.05)),
 dict(id="ambicioso", name="Ambicioso", desc="+10% expansion, -5 estabilidad", mods=dict(expansion=0.10,stab=-0.05)),
 dict(id="cruel", name="Cruel", desc="+10% fuerza militar, -10 estabilidad", mods=dict(mil=0.10,stab=-0.10)),
 dict(id="sabio", name="Sabio", desc="+10% tecnologia", mods=dict(tech=0.10)),
 dict(id="constructor", name="Constructor", desc="-15% coste de edificios", mods=dict(build=-0.15)),
 dict(id="enfermizo", name="Enfermizo", desc="-10 anos de esperanza de vida", mods=dict(life=-10)),
 dict(id="robusto", name="Robusto", desc="+10 anos de esperanza de vida", mods=dict(life=10)),
 dict(id="codicioso", name="Codicioso", desc="+10% ingresos, -5 estabilidad", mods=dict(income=0.10,stab=-0.05)),
 dict(id="carismatico", name="Carismatico", desc="+10 legitimidad", mods=dict(legit=10)),
]

# Ciudades-estado jugables / IA al inicio
CITY_STATES = [
 dict(name="Menfis",   prov="Menfis",   culture="egipcio", gov="teocracia", color=[212,175,55], playable=True,  desc="La ciudad del muro blanco, heredera de las tradiciones del Bajo y Alto Egipto."),
 dict(name="Uruk",     prov="Uruk",     culture="sumerio", gov="monarquia", color=[46,139,139], playable=True,  desc="La primera gran ciudad. Templos de adobe, canales y escritura cuneiforme."),
 dict(name="Catal Huyuk", prov="Catalhoyuk", culture="anatolio", gov="teocracia", color=[160,74,54], playable=True, desc="Milenios de vida urbana en la meseta de Konya; santuarios y obsidiana."),
 dict(name="Jerico",   prov="Jerico",   culture="semita",   gov="monarquia", color=[62,142,90], playable=True,  desc="Una de las ciudades mas antiguas del mundo, junto al Jordan y sus manantiales."),
 dict(name="Troya",    prov="Troya",    culture="anatolio", gov="monarquia", color=[201,123,60], playable=True,  desc="El control del estrecho de los Dardanelos vale mas que un reino."),
 dict(name="Knossos",  prov="Knossos",  culture="egeo",     gov="teocracia", color=[47,111,158], playable=True,  desc="El laberinto de Creta: aceite, bronce y una flota que domina el Egeo."),
 dict(name="Ur",       prov="Ur",       culture="sumerio", gov="monarquia", color=[70,160,150], playable=False, desc="Puerto del golfo y ciudad de la luna."),
 dict(name="Susa",     prov="Susa",     culture="elamita",  gov="monarquia", color=[122,79,160], playable=False, desc="La puerta de las montanas elamitas."),
 dict(name="Biblos",   prov="Biblos",   culture="semita",   gov="monarquia", color=[58,126,154], playable=False, desc="Emporio del cedro y del comercio maritimo levantino."),
 dict(name="Ebla",     prov="Ebla",     culture="semita",   gov="monarquia", color=[196,107,74], playable=False, desc="Senora de las llanuras sirias y de la ruta de la plata."),
 dict(name="Varna",    prov="Varna",    culture="balcanico",gov="teocracia", color=[150,110,50], playable=False, desc="Oro, cobre y necropolis principescas del Mar Negro."),
 dict(name="Los Millares", prov="Los Millares", culture="iberico", gov="monarquia", color=[176,106,58], playable=False, desc="Fortaleza del cobre en el sureste iberico."),
]

# Plantillas de naciones fundables (la cultura debe coincidir con la capital)
NATIONS = [
 dict(id="egipto",   name="Egipto",    culture="egipcio",  capital="Menfis",      color=[212,175,55], gov="monarquia", adj="Egipcio",   desc="El reino de las Dos Tierras, del Delta a Elefantina."),
 dict(id="sumeria",  name="Sumeria",   culture="sumerio",  capital="Uruk",        color=[46,139,139], gov="republica", adj="Sumerio",  desc="La liga de las ciudades del sur mesopotamico."),
 dict(id="acad",     name="Acad",      culture="semita",   capital="Kish",        color=[155,107,58], gov="monarquia", adj="Acadio",   desc="El imperio de los cuatro confines, bajo un solo rey."),
 dict(id="elam",     name="Elam",      culture="elamita",  capital="Susa",        color=[122,79,160], gov="monarquia", adj="Elamita",  desc="La meseta y la llanura unidas por los arqueros de Susa."),
 dict(id="siria",    name="Siria",     culture="semita",   capital="Ebla",        color=[196,107,74], gov="monarquia", adj="Sirio",    desc="El corazon de las rutas entre Mesopotamia y el mar."),
 dict(id="canaan",   name="Canaan",    culture="semita",   capital="Jerico",      color=[62,142,90], gov="teocracia", adj="Cananeo",  desc="Ciudades del Jordan y de la costa, entre Egipto y Asia."),
 dict(id="fenicia",  name="Fenicia",   culture="semita",   capital="Biblos",      color=[58,126,154], gov="republica", adj="Fenicio",  desc="Cedro, purpura y puertos que llegan a todo el Mediterraneo."),
 dict(id="asiria",   name="Asiria",    culture="semita",   capital="Assur",       color=[124,127,138], gov="monarquia", adj="Asirio",   desc="El dios Assur y un ejercito que no retrocede."),
 dict(id="babilonia",name="Babilonia", culture="semita",   capital="Babilonia",   color=[177,85,60], gov="monarquia", adj="Babilonio", desc="La puerta de los dioses y la ciudad de las leyes."),
 dict(id="hatti",    name="Hatti",     culture="anatolio", capital="Hattusa",     color=[138,59,46], gov="monarquia", adj="Hatti",    desc="El reino de las mil divinidades y de los carros de guerra."),
 dict(id="arzawa",   name="Arzawa",    culture="anatolio", capital="Efeso",       color=[168,92,62], gov="monarquia", adj="Arzawano", desc="Las ciudades libres del oeste anatolio."),
 dict(id="wilusa",   name="Wilusa",    culture="anatolio", capital="Troya",       color=[201,123,60], gov="monarquia", adj="Wilusano", desc="La Ilios de las leyendas, guardiana de los estrechos."),
 dict(id="creta",    name="Creta",     culture="egeo",     capital="Knossos",     color=[47,111,158], gov="teocracia", adj="Cretense", desc="La talasocracia del laberinto y del toro sagrado."),
 dict(id="micenas",  name="Micenas",   culture="egeo",     capital="Micenas",     color=[107,76,154], gov="monarquia", adj="Aqueo",    desc="Los aqueos de la edad del bronce, ricos en oro y carros."),
 dict(id="atenas",   name="Atenas",    culture="egeo",     capital="Atenas",      color=[62,127,168], gov="republica", adj="Ateniense",desc="La ciudad de la colina y del aceite de oliva."),
 dict(id="yamnaya",  name="Yamnaya",   culture="estepario",capital="Estepa Pontica",color=[110,127,62],gov="monarquia",adj="Yamnaya", desc="Los senores de los kurganes y de la estepa sin fin."),
 dict(id="tracia",   name="Tracia",    culture="balcanico",capital="Varna",       color=[150,110,50], gov="monarquia", adj="Tracio",   desc="Oro, caballos y montanas entre el Danubio y el Egeo."),
 dict(id="dacia",    name="Dacia",     culture="danubiano",capital="Cucuteni",    color=[78,122,58], gov="monarquia", adj="Dacio",    desc="Las grandes aldeas de la cultura Cucuteni-Tripilia."),
 dict(id="vincha",   name="Vincha",    culture="balcanico",capital="Vinca",       color=[160,82,45], gov="teocracia", adj="Vinchano", desc="La cultura del Danubio y de sus templos de barro."),
 dict(id="iberia",   name="Iberia",    culture="iberico",  capital="Los Millares",color=[176,106,58], gov="monarquia", adj="Ibero",    desc="Cobre, fortalezas y clanes del sureste de Iberia."),
 dict(id="tartessos",name="Tartessos", culture="iberico",  capital="Valencina",   color=[192,138,46], gov="monarquia", adj="Tartesio", desc="El reino del metal y de las rutas atlanticas."),
 dict(id="lusitania",name="Lusitania", culture="iberico",  capital="Lisboa",      color=[122,138,58], gov="republica", adj="Lusitano", desc="Pueblos del oeste iberico, pastores y guerreros."),
 dict(id="latinos",  name="Latinos",   culture="italico",  capital="Lacio",       color=[122,59,90], gov="monarquia", adj="Latino",   desc="Las aldeas del Lacio antes de la fundacion de Roma."),
 dict(id="etruria",  name="Etruria",   culture="italico",  capital="Etruria",     color=[139,90,139], gov="republica", adj="Etrusco",  desc="Los senores del metal y de los ritos de Italia central."),
 dict(id="sicanos",  name="Sicanos",   culture="italico",  capital="Siracusa",    color=[168,118,62], gov="monarquia", adj="Sicano",   desc="Los pueblos de la Sicilia del trigo y del vino."),
 dict(id="sardena",  name="Sardena",   culture="sardino",  capital="Cerdana",     color=[90,122,138], gov="republica", adj="Sardo",    desc="Obsidiana, nuragas y pastores de las montanas sardas."),
 dict(id="alashiya", name="Alashiya",  culture="chipriota",capital="Kition",      color=[79,111,168], gov="monarquia", adj="Alashiyo", desc="El reino del cobre de Chipre, entre Anatolia, Levante y Egipto.", era=2),
 dict(id="libia",    name="Libia",     culture="bereber",  capital="Cirenaica",   color=[160,138,74], gov="monarquia", adj="Libico",   desc="Las tribus libias entre el desierto y el mar."),
 dict(id="numidia",  name="Numidia",   culture="bereber",  capital="Numidia",     color=[176,146,74], gov="monarquia", adj="Numida",   desc="Jinetes numidas, la mejor caballeria de Africa."),
 dict(id="mauretania",name="Mauretania",culture="bereber", capital="Marruecos",   color=[154,106,42], gov="monarquia", adj="Mauritano",desc="El extremo occidental de Africa y las Columnas de Hercules."),
 dict(id="nubia",    name="Nubia",     culture="nubio",    capital="Kerma",       color=[107,74,42], gov="monarquia", adj="Nubio",    desc="El reino de Kerma, entre la primera y la tercera catarata."),
 dict(id="kush",     name="Kush",      culture="nubio",    capital="Kerma",       color=[74,107,58], gov="teocracia", adj="Kushita",  desc="Los arqueros del sur y el oro del desierto."),
 dict(id="arabia",   name="Arabia",    culture="arabigo",  capital="Hiyaz",       color=[176,138,90], gov="monarquia", adj="Arabigo",  desc="Las tribus del Hiyaz y las rutas de caravanas."),
 dict(id="dilmun",   name="Dilmun",    culture="arabigo",  capital="Dilmun",      color=[58,138,154], gov="republica", adj="Dilmunita",desc="El emporio del golfo, entre Mesopotamia y el Indo."),
 dict(id="saba",     name="Saba",      culture="arabigo",  capital="Yemen",       color=[160,82,45], gov="monarquia", adj="Sabeo",    desc="Incienso, mirra y las montanas verdes del sur."),
 dict(id="urartu",   name="Urartu",    culture="caucasico",capital="Van",         color=[74,90,138], gov="monarquia", adj="Urarteo",  desc="Fortalezas de montana en torno al lago Van."),
 dict(id="media",    name="Media",     culture="iranio",   capital="Rayy",        color=[90,74,138], gov="monarquia", adj="Medo",     desc="Los caballos de la meseta irania."),
 dict(id="persia",   name="Persia",    culture="iranio",   capital="Anshan",      color=[106,42,90], gov="monarquia", adj="Persa",    desc="La tierra de Anshan y de los grandes reyes.", era=2),
 dict(id="britania", name="Britania",  culture="megalitico",capital="Wessex",     color=[90,110,122], gov="republica",adj="Britano",  desc="Las islas del estaño y de los circulos de piedra."),
 dict(id="caledonia",name="Caledonia", culture="megalitico",capital="Escocia",     color=[58,90,122], gov="monarquia", adj="Caledonio",desc="Las tierras altas del norte."),
 dict(id="germania", name="Germania",  culture="nordico",  capital="Alemania Norte",color=[90,106,74],gov="monarquia",adj="Germanico",desc="Los bosques y marismas del norte."),
 dict(id="escandia", name="Escandia",  culture="nordico",  capital="Dinamarca",   color=[74,122,106], gov="monarquia", adj="Escandio", desc="Los estrechos y las islas del norte."),
]

UNITS = [
 dict(id="soldado", name="Soldado", plural="Soldados", era=0, gold=25, food=10, wood=0, stone=0,
      men=800, atk=10, defense=10, upkeep=0.8, speed=1.0, naval=False, icon="soldado",
      desc="Infanteria ligera de lanza corta y escudo de cuero. Barata y fiable."),
 dict(id="lancero", name="Lancero", plural="Lanceros", era=0, gold=32, food=12, wood=4, stone=0,
      men=800, atk=8, defense=16, upkeep=0.9, speed=0.9, naval=False, icon="lancero",
      desc="Formacion cerrada de lanzas largas. Excelente contra jinetes y en colinas."),
 dict(id="jinete", name="Jinete", plural="Jinetes", era=2, gold=60, food=20, wood=0, stone=0,
      men=400, atk=20, defense=8, upkeep=1.6, speed=1.8, naval=False, icon="jinete", requires="caballos",
      desc="Carros y jinetes de la estepa. Devastadores en campo abierto, fragiles en asedios."),
 dict(id="barco", name="Barco", plural="Barcos", era=0, gold=45, food=0, wood=25, stone=0,
      men=200, atk=8, defense=6, upkeep=0.8, speed=1.3, naval=True, icon="barco",
      desc="Galera de remos para comercio, desembarcos y control de las rutas maritimas."),
]

BUILDINGS = [
 dict(id="granja",   name="Granjas",          cost=dict(gold=40, wood=20, stone=0),  era=0, mods=dict(food=0.15), desc="Campos de regadio y graneros: +15% alimentos."),
 dict(id="pozo",     name="Pozo",             cost=dict(gold=30, wood=10, stone=15), era=0, mods=dict(growth=0.10), desc="Agua segura: +10% crecimiento de poblacion."),
 dict(id="muralla",  name="Muralla",          cost=dict(gold=60, wood=10, stone=45), era=0, mods=dict(defense=0.30), desc="Muros de adobe o piedra: +30% defensa provincial."),
 dict(id="taller",   name="Taller",           cost=dict(gold=35, wood=25, stone=0),  era=0, mods=dict(wood=0.20), desc="Carpinteria y herramientas: +20% madera."),
 dict(id="cantera",  name="Cantera",          cost=dict(gold=40, wood=20, stone=0),  era=1, mods=dict(stone=0.20), desc="Extraccion de piedra: +20% piedra."),
 dict(id="mina",     name="Mina",             cost=dict(gold=55, wood=30, stone=10), era=1, mods=dict(copper=0.25), desc="Galerias de cobre: +25% cobre."),
 dict(id="mercado",  name="Mercado",          cost=dict(gold=60, wood=15, stone=20), era=1, mods=dict(trade=0.20), desc="Ferias y rutas: +20% comercio."),
 dict(id="templo",   name="Templo",           cost=dict(gold=50, wood=10, stone=35), era=0, mods=dict(stability=0.06, legit=1.0), desc="Casa de los dioses: +6% estabilidad."),
 dict(id="escribas", name="Casa de Escribas", cost=dict(gold=55, wood=10, stone=20), era=1, mods=dict(tech=0.18), desc="Contables y archivos: +18% tecnologia."),
 dict(id="puerto",   name="Puerto",           cost=dict(gold=60, wood=40, stone=10), era=0, mods=dict(trade=0.12, naval=0.20), desc="Muelle y astillero: +comercio, permite barcos."),
 dict(id="cuartel",  name="Cuartel",          cost=dict(gold=50, wood=10, stone=30), era=0, mods=dict(manpower=0.25), desc="Levas organizadas: +25% mano de obra."),
 dict(id="ganaderia",name="Ganaderia",        cost=dict(gold=45, wood=15, stone=0),  era=0, mods=dict(food=0.08, trade=0.05), desc="Rebanos y pastos: +8% alimentos."),
]

ERAS = [
 dict(id=0, name="Neolitico Tardio", short="Neolitico", tech=0,    prod=1.00, start=-3300, desc="Aldeas agricolas, piedra pulida y primeras ciudades."),
 dict(id=1, name="Calcolitico",      short="Calcolitico",tech=120,  prod=1.10, start=-3300, desc="Cobre, rueda y jerarquias cada vez mas complejas."),
 dict(id=2, name="Edad del Bronce",  short="Bronce",    tech=420,  prod=1.25, start=-3300, desc="Bronce, carros de guerra y primeros imperios."),
 dict(id=3, name="Edad del Hierro",  short="Hierro",    tech=950,  prod=1.40, start=-3300, desc="Hierro, alfabeto y pueblos en movimiento."),
 dict(id=4, name="Epoca Clasica",    short="Clasica",   tech=1800, prod=1.60, start=-3300, desc="Ciudades-estado, filosofia y grandes imperios."),
]

GOVS = {
 "teocracia": dict(name="Teocracia", desc="El poder del templo. Sucesion por el clero; legitimidad alta, estabilidad dependiente de la fe.", legit=75, stab=0.10, election=0),
 "monarquia": dict(name="Monarquia", desc="El rey y su dinastia. Sucesion hereditaria; legitimidad media.", legit=60, stab=0.05, election=0),
 "republica": dict(name="Republica", desc="Consejo de notables. Elecciones periodicas: los lideres pueden ser reelegidos, pero siguen siendo mortales.", legit=50, stab=0.00, election=8),
}

def haversine(lon1, lat1, lon2, lat2):
    R = 6371.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp = math.radians(lat2 - lat1)
    dl = math.radians(lon2 - lon1)
    a = math.sin(dp/2)**2 + math.cos(p1)*math.cos(p2)*math.sin(dl/2)**2
    return 2*R*math.asin(math.sqrt(a))

def parse_provinces():
    out = []
    for line in RAW_PROVINCES.strip().splitlines():
        parts = [p.strip() for p in line.split("|")]
        if len(parts) != 9:
            raise SystemExit(f"Provincia mal formada: {line}")
        name, lon, lat, terr, res, cult, pop, dev, region = parts
        out.append(dict(name=name, lon=float(lon), lat=float(lat), terrain=terr,
                        resource=res, culture=cult, pop=float(pop), dev=int(dev), region=region))
    return out

def load_land_mask():
    try:
        from PIL import Image
        im = Image.open(CONTENT / "textures" / "earth_src.png").convert("RGB")
        im = im.resize((720, 360))
        px = im.load()
        def is_land(lon, lat):
            x = int((lon + 180.0) / 360.0 * 720) % 720
            y = int((90.0 - lat) / 180.0 * 360)
            y = max(0, min(359, y))
            r, g, b = px[x, y]
            # oceano: azul marino dominante (10,10,51); tierra: colores calidos/verdes
            if b > r + 22 and b > g + 22:
                return False
            if r < 6 and g < 6 and b < 12:
                return False
            return True
        return is_land
    except Exception as e:
        print("Aviso: sin mascara de tierra:", e)
        return lambda lon, lat: True

def main():
    provs = parse_provinces()
    if len({p["name"] for p in provs}) != len(provs):
        raise SystemExit("Hay nombres de provincia duplicados")
    is_land = load_land_mask()

    # Costa: si hay mar cerca del centro en un radio de ~1.6 grados.
    for p in provs:
        coastal = False
        for r in (0.4, 0.8, 1.2, 1.6):
            for a in range(0, 360, 30):
                dlon = r * 1.4 * math.cos(math.radians(a)) / max(0.35, math.cos(math.radians(p["lat"])))
                dlat = r * math.sin(math.radians(a))
                if not is_land(p["lon"] + dlon, p["lat"] + dlat):
                    coastal = True
                    break
            if coastal:
                break
        p["coastal"] = bool(coastal)

    # ------------------------------------------------------------------
    # Adyacencia: se muestrea el camino geodesico sobre la mascara de tierra.
    # Si el camino es mayoritariamente mar, el enlace es maritimo.
    # ------------------------------------------------------------------
    def sea_fraction(a, b, steps=28):
        sf = 0
        for k in range(1, steps):
            t = k / float(steps)
            # interpolacion esferica simplificada (suficiente a esta escala)
            lon = a["lon"] + (b["lon"] - a["lon"]) * t
            lat = a["lat"] + (b["lat"] - a["lat"]) * t
            if not is_land(lon, lat):
                sf += 1
        return sf / float(steps - 1)

    adj = {p["name"]: set() for p in provs}
    sea = {p["name"]: set() for p in provs}
    pairs = []
    for i, a in enumerate(provs):
        for b in provs[i+1:]:
            d = haversine(a["lon"], a["lat"], b["lon"], b["lat"])
            if d > 1400:
                continue
            pairs.append((d, a, b))

    pairs.sort(key=lambda t: t[0])
    for d, a, b in pairs:
        sf = sea_fraction(a, b)
        if (d <= 520 and sf < 0.15 and len(adj[a["name"]]) < 7 and len(adj[b["name"]]) < 7
                and not (a["terrain"] == "isla" and sf > 0.06)
                and not (b["terrain"] == "isla" and sf > 0.06)):
            adj[a["name"]].add(b["name"]); adj[b["name"]].add(a["name"])
        elif (a["coastal"] and b["coastal"] and d <= 1350 and sf > 0.55
              and len(sea[a["name"]]) < 6 and len(sea[b["name"]]) < 6):
            sea[a["name"]].add(b["name"]); sea[b["name"]].add(a["name"])

    # Islas: garantizar al menos dos rutas maritimas a la costa mas cercana.
    for p in provs:
        if not adj[p["name"]] or p["terrain"] == "isla":
            near = sorted((haversine(p["lon"], p["lat"], q["lon"], q["lat"]), q["name"])
                          for q in provs if q["name"] != p["name"] and q["coastal"])
            added = 0
            for d, nm in near:
                if len(sea[p["name"]]) >= 3:
                    break
                sea[p["name"]].add(nm); sea[nm].add(p["name"])
                added += 1
                if added >= 3:
                    break

    # IDs y owners
    provinces = []
    name_to_id = {}
    for i, p in enumerate(sorted(provs, key=lambda q: q["name"])):
        name_to_id[p["name"]] = i
        p["id"] = i
        provinces.append(p)

    owners = {}
    for cs in CITY_STATES:
        pid = name_to_id[cs["prov"]]
        cs["id"] = len(owners) + 1
        cs["capital"] = pid
        cs["is_city_state"] = True
        owners[pid] = cs["id"]

    for p in provinces:
        p["owner"] = owners.get(p["id"], 0)
        p["adj"] = sorted(name_to_id[n] for n in adj[p["name"]])
        p["sea_adj"] = sorted(name_to_id[n] for n in sea[p["name"]])
        p["buildings"] = []
        p["loyalty"] = 60.0
        p["occupied"] = False
        # eliminar duplicados tierra/mar
        p["sea_adj"] = [x for x in p["sea_adj"] if x not in p["adj"]]
        out = {k: v for k, v in p.items() if k not in ("name_to_id",)}
        out.pop("id", None)
        provinces[provinces.index(p)] = out
    # reindexar limpio
    for i, p in enumerate(provinces):
        p["id"] = i

    world = dict(
        meta=dict(world="Tierra", start_year=-3300, map_w=2048, map_h=1024,
                  playable_regions=["Europa", "Africa del Norte", "Oriente Proximo"]),
        terrains=TORN, resources=RESOURCES,
        provinces=provinces, city_states=CITY_STATES,
    )
    (CONTENT / "world.json").write_text(json.dumps(world, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "cultures.json").write_text(json.dumps(CULTURES, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "nations.json").write_text(json.dumps(NATIONS, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "units.json").write_text(json.dumps(UNITS, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "buildings.json").write_text(json.dumps(BUILDINGS, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "eras.json").write_text(json.dumps(ERAS, ensure_ascii=False, indent=1), encoding="utf-8")
    (CONTENT / "leaders.json").write_text(json.dumps(dict(traits=TRAITS, names=NAME_POOLS, governments=GOVS), ensure_ascii=False, indent=1), encoding="utf-8")

    print(f"OK: {len(provinces)} provincias, {len(CITY_STATES)} ciudades-estado, {len(NATIONS)} naciones fundables")
    land = sum(1 for p in provinces if p["coastal"])
    print(f"   costeras: {land}, con adyacencia terrestre: {sum(1 for p in provinces if p['adj'])}, con ruta maritima: {sum(1 for p in provinces if p['sea_adj'])}")
    # comprobar conectividad terrestre de la region
    seen = set([provinces[0]["id"]]); stack = [provinces[0]["id"]]
    while stack:
        cur = stack.pop()
        for n in provinces[cur]["adj"]:
            if n not in seen:
                seen.add(n); stack.append(n)
    print(f"   conectadas por tierra: {len(seen)}/{len(provinces)}")

if __name__ == "__main__":
    main()

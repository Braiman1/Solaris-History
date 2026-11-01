# Solaris History - informe de balance

Generado por `java/tools` (motor de contenido Java).

## Resumen

| Metrica | Valor |
|---|---:|
| Provincias | 164 |
| Ciudades-estado | 12 |
| Naciones fundables | 42 |
| Culturas | 20 |
| Unidades | 4 |
| Edificios | 12 |
| Errores de validacion | 0 |
| Avisos de validacion | 0 |

## Economia provincial (estimacion)

Formula: `(poblacion*0.10 + desarrollo*0.05) * alimento_terreno * (0.8 + valor_recurso*0.35) * comercio_cultura`

| Metrica | Valor |
|---|---:|
| Ingreso mensual medio | 0.73 oro |
| Ingreso mensual minimo | 0.03 oro |
| Ingreso mensual maximo | 2.96 oro |
| Poblacion media | 4.7k |
| Poblacion minima / maxima | 1.0k / 18.0k |

### Ingreso estimado por cultura

| Cultura | Oro/mes |
|---|---:|
| semita | 26.62 |
| sumerio | 15.10 |
| egipcio | 14.92 |
| egeo | 9.46 |
| anatolio | 8.57 |
| danubiano | 6.62 |
| megalitico | 5.44 |
| italico | 5.14 |
| bereber | 4.52 |
| balcanico | 3.99 |
| iranio | 3.62 |
| nordico | 3.15 |
| iberico | 2.36 |
| elamita | 2.21 |
| nubio | 1.71 |
| arabigo | 1.50 |
| caucasico | 1.49 |
| chipriota | 1.22 |
| sardino | 1.04 |
| estepario | 0.81 |

## Unidades

| Unidad | Era | Hombres | Oro | Alimento | ATQ | DEF | (ATQ+DEF)/100 oro | Mant. |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Soldado | 0 | 800 | 25 | 10 | 10 | 10 | 80.0 | 0.8 |
| Lancero | 0 | 800 | 32 | 12 | 8 | 16 | 75.0 | 0.9 |
| Jinete | 2 | 400 | 60 | 20 | 20 | 8 | 46.7 | 1.6 |
| Barco | 0 | 200 | 45 | 0 | 8 | 6 | 31.1 | 0.8 |

## Edificios

| Edificio | Era | Oro | Madera | Piedra | Efectos |
|---|---:|---:|---:|---:|---|
| Granjas | 0 | 40 | 20 | 0 | food +0.15 |
| Pozo | 0 | 30 | 10 | 15 | growth +0.10 |
| Muralla | 0 | 60 | 10 | 45 | defense +0.30 |
| Taller | 0 | 35 | 25 | 0 | wood +0.20 |
| Cantera | 1 | 40 | 20 | 0 | stone +0.20 |
| Mina | 1 | 55 | 30 | 10 | copper +0.25 |
| Mercado | 1 | 60 | 15 | 20 | trade +0.20 |
| Templo | 0 | 50 | 10 | 35 | stability +0.06, legit +1.00 |
| Casa de Escribas | 1 | 55 | 10 | 20 | tech +0.18 |
| Puerto | 0 | 60 | 40 | 10 | trade +0.12, naval +0.20 |
| Cuartel | 0 | 50 | 10 | 30 | manpower +0.25 |
| Ganaderia | 0 | 45 | 15 | 0 | food +0.08, trade +0.05 |

## Arranque de las ciudades jugables

| Ciudad | Cultura | Provincia | Pop. | Ingreso capital | Oro inicial | Unidades asequibles |
|---|---|---|---:|---:|---:|---:|
| Menfis | egipcio | Menfis | 16.0k | 2.72 | 140 | 4/4 |
| Uruk | sumerio | Uruk | 18.0k | 2.96 | 140 | 4/4 |
| Catal Huyuk | anatolio | Catalhoyuk | 12.0k | 1.79 | 140 | 4/4 |
| Jerico | semita | Jerico | 6.0k | 1.21 | 140 | 4/4 |
| Troya | anatolio | Troya | 8.0k | 1.05 | 140 | 4/4 |
| Knossos | egeo | Knossos | 10.0k | 1.67 | 140 | 4/4 |

## Naciones fundables por cultura

| Cultura | Naciones |
|---|---:|
| anatolio | 3 |
| arabigo | 3 |
| balcanico | 2 |
| bereber | 3 |
| caucasico | 1 |
| chipriota | 1 |
| danubiano | 1 |
| egeo | 3 |
| egipcio | 1 |
| elamita | 1 |
| estepario | 1 |
| iberico | 3 |
| iranio | 2 |
| italico | 3 |
| megalitico | 2 |
| nordico | 2 |
| nubio | 2 |
| sardino | 1 |
| semita | 6 |
| sumerio | 1 |

## Avisos de balance

- `jinete` cuesta 15.0 oro por cada 100 hombres.
- `barco` cuesta 22.5 oro por cada 100 hombres.

## Validacion

Sin errores. El motor puede cargar este contenido.

# Solaris History

Juego de estrategia en C++17 para **Windows + OpenGL**, con herramientas de contenido en Java 21. La simulación y sus pruebas también se compilan en Linux.

## Jugar

El menú incluye **Crear partida**, **Cargar partida**, **Opciones**, **Créditos** y **Salir**. En Opciones se elige Español o English, junto a las banderas de España y Reino Unido. La elección se conserva en `solaris_options.cfg`, en el directorio desde el que se inicia el juego.

- **F5 / F9:** guardar / cargar `solaris_quicksave.sav`.
- **Espacio / 1–4:** pausa / velocidad. **F1:** ayuda.
- **Menú:** vuelve al menú principal; guarda antes si quieres conservar la partida.
- **Tecnologías:** muestra un árbol de descubrimientos con imágenes, requisitos y bonificaciones; elige qué investigar con los puntos acumulados.
- **Recursos:** catálogo ilustrado de los 23 recursos, sus valores comerciales y las provincias propias que los producen.
- **Créditos:** galería de imágenes con sus autores y licencias. Al pasar el ratón por un símbolo se muestra su contexto histórico.

Las expediciones duran **15 años** en una región de la misma cultura y **25 años** en otra, con una sola expedición activa y cinco años de espera después de completarla. Cuestan 120 de oro, más 15 por cada provincia adicional y 100 si la cultura es distinta, y requieren 30 alimentos y 50 de estabilidad. Solo salen de provincias integradas al menos al 50 % y asentadas hace diez años. El territorio permanece libre hasta terminar; perder la ruta cancela la expedición sin reembolso. Los ejércitos no reclaman territorio vacío.

La expansión está limitada a **900 km de la capital**, pasos terrestres de hasta 225 km y travesías de hasta 150 km, estas últimas con navegación y puerto. Se reconstruyeron las conexiones geográficas para eliminar saltos entre regiones lejanas. Estos límites también se aplican a la IA.

Fundar una nación requiere **4 provincias en Egipto, Mesopotamia y el Levante, o 5 en las demás regiones**, además de 60 de oro, 5 de prestigio y 3 puntos tecnológicos acumulados. También exige la fecha y el territorio de origen del país. Desde 500 a. C., las tribus necesitan instituciones: escritura y leyes, o ayuda de una nación establecida. La ayuda cuesta al patrocinador 100 de oro y tarda diez años; transmite su cultura, instituciones y tres puntos tecnológicos. Roma transmite cultura latina.

El mapa tiene **515 provincias**, **46 grupos iniciales**, **23 recursos**, **27 tecnologías** y **59 opciones de países**. Se amplían Galia, Britania, Irlanda, Helvecia, el Danubio y los Balcanes, además de Iberia, Germania, Escandinavia, el Báltico, el Sinaí, el Levante, Mesopotamia, el golfo Pérsico y Asia central. Hay 27 culturas; las nuevas representan Narva, Dilmun, Namazga/Oxus, Kelteminar/Aral, Botai y Afanasievo/Altái. Las etiquetas iniciales son abstracciones regionales; las tradiciones protocélticas pasan a cultura celta desde 1200 a. C. Las formaciones posteriores incluyen arvernos, eduos, belgas, helvecios y otros pueblos, con fechas aproximadas. Roma conserva su fecha tradicional de 753 a. C.; Israel requiere Samaria y la fecha aproximada de 1000 a. C.

Desde **300 a. C.** el calendario avanza por días; los ingresos, la investigación y las expediciones siguen calculándose por meses. Se usa un calendario proléptico con año cero como convención de simulación. La **Luna** orbita alrededor de la Tierra en coordenadas del mundo, con una vuelta visual cada 90 segundos y una órbita inclinada. Puede pasar delante o detrás del planeta; girar la cámara cambia la perspectiva. Se puede mostrar u ocultar. Es decorativa, usa una textura de Wikimedia y continúa moviéndose durante la pausa; su escala y período no son astronómicos.

Si usas el paquete compilado, extrae toda la carpeta y ejecuta `Jugar.cmd`; no necesitas recompilar.

## Exploración y biomas

En una partida nueva solo conoces los alrededores de tu capital (350 km) y las fronteras inmediatas de tus provincias. La oscuridad oculta las provincias no descubiertas de cualquier continente, incluidos sus nombres, fronteras políticas, recursos y ejércitos. No se pueden seleccionar ni usar como destino militar. La lista diplomática muestra únicamente pueblos contactados. Los acontecimientos de provincias y países desconocidos tampoco aparecen en la crónica. El menú muestra una vista general de la Tierra como fondo.

- **Exploración**, en el panel nacional: envía una expedición terrestre por 40 de oro y 10 alimentos, o marítima por 80 de oro, 20 alimentos y 20 madera. Hay una misión de cada tipo por nación, de hasta 36 meses; se puede retirar antes. Las terrestres parten de la provincia propia seleccionada o de la capital. Las marítimas necesitan **Navegación a vela** y un puerto costero; el juego usa el puerto seleccionado o uno disponible.
- Los exploradores recorren rutas hacia la frontera del mapa conocido y revelan regiones gradualmente. Los biomas y el relieve retrasan las travesías terrestres; los pasos marítimos tardan dos meses y conectan costas a hasta 450 km. No cruzan países en guerra ni reclaman territorio. Estas rutas marítimas ampliadas son exclusivas de exploración: la colonización mantiene sus límites de 150 km por mar y 900 km desde la capital.
- **Diplomacia → Negociar:** mejora relaciones (+15 por 30 de oro, una vez al año). Los regalos suman 5 y una alianza establece al menos 60. Con paz y **relaciones ≥ 60**, pide mapas por 25 de oro o entrega los tuyos gratis. Solo se transmiten las provincias que el donante conoce; contactar un país no revela automáticamente todo su territorio. La IA también explora y solicita mapas.
- **Bioma**, en cada provincia: tundra, taiga, bosque templado, matorral mediterráneo, pradera, estepa, desierto, humedal o alpino. Es independiente del relieve: una costa o montaña puede tener distintos biomas. La descripción muestra sus factores de alimento, defensa y tiempo de desplazamiento. Se definen en `content/biomes.json`; cada provincia tiene `biome` y `continent`.

Los pueblos de Asia central y del norte son agrupaciones regionales de juego. No se supone descendencia étnica directa entre una comunidad de 3300 a. C. y una formación estatal posterior. Las comunidades sin formación propia pueden adoptar otra cultura mediante patrocinio. Las nuevas imágenes culturales reutilizan ilustraciones regionales acreditadas, identificadas como tales.

El guardado 5 conserva mapas por nación, relaciones, esperas diplomáticas y exploradores en tránsito. Las partidas anteriores incorporan las nuevas provincias y tribus y reconstruyen el conocimiento alrededor de sus capitales, territorios, ejércitos y contactos diplomáticos existentes.

## Compilar y probar

Windows: instala CMake >= 3.16 y MinGW y ejecuta **`run.cmd`**. El lanzador compila los cambios de forma incremental, copia el contenido actualizado y abre `.tools/build-windows/SolarisHistory.exe`. También puedes usar `build.cmd` para compilar sin abrir el juego. Las partidas y opciones se guardan en `.tools/build-windows/`.

El repositorio original contiene artefactos antiguos en `build/`; los lanzadores actualizados usan la carpeta nueva. Compilación manual:

```powershell
cmake -S engine -B .tools/build-windows -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .tools/build-windows --parallel 4
cd .tools/build-windows
.\SolarisHistory.exe
```

Linux, pruebas sin interfaz gráfica:

```sh
cmake -S engine -B /tmp/solaris-build -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/solaris-build --parallel 4
ctest --test-dir /tmp/solaris-build --output-on-failure
```

Herramientas Java, sin sobrescribir el JAR ni las clases versionadas:

```sh
mkdir -p /tmp/solaris-java
javac -encoding UTF-8 -d /tmp/solaris-java java/src/solaris/tools/*.java
java -cp /tmp/solaris-java solaris.tools.Main validate content
```

El formato **5** conserva calendario, leyes, integración provincial, patrocinio e historia religiosa. Migra partidas de los formatos 1–4 del mapa anterior, conserva sus conquistas y añade las nuevas provincias y tribus sin cambiar los IDs existentes. El nuevo equilibrio de expansión se aprecia desde el principio al crear una partida nueva. Las partidas inválidas se rechazan sin reemplazar la partida activa.

## Imágenes de Wikipedia / Wikimedia

Se incluyen **141 entradas gráficas derivadas de imágenes reales de Wikimedia Commons**, para idiomas, recursos, tecnologías, culturas, países, religiones y la Luna. Los nodos nuevos reutilizan ilustraciones acreditadas de su rama. El juego carga estas imágenes desde archivos locales; no necesita Internet para mostrarlas.

El catálogo `content/media_catalog.json` fija los archivos elegidos y su contexto. Las fuentes, autores, licencias, enlaces y SHA-256 están en `content/media/attributions.json`. Sus créditos también se consultan dentro del juego. Las miniaturas RGBA se distribuyen a 128 × 128 píxeles y conservan la proporción de la imagen. La textura esférica de la Luna usa además un archivo de 512 × 256, con dimensiones y SHA-256 propios en las atribuciones.

No se conocen banderas o escudos nacionales de muchas comunidades de 3300 a. C. Para ellas se muestran piezas arqueológicas o símbolos culturales documentados, identificados como tales. Por ejemplo: paletas de Naqada para el grupo egipcio, la doble corona para Egipto, el símbolo de Assur para Asiria, la lechuza de una moneda ateniense y monedas de Numidia. La loba romana y otros símbolos posteriores llevan una nota que aclara su época o su uso como identificación cultural. Algunos pueblos comparten un motivo regional; no se presenta como un escudo propio conservado.

Para reproducir o actualizar la importación se requiere acceso a `en.wikipedia.org`, `commons.wikimedia.org` y `upload.wikimedia.org`:

```sh
python3 -m pip install CairoSVG==2.8.2
python3 tools/fetch_wikimedia.py                 # reutiliza archivos con SHA-256 verificado
python3 tools/fetch_wikimedia.py --only flag_es --refresh
python3 tools/test_wikimedia.py
```

El importador limita las solicitudes, respeta las esperas ante HTTP 429/503 y conserva el avance. Rechaza licencias no libres y referencias externas en los SVG. Las cinco pruebas verifican el catálogo completo, sus texturas y atribuciones, la conversión SVG y el rechazo de contenido no libre. Después de una importación, `cmake --build` vuelve a copiar el contenido junto al ejecutable aunque no cambie el código C++.

La interfaz se comprobó con Wine 10 y Xvfb: menú en español e inglés, banderas, selección de grupos, nueva partida, recursos, tecnologías, créditos y guardado/carga. Es una verificación en Linux mediante Wine, no una ejecución en Windows nativo.

`tools/gen_world.py` regenera los datos e invoca la expansión de `tools/expand_content.py`. La expansión conserva los IDs existentes y es idempotente. No ejecutes la generación durante la preparación del entorno: modifica los datos del juego.

## Entorno de nube

Herramientas y artefactos locales: `/workspace/.solaris-env/`. El script de instalación y las instrucciones de inicio se guardan en el borrador del entorno. La publicación de la configuración es un paso separado en la interfaz del producto. Usa el checkout existente; cada tarea ya tiene un entorno aislado.

## Expansion de estrategia

- **Diplomacia → Negociar:** paz tras seis meses de guerra; alianzas defensivas (5 de prestigio), ruptura de alianzas y transferencias de 20 unidades de oro, alimentos, madera, piedra o cobre. Las alianzas impiden atacarse y sus miembros ayudan al defensor.
- **Títeres:** exige doce meses de guerra, el doble de fuerza militar y 100 de oro. El vasallo conserva su territorio y paga hasta 2 de oro al mes. Puede liberarse. **Anexar** exige diez años de vasallaje, 200 de oro y 60 de estabilidad; cuesta 10 de estabilidad y transfiere territorio y ejércitos. El jugador no se anexa automáticamente.
- **Tecnologías:** tres pestañas, militar, económica y social, con conexiones entre requisitos. Se elige qué investigar; los puntos disponibles son el total acumulado menos los invertidos. Cada nodo tiene coste, prerrequisitos y fecha mínima, hasta el año 0. La IA también investiga. La fundación sigue comprobando los 3 puntos totales.
- **Gestión nacional:** pestañas de decisiones, religión, cultura, militar, consejeros y leyes. Festivales, levas y reformas de gobierno cuestan 40 de oro y comparten una espera de cinco años. Para esta época, la ideología se representa mediante monarquía, república o teocracia, que dan tonos distintos al color base del país.
- **Religiones:** diez entradas con ventajas y desventajas: cinco familias regionales antiguas, judaísmo, cristianismo y tres ramas posteriores. Israel se funda con judaísmo y su icono de menorá. El cristianismo puede aparecer desde el año 30, únicamente si el judaísmo sobrevivió hasta el año cero. Su desaparición completa antes del cero bloquea permanentemente el cristianismo y sus variantes en esa partida. Se cuentan tanto estados como comunidades provinciales. Las ramas católica y ortodoxa tienen fecha 1054 y la protestante 1517; no están disponibles durante la Antigüedad. Las fechas y relaciones son reglas simplificadas del juego. Convertirse cuesta 100 de oro y 15 de estabilidad, con cinco años entre decisiones. Las teocracias solo pueden adoptar una evolución directa de su religión.
- **Leyes:** mandatos republicanos de 2, 4 u 8 años; monarquía absoluta (+5 % militar, −3 % estabilidad) o limitada (+5 % estabilidad, −2 % ingresos); convivencia o asimilación cultural. Las leyes cuestan 20 de oro y comparten la espera de cinco años de las decisiones. Cambiar la duración del mandato no retrasa una elección ya próxima.
- **Rebeliones:** las provincias conquistadas conservan cultura, religión, antiguo propietario e ideología. Pueden levantarse por independencia, religión, ideología o hambre según lealtad y descontento. La independencia puede restaurar un país derrotado. La convivencia conserva las culturas; la asimilación las integra gradualmente (unos 42 años con estabilidad 70). Una provincia completamente asimilada no se rebela.
- **Ejércitos:** cuatro líneas de evolución (infantería, arquería, caballería y flotas), con 16 tipos en total y nombres según la cultura. La pestaña militar permite modernizar unidades por 20 de oro, con la tecnología correspondiente y fuera de tránsito. Hay infantes de bronce/hierro, arqueros compuestos, jinetes acorazados antiguos, galeras, birremes y trirremes; no caballeros medievales en el año 0.
- **Datos de países:** `content/countries/index.json` enumera los 59 archivos individuales; Roma está en `content/countries/rom.json`, con `tag: rom`, `culture_tag` y `religion_tag`. El motor y las herramientas Java leen esta carpeta. `nations.json` queda como referencia heredada; edita los archivos individuales para cambiar los países. Las culturas tienen su `tag` y `religions.json` define los tags religiosos.
- **Partidas:** el formato 5 incluye las nuevas reglas y migra los formatos anteriores, además de conservar alianzas, títeres, religión e investigación.

La generación antigua de `tools/gen_world.py` no actualiza las nuevas líneas militares ni los tags; no debe utilizarse para regenerar el contenido ampliado sin adaptar primero el generador.

Las pruebas incluyen simulación, estrategia, fronteras y exploración (`ctest`), más `python3 tools/test_strategy_data.py`, `python3 tools/test_exploration_data.py` y `python3 tools/test_wikimedia.py`. El validador Java comprueba también biomas, continentes y rutas de exploración. Informa cinco advertencias: ingreso medio bajo al contar las numerosas provincias libres, y cuatro culturas sin formación estatal directa (megalítica, Narva, Botai y Afanasievo). Sus vías son las transiciones culturales o el patrocinio, sin inventar estados tempranos.

`tools/expand_exploration.py` reproduce de forma idempotente la ampliación geográfica, los biomas, las rutas y los grupos nuevos, conservando los IDs existentes. Ejecútalo únicamente al editar el contenido, no al preparar el entorno.

# Solaris History

Juego de estrategia en C++17 para **Windows + OpenGL**, con herramientas de contenido en Java 21. La simulación y sus pruebas también se compilan en Linux.

## Jugar

El menú incluye **Crear partida**, **Cargar partida**, **Opciones**, **Créditos** y **Salir**. En Opciones se elige Español o English, junto a las banderas de España y Reino Unido. La elección se conserva en `solaris_options.cfg`, en el directorio desde el que se inicia el juego.

- **F5 / F9:** guardar / cargar `solaris_quicksave.sav`.
- **Espacio / 1–4:** pausa / velocidad. **F1:** ayuda.
- **Menú:** vuelve al menú principal; guarda antes si quieres conservar la partida.
- **Tecnologías:** muestra descubrimientos, imágenes y bonificaciones, desbloqueados automáticamente por puntos acumulados.
- **Recursos:** catálogo ilustrado de los 23 recursos, sus valores comerciales y las provincias propias que los producen.
- **Créditos:** galería de imágenes con sus autores y licencias. Al pasar el ratón por un símbolo se muestra su contexto histórico.

Las expediciones de colonización duran **36 meses**, cuestan oro al comenzar y permiten una expedición activa por nación. El territorio sigue libre hasta terminar. Si otra nación lo conquista o el colonizador desaparece, la expedición se cancela sin reembolso. El progreso se guarda y carga.

Fundar una nación requiere **2 provincias, 60 de oro, 5 de prestigio y 3 puntos tecnológicos**. Los puntos no se gastan. El grupo egipcio puede fundar Egipto. El grupo latino tiene estados independientes: Alba Longa, Lavinium, Tusculum, Aricia y Roma. Sus fechas de aparición son aproximaciones de juego; la fecha tradicional de Roma es **753 a. C.** y no aparece en 3300 a. C. Las etiquetas culturales de la partida inicial son abstracciones, no una afirmación de que esos estados existieran entonces.

El mapa tiene **184 provincias**, **23 recursos**, **8 tecnologías** y **46 opciones de naciones**. Arcilla, cerámica, lana, papiro y lapislázuli amplían las cadenas económicas existentes. La traducción se aplica al presentar textos, de modo que cambiar de idioma también traduce el historial de eventos.

Si usas el paquete compilado, extrae toda la carpeta y ejecuta `Jugar.cmd`; no necesitas recompilar.

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

Las partidas del mapa anterior no son compatibles con el mapa ampliado. Se rechazan al cargar sin reemplazar la partida activa. El formato nuevo conserva las expediciones en curso.

## Imágenes de Wikipedia / Wikimedia

Se incluyen **99 imágenes reales de Wikimedia Commons**: dos banderas de idioma, 23 recursos, ocho tecnologías, 20 culturas y 46 naciones. El juego carga estas imágenes desde archivos locales; no necesita Internet para mostrarlas.

El catálogo `content/media_catalog.json` fija los archivos elegidos y su contexto. Las fuentes, autores, licencias, enlaces y SHA-256 están en `content/media/attributions.json`. Sus créditos también se consultan dentro del juego. Las texturas RGBA se distribuyen a 128 × 128 píxeles y conservan la proporción de la imagen dentro del recuadro.

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

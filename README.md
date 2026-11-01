# Solaris History

Juego de estrategia en C++17 para **Windows + OpenGL**, con herramientas de contenido en Java 21. La simulación y sus pruebas también se compilan en Linux.

## Jugar

El menú incluye **Crear partida**, **Cargar partida**, **Opciones**, **Créditos** y **Salir**. En Opciones se elige Español o English, junto a las banderas de España y Reino Unido. La elección se conserva en `solaris_options.cfg`, en el directorio desde el que se inicia el juego.

- **F5 / F9:** guardar / cargar `solaris_quicksave.sav`.
- **Espacio / 1–4:** pausa / velocidad. **F1:** ayuda.
- **Menú:** vuelve al menú principal; guarda antes si quieres conservar la partida.
- **Tecnologías:** muestra descubrimientos y bonificaciones, desbloqueados automáticamente por puntos acumulados.

Las expediciones de colonización duran **36 meses**, cuestan oro al comenzar y permiten una expedición activa por nación. El territorio sigue libre hasta terminar. Si otra nación lo conquista o el colonizador desaparece, la expedición se cancela sin reembolso. El progreso se guarda y carga.

Fundar una nación requiere **2 provincias, 60 de oro, 5 de prestigio y 3 puntos tecnológicos**. Los puntos no se gastan. El grupo egipcio puede fundar Egipto. El grupo latino tiene estados independientes: Alba Longa, Lavinium, Tusculum, Aricia y Roma. Sus fechas de aparición son aproximaciones de juego; la fecha tradicional de Roma es **753 a. C.** y no aparece en 3300 a. C. Las etiquetas culturales de la partida inicial son abstracciones, no una afirmación de que esos estados existieran entonces.

El mapa tiene **184 provincias**, **23 recursos**, **8 tecnologías** y **46 opciones de naciones**. Arcilla, cerámica, lana, papiro y lapislázuli amplían las cadenas económicas existentes. La traducción se aplica al presentar textos, de modo que cambiar de idioma también traduce el historial de eventos.

## Compilar y probar

Windows: instala CMake >= 3.16 y MinGW, y compila en una carpeta nueva. El repositorio original contiene una carpeta `build/` con artefactos antiguos; no la reutilices.

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

**Las imágenes de Wikimedia todavía no se han descargado en este entorno: el proxy devuelve HTTP 403.** Los índices en `content/media/` están vacíos. Las banderas de idioma tienen una representación dibujada de respaldo; los recursos, tecnologías y pueblos mostrarán las imágenes locales cuando se importen.

El catálogo `content/media_catalog.json` incluye recursos, tecnologías, culturas y naciones. El importador consulta Wikipedia y Wikimedia Commons, acepta imágenes con licencias libres, conserva autores, licencias, enlaces y SHA-256 en `content/media/attributions.json`, y genera texturas RGBA locales. Los pueblos antiguos suelen carecer de banderas documentadas: sus ilustraciones arqueológicas se identifican como ilustraciones, sin presentarlas como banderas históricas.

Después de habilitar `en.wikipedia.org`, `commons.wikimedia.org` y `upload.wikimedia.org` en la configuración de red:

```sh
python3 -m pip install CairoSVG==2.8.2
python3 tools/fetch_wikimedia.py --only flag_es  # comprobar primero acceso y conversión
python3 tools/fetch_wikimedia.py                 # catálogo completo
python3 tools/test_wikimedia.py
```

Revisa los resultados y sus atribuciones antes de distribuir el juego. Algunas páginas pueden no disponer de una ilustración libre o de un emblema adecuado; el importador informa esos casos y termina con error, sin inventar imágenes. Vuelve a compilar para copiar el contenido nuevo junto al ejecutable.

`tools/gen_world.py` regenera los datos e invoca la expansión de `tools/expand_content.py`. La expansión conserva los IDs existentes y es idempotente. No ejecutes la generación durante la preparación del entorno: modifica los datos del juego.

## Entorno de nube

Herramientas y artefactos locales: `/workspace/.solaris-env/`. El script de instalación y las instrucciones de inicio se guardan en el borrador del entorno. La publicación de la configuración es un paso separado en la interfaz del producto. Usa el checkout existente; cada tarea ya tiene un entorno aislado.

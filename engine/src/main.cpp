// Solaris History - punto de entrada (Win32 + OpenGL)
#include "solaris/game.hpp"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    sol::Platform platform;
    if (!platform.init(hInst, 1600, 900, L"Solaris History")) {
        MessageBoxW(nullptr, L"No se pudo crear la ventana o el contexto OpenGL.",
                    L"Solaris History", MB_ICONERROR);
        return 1;
    }

    sol::Game game(platform);
    std::string contentDir = sol::findContentDir();
    if (!game.init(contentDir)) {
        platform.shutdown();
        MessageBoxW(nullptr,
                    L"No se encontro la carpeta de contenido.\n\n"
                    L"Coloca 'content' junto al ejecutable o ejecuta desde la raiz del proyecto.",
                    L"Solaris History", MB_ICONERROR);
        return 1;
    }

    double last = platform.now();
    while (platform.running) {
        platform.pump();

        double now = platform.now();
        double dt = now - last;
        last = now;
        if (dt < 0) dt = 0;
        if (dt > 0.1) dt = 0.1; // evita saltos tras perder el foco

        game.update(dt);
        game.render();
        platform.swap();
    }

    platform.shutdown();
    return 0;
}

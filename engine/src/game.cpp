#include "solaris/game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>

namespace sol {

static void trace(const char* fmt, ...) {
    FILE* f = std::fopen("solaris_trace.log", "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(f, fmt, ap);
    va_end(ap);
    std::fprintf(f, "\n");
    std::fclose(f);
}

static std::string shortDate(int year, int month) {
    std::string m = Locale::text(monthName(month));
    if (m.size() > 3) m = m.substr(0, 3);
    return m + " " + std::to_string(year < 0 ? -year : year);
}

static std::string ellipsize(const Font& f, const std::string& s, float scale, float maxW) {
    std::string out = Locale::text(s);
    if (f.width(out, scale) <= maxW) return out;
    while (!out.empty() && f.width(out + "...", scale) > maxW) {
        size_t pos=out.size()-1;
        while(pos>0 && ((unsigned char)out[pos]&0xc0)==0x80) --pos;
        out.erase(pos);
    }
    return out + "...";
}

static std::string unitCosts(const UnitDef& u) {
    std::string s;
    if (u.gold > 0) s += numStr(u.gold, 0) + " oro";
    if (u.food > 0) s += (s.empty() ? "" : " · ") + numStr(u.food, 0) + " alim";
    if (u.wood > 0) s += (s.empty() ? "" : " · ") + numStr(u.wood, 0) + " madera";
    if (u.stone > 0) s += (s.empty() ? "" : " · ") + numStr(u.stone, 0) + " piedra";
    return s;
}
static std::string buildCosts(const BuildingDef& b) {
    return numStr(b.gold, 0) + " oro · " + numStr(b.wood, 0) + " madera · " + numStr(b.stone, 0) + " piedra";
}

// ---------------------------------------------------------------------------
bool Game::init(const std::string& contentDir) {
    if (!C.load(contentDir)) return false;
    if (!G.init(C, contentDir)) {
        MessageBoxA(nullptr, "No se pudieron cargar los graficos.\nComprueba la carpeta content/textures.",
                    "Solaris History", MB_ICONERROR);
        return false;
    }
    if (!Locale::load(contentDir + "/locales.json")) return false;
    Locale::english = readTextFile("solaris_options.cfg").find("en") == 0;
    sim.init(C, 0);
    screen = Screen::Menu;
    for (int i = 0; i < (int)C.cityStates.size(); ++i) {
        if (C.cityStates[(size_t)i].playable) { menuSelection = i; break; }
    }
    G.cam.yaw = 0.32f;
    G.cam.pitch = 0.28f;
    G.cam.dist = 3.05f;
    return true;
}

int Game::armyIndexById(int id) const {
    for (int i = 0; i < (int)sim.armies.size(); ++i)
        if (sim.armies[(size_t)i].id == id) return i;
    return -1;
}

bool Game::isPlayerArmy(int id) const {
    int idx = armyIndexById(id);
    return idx >= 0 && sim.armies[(size_t)idx].nation == sim.player;
}

void Game::setStatus(const std::string& s) {
    status = s;
    statusTimer = 5.0f;
}

void Game::openModal(Modal m) {
    modal = m;
    modalScroll = 0;
    speed = 0;
}

void Game::startNewGame() {
    trace("startNewGame begin firstPlay=%d menuSelection=%d", (int)firstPlay, menuSelection);
    sim.init(C, menuSelection);
    screen = Screen::Play;
    speed = 0;
    selectedProvince = sim.nations[(size_t)sim.player].capital;
    selectedArmyId = -1;
    modal = Modal::None;
    statusTimer = 0;
    tickTimer = 0;
    if (sim.validProvince(selectedProvince)) {
        const ProvinceDef& pd = sim.pdef(selectedProvince);
        Vec3 d = latLonToDir(pd.lat, pd.lon);
        G.cam.yaw = std::atan2(d.x, d.z);
        G.cam.pitch = clampf(std::asin(clampf(d.y, -1, 1)) * 0.9f, -0.1f, 0.9f);
        G.cam.dist = 2.45f;
    }
    if (firstPlay) {
        firstPlay = false;
        openModal(Modal::Help);
    }
    trace("startNewGame end modal=%d speed=%d firstPlay=%d", (int)modal, speed, (int)firstPlay);
}

// ---------------------------------------------------------------------------
// entrada
// ---------------------------------------------------------------------------
int Game::pickProvince(float mx, float my) const {
    Vec3 o = G.cam.pos();
    Vec3 d = G.rayDirection(mx, my, P.w, P.h);
    float b = dot(o, d);
    float c = dot(o, o) - 1.0f;
    float disc = b * b - c;
    if (disc < 0) return -1;
    float t = -b - std::sqrt(disc);
    if (t <= 0) return -1;
    Vec3 hit = o + d * t;
    float lat, lon;
    dirToLatLon(normalize(hit), lat, lon);
    return G.map.pick(lat, lon);
}

void Game::handleKeys() {
    bool esc = P.keyPressed[VK_ESCAPE];
    if (esc) {
        if (modal != Modal::None && modal != Modal::Elections && modal != Modal::GameOver) {
            modal = Modal::None;
        } else if (selectedArmyId >= 0) {
            selectedArmyId = -1;
        } else {
            selectedProvince = -1;
        }
    }
    if (P.keyPressed[VK_SPACE]) {
        if (speed == 0) speed = 2; else speed = 0;
    }
    if (P.keyPressed['1']) speed = 1;
    if (P.keyPressed['2']) speed = 2;
    if (P.keyPressed['3']) speed = 3;
    if (P.keyPressed['4']) speed = 4;
    if (P.keyPressed[VK_F1]) openModal(Modal::Help);
    if (P.keyPressed[VK_F5]) {
        sim.save("solaris_quicksave.sav");
        setStatus("Partida guardada en solaris_quicksave.sav");
    }
    if (P.keyPressed[VK_F9]) loadGame();
}

void Game::handlePlayInput() {
    int pid = pickProvince(P.mx, P.my);
    bool onUI = ui.overUI(P.mx, P.my) || modal != Modal::None;
    hoverProvince = (!onUI && pid >= 0) ? pid : -1;

    if (P.mousePressed[1]) {
        if (modal != Modal::None && modal != Modal::Elections && modal != Modal::GameOver) modal = Modal::None;
        else if (selectedArmyId >= 0) selectedArmyId = -1;
        else selectedProvince = -1;
    }

    if (P.mousePressed[0] && !onUI) {
        cameraDrag = true;
        dragStartX = dragStartY = 0;
        dragMoved = 0;
        lastMx = P.mx;
        lastMy = P.my;
    }
    if (cameraDrag && P.mouseDown[0]) {
        float dx = P.mx - lastMx, dy = P.my - lastMy;
        if (std::fabs(dx) + std::fabs(dy) > 0.5f) {
            G.cam.orbit(dx, dy);
            dragMoved += std::fabs(dx) + std::fabs(dy);
        }
        lastMx = P.mx;
        lastMy = P.my;
    }
    if (cameraDrag && P.mouseReleased[0]) {
        if (dragMoved < 7.0f) {
            if (pid >= 0) {
                if (selectedArmyId >= 0) {
                    int ai = armyIndexById(selectedArmyId);
                    if (ai >= 0 && sim.armies[(size_t)ai].nation == sim.player) {
                        std::vector<int> path;
                        float cost = 0;
                        if (sim.findPath(ai, pid, path, cost) && path.size() >= 2) {
                            sim.orderMove(ai, pid);
                            setStatus("Marcha hacia " + sim.provName(pid) + " (" + numStr(std::max(1.0f, cost), 1) + " meses)");
                            selectedArmyId = -1;
                        } else {
                            setStatus("No hay ruta practicable hasta " + sim.provName(pid) + ".");
                        }
                    } else {
                        selectedArmyId = -1;
                    }
                } else {
                    selectedProvince = pid;
                }
            } else if (selectedArmyId < 0) {
                selectedProvince = -1;
            }
        }
        cameraDrag = false;
    }
    if (std::fabs(P.wheel) > 0.01f && !onUI) G.cam.zoom(P.wheel);
    if (selectedArmyId >= 0 && armyIndexById(selectedArmyId) < 0) selectedArmyId = -1;
}

void Game::update(double dt) {
    trace("update screen=%d modal=%d speed=%d firstPlay=%d mouse=%.0f,%.0f pressed=%d", (int)screen, (int)modal, speed, (int)firstPlay, P.mx, P.my, (int)P.mousePressed[0]);
    dt = std::min(dt, 0.1);
    time += (float)dt;
    if (statusTimer > 0) statusTimer = std::max(0.0f, statusTimer - (float)dt);

    if (screen == Screen::Menu) {
        G.cam.yaw += (float)dt * 0.035f;
        G.cam.pitch = 0.26f + std::sin(time * 0.13f) * 0.05f;
        return;
    }

    handleKeys();
    handlePlayInput();

    if (modal == Modal::None && speed > 0 && !sim.gameOver) {
        static const float intervals[5] = {0.0f, 1.10f, 0.55f, 0.26f, 0.12f};
        tickTimer += dt;
        int guard = 0;
        while (tickTimer >= intervals[std::clamp(speed, 0, 4)] && guard++ < 30) {
            tickTimer -= intervals[std::clamp(speed, 0, 4)];
            sim.step();
            if (sim.alertPending) {
                eventTitle = sim.alertTitle;
                eventText = sim.alertText;
                eventKind = sim.alertKind;
                sim.alertPending = false;
                modal = Modal::Event;
                speed = 0;
            }
        }
    }

    // elecciones pendientes
    if ((modal == Modal::None) && sim.nationAlive(sim.player) && !sim.nations[(size_t)sim.player].isCityState) {
        const Nation& n = sim.nations[(size_t)sim.player];
        const GovDef& g = C.gov(n.gov);
        if (g.election > 0) {
            int ruler = sim.livingRuler(sim.player);
            bool due = (n.electionAt > 0 && sim.abs() >= n.electionAt) || ruler < 0;
            if (due) {
                modal = Modal::Elections;
                speed = 0;
            }
        }
    }

    if ((sim.gameOver || !sim.nations[(size_t)sim.player].alive) && modal != Modal::GameOver) {
        modal = Modal::GameOver;
        speed = 0;
    }
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------
void Game::render() {
    if (screen == Screen::Menu) {
        G.map.sync(sim, -1);
        G.drawGlobe(sim, -1, time, P.w, P.h);
        ui.begin(P, G);
        drawMenu();
        ui.end();
        return;
    }

    G.map.sync(sim, selectedProvince);
    G.drawGlobe(sim, selectedProvince, time, P.w, P.h);
    ui.begin(P, G);
    bool modalClick = ui.clicked;
    if (modal != Modal::None) { ui.clicked = false; P.mousePressed[0] = false; }
    drawMarkers();
    drawTopBar();
    drawProvincePanel();
    drawNationPanel();
    drawLog();
    ui.clicked = modalClick; P.mousePressed[0] = modalClick;

    if (selectedArmyId >= 0) {
        std::string hint = "Elige la provincia de destino  ·  Esc para cancelar";
        float tw = ui.textW(hint, 0.34f) + 26;
        ui.rect({P.w * 0.5f - tw * 0.5f, 64, tw, 28}, Color(18, 16, 12, 235));
        ui.outline({P.w * 0.5f - tw * 0.5f, 64, tw, 28}, pal::border, 1);
        ui.textC(hint, P.w * 0.5f, 68, 0.34f, pal::gold);
    }

    if (statusTimer > 0 && !status.empty()) {
        float tw = ui.textW(status, 0.36f) + 30;
        float x = P.w * 0.5f - tw * 0.5f;
        float y = P.h - 182;
        ui.rect({x + 2, y + 3, tw, 32}, Color(0, 0, 0, 100));
        ui.rect({x, y, tw, 32}, Color(20, 19, 16, 240));
        ui.outline({x, y, tw, 32}, pal::goldDim, 1);
        ui.textC(status, P.w * 0.5f, y + 5, 0.36f, pal::text);
    }

    if (modal != Modal::None) drawModal();

    if (hoverProvince >= 0 && modal == Modal::None && !ui.overUI(P.mx, P.my)) {
        const ProvinceDef& pd = sim.pdef(hoverProvince);
        const Province& pv = sim.prov[(size_t)hoverProvince];
        std::string owner = pv.owner > 0 ? sim.nationName(pv.owner) : "Independiente";
        ui.tooltip = pd.name + "  ·  " + owner + "  ·  " + C.culture(pd.cultureId).shortName;
    }
    ui.end();
}

// ---------------------------------------------------------------------------
// menu
// ---------------------------------------------------------------------------
void Game::drawMenu() {
    float h = (float)P.h;
    ui.rect({0, 0, 800, h}, Color(8, 10, 15, 208));
    ui.rect({798, 0, 1, h}, Color(176, 144, 78, 60));
    ui.text("SOLARIS", 54, 48, 1.24f, pal::gold, true);
    ui.text("HISTORY", 54, 122, 0.86f, Color(222, 214, 194), true);
    ui.text("GRAN ESTRATEGIA EN TIEMPO REAL  ·  3300 a.C.", 58, 188, 0.34f, pal::dim);
    ui.divider(56, 222, 660, pal::border);

    if (menuPage == 0) {
        const char* entries[] = {"Crear partida", "Cargar partida", "Opciones", "Creditos", "Salir"};
        for (int i = 0; i < 5; ++i) {
            if (!ui.button(entries[i], {56, 270.0f + i * 66, 450, 50}, true, pal::gold, 0.40f)) continue;
            if (i == 0) menuPage = 1;
            else if (i == 1) loadGame();
            else if (i == 2) menuPage = 2;
            else if (i == 3) menuPage = 3;
            else P.running = false;
        }
        if (statusTimer > 0) ui.textBlock(status, 56, 620, 650, 0.30f, pal::bad);
        return;
    }
    if (ui.button("Volver", {56, h - 68, 160, 38})) { menuPage = 0; return; }
    if (menuPage == 2) {
        ui.text("Idioma / Language", 56, 270, 0.46f, pal::gold);
        drawLanguageFlag({56, 320, 64, 40}, false);
        drawLanguageFlag({56, 385, 64, 40}, true);
        bool change = false;
        if (ui.button("Español", {140, 315, 240, 50})) { Locale::english = false; change = true; }
        if (ui.button("English", {140, 380, 240, 50})) { Locale::english = true; change = true; }
        if (change) {
            FILE* f = std::fopen("solaris_options.cfg", "w");
            if (f) { std::fputs(Locale::english ? "en\n" : "es\n", f); std::fclose(f); }
        }
        ui.text(Locale::english ? "English" : "Español", 56, 465, 0.36f, pal::good);
        return;
    }
    if (menuPage == 3) {
        ui.text("Creditos", 56, 250, 0.50f, pal::gold);
        ui.text("Solaris History - Braiman1 · NASA Blue Marble · Wikimedia Commons", 56, 288, 0.28f, pal::text);
        ui.text("Fuentes, autores y licencias de las imagenes", 56, 322, 0.30f, pal::goldDim);
        const int visible=std::max(1,std::min(5,(P.h-450)/78));
        if(ui.wheel) creditsScroll-=(int)ui.wheel;
        creditsScroll=std::clamp(creditsScroll,0,std::max(0,(int)G.mediaCredits.size()-visible));
        auto credit=G.mediaCredits.begin();
        std::advance(credit,creditsScroll);
        for(int k=0;k<visible && credit!=G.mediaCredits.end();++k,++credit) {
            float y=362.0f+k*78;
            drawMedia(credit->first,{56,y,64,64});
            ui.text(ellipsize(G.font,credit->second.title,0.28f,560),132,y,0.28f,pal::text);
            ui.text(ellipsize(G.font,credit->second.author,0.25f,560),132,y+22,0.25f,pal::dim);
            ui.text(credit->second.license,132,y+44,0.25f,pal::good);
        }
        if(ui.smallButton("Subir",{530,h-68,85,32},creditsScroll>0)) --creditsScroll;
        if(ui.smallButton("Bajar",{628,h-68,85,32},creditsScroll+visible<(int)G.mediaCredits.size())) ++creditsScroll;
        ui.text("Los simbolos no son banderas de 3300 a.C. Consulta su contexto al pasar el raton.",56,h-105,0.26f,pal::dim);
        return;
    }
    ui.text("ELIGE TU GRUPO", 56, 240, 0.40f, pal::gold);
    std::vector<int> playable;
    for (int i = 0; i < (int)C.cityStates.size(); ++i) if (C.cityStates[i].playable) playable.push_back(i);
    const int visible = std::max(1, std::min(5, (P.h - 490) / 63));
    if (ui.wheel != 0) menuScroll -= (int)ui.wheel;
    menuScroll = std::clamp(menuScroll, 0, std::max(0, (int)playable.size() - visible));
    for (int k = 0; k < visible && menuScroll + k < (int)playable.size(); ++k) {
        int i = playable[menuScroll + k];
        const CityStateDef& cs = C.cityStates[i];
        Rect row(56, 280.0f + k * 63, 664, 55);
        if (ui.button(cs.name+" · "+C.province(cs.capital).name, row, true, i == menuSelection ? pal::gold : pal::dim, 0.36f)) menuSelection = i;
        drawMedia("culture_"+cs.culture, {row.x+8,row.y+5,44,44});
    }
    if (ui.smallButton("Subir", {560, 238, 76, 30}, menuScroll > 0)) --menuScroll;
    if (ui.smallButton("Bajar", {644, 238, 76, 30}, menuScroll + visible < (int)playable.size())) ++menuScroll;
    int visibleIdx = visible;
    const CityStateDef& cs = C.cityStates[(size_t)menuSelection];
    const CultureDef& cu = C.culture(cs.culture);
    float dy = 280 + visibleIdx * 63 + 10;
    ui.divider(56, dy, 664, pal::border);
    ui.text(cu.name, 56, dy + 12, 0.38f, pal::gold);
    ui.textBlock(cs.desc, 56, dy + 36, 660, 0.32f, pal::text, 2, 3);
    std::string mods = "Ventajas: ";
    if (cu.farm > 1.05f) mods += "agricultura ";
    if (cu.trade > 1.05f) mods += "comercio ";
    if (cu.mil > 1.05f) mods += "militar ";
    if (cu.tech > 1.05f) mods += "tecnologia ";
    if (cu.naval > 1.05f) mods += "naval ";
    if (cu.stab > 1.05f) mods += "estabilidad ";
    if (mods == "Ventajas: ") mods += "equilibrio";
    ui.text(mods, 56, dy + 100, 0.28f, pal::dim);

    Rect startR(56, h - 104, 300, 46);
    if (ui.button("COMENZAR PARTIDA", startR, true, pal::gold, 0.44f)) startNewGame();
    Rect helpR(370, h - 104, 120, 46);
    if (ui.button("Ayuda", helpR, true, pal::goldDim, 0.36f)) openModal(Modal::Help);

    ui.text("Globo: NASA Blue Marble", 56, h - 22, 0.24f, Color(100, 100, 100));

    if (modal != Modal::None) drawModal();
}

// ---------------------------------------------------------------------------
// barra superior
// ---------------------------------------------------------------------------
void Game::drawTopBar() {
    float w = (float)P.w;
    ui.rect({0, 0, w, 54}, Color(12, 14, 19, 242));
    ui.rect({0, 53, w, 1}, Color(176, 144, 78, 110));
    ui.text("SOLARIS HISTORY", 18, 5, 0.44f, pal::gold);
    const Nation& n = sim.nations[(size_t)sim.player];

    ui.text(Locale::english ? Locale::text(monthName(sim.month)) + " " + yearLabel(sim.year) : dateLabel(sim.year, sim.month), 224, 4, 0.42f, pal::text);
    ui.text(C.era(n.era).name + "  ·  " + C.gov(n.gov).name, 224, 31, 0.26f, pal::dim);

    if (ui.smallButton("Menu", {18, 31, 100, 20})) {
        speed = 0; screen = Screen::Menu; menuPage = 0;
    }
    // velocidad
    const char* labels[5] = {"II", ">", ">>", ">>>", "x4"};
    for (int i = 0; i < 5; ++i) {
        Rect r(430 + i * 38, 11, 34, 31);
        bool active = (speed == i);
        if (active) {
            ui.rect(r, Color(60, 49, 30, 250));
            ui.outline(r, pal::gold, 1);
        } else {
            ui.rect(r, ui.hover(r) ? Color(38, 37, 33, 240) : Color(25, 27, 31, 240));
            ui.outline(r, Color(80, 74, 62, 120), 1);
        }
        ui.textC(labels[i], r.x + r.w * 0.5f, r.y + 3, 0.30f, active ? pal::gold : pal::dim);
        if (ui.hover(r)) {
            P.setCursorHand(true);
            ui.tooltip = i == 0 ? "Pausa" : ("Velocidad " + std::to_string(i));
        }
        if (ui.clicked && ui.hover(r)) speed = i;
        if (ui.clicked && ui.hover(r)) trace("speed button click i=%d mouse=%.0f,%.0f speednow=%d", i, ui.mx, ui.my, speed);
        ui.addHit(r);
    }

    if (ui.smallButton("Tecnologias", {842, 14, 108, 28})) openModal(Modal::Technology);
    if (ui.smallButton("Recursos", {958, 14, 92, 28})) openModal(Modal::Resources);
    // tecnologia
    float tech = n.tech;
    float next = (n.era + 1 < (int)C.eras.size()) ? C.eras[(size_t)n.era + 1].tech : tech;
    float tp = next > 0 ? clampf(tech / next, 0, 1) : 1;
    ui.text("Tecnologia", 632, 6, 0.26f, pal::dim);
    ui.bar({632, 27, 130, 9}, tp, pal::blue);
    ui.text(numStr(tech, 0) + " / " + numStr(next, 0), 768, 9, 0.24f, pal::faint);

    // recursos
    struct Res { const char* label; double value; Color c; };
    Res res[6] = {
        {"Oro", n.gold, Color(226, 190, 90)},
        {"Alimento", n.food, Color(164, 190, 112)},
        {"Madera", n.wood, Color(132, 158, 100)},
        {"Piedra", n.stone, Color(168, 166, 158)},
        {"Cobre", n.copper, Color(198, 126, 76)},
        {"Ingreso", n.lastIncome, Color(120, 190, 170)}
    };
    float x = w - 12;
    for (int i = 5; i >= 0; --i) {
        float iw = 88;
        x -= iw;
        ui.rect({x + 6, 17, 8, 8}, res[i].c);
        std::string v = (i == 5) ? (res[i].value >= 0 ? "+" : "") + numStr(res[i].value, 1)
                                : numStr(res[i].value, 0);
        ui.text(v, x + 20, 6, 0.34f, i == 5 ? (res[i].value >= 0 ? pal::good : pal::bad) : pal::text);
        ui.text(res[i].label, x + 20, 31, 0.23f, pal::faint);
        if (ui.hover({x, 0, iw, 54})) {
            if (i == 0) ui.tooltip = "Tesoro: oro disponible";
            else if (i == 1) ui.tooltip = "Alimento almacenado";
            else if (i == 5) ui.tooltip = "Ingreso neto de oro por mes";
            else ui.tooltip = "Material almacenado";
        }
    }
}

// ---------------------------------------------------------------------------
// panel de provincia
// ---------------------------------------------------------------------------
void Game::drawProvincePanel() {
    Rect r(14, 66, 306, 498);
    ui.panel(r);
    float x = r.x + 16;
    float wd = r.w - 32;

    if (selectedProvince < 0 || !sim.validProvince(selectedProvince)) {
        ui.text("EL MUNDO", x, r.y + 12, 0.28f, pal::gold);
        ui.text("Tierra conocida", x, r.y + 38, 0.48f, pal::text);
        ui.divider(x, r.y + 68, wd);
        int nations = 0, owned = 0, indep = 0;
        for (auto& n : sim.nations) if (n.alive) ++nations;
        for (auto& p : sim.prov) { if (p.owner > 0) ++owned; else ++indep; }
        ui.text("Naciones vivas", x, r.y + 82, 0.30f, pal::dim);
        ui.textR(std::to_string(nations - 1), x + wd, r.y + 82, 0.32f, pal::text);
        ui.text("Provincias pobladas", x, r.y + 106, 0.30f, pal::dim);
        ui.textR(std::to_string(sim.prov.size()), x + wd, r.y + 106, 0.32f, pal::text);
        ui.text("Territorios libres", x, r.y + 130, 0.30f, pal::dim);
        ui.textR(std::to_string(indep), x + wd, r.y + 130, 0.32f, pal::text);
        ui.divider(x, r.y + 158, wd);
        ui.text("COMO JUGAR", x, r.y + 172, 0.27f, pal::gold);
        const char* tips[] = {
            "Arrastra el globo para girarlo.",
            "Rueda del raton para acercarte.",
            "Haz clic en una provincia para verla.",
            "Coloniza territorios vecinos y funda tu nacion.",
            "Recluta ejercitos y conquista o defiende.",
            "Los lideres envejecen y mueren para siempre.",
            "Republicas: elecciones y reeleccion.",
            "F5 guarda, F9 carga, Espacio pausa."
        };
        for (int i = 0; i < 8; ++i)
            ui.textBlock(tips[i], x, r.y + 200 + i * 34, wd, 0.285f, pal::dim, 1, 2);
        return;
    }

    const ProvinceDef& pd = sim.pdef(selectedProvince);
    Province& pv = sim.prov[(size_t)selectedProvince];
    const TerrainDef& terr = C.terrainOfProvince(selectedProvince);
    const ResourceDef& res = C.resourceOfProvince(selectedProvince);
    drawMedia("resource_"+res.id, {r.x+r.w-54,r.y+10,40,40});
    const CultureDef& cu = C.culture(pd.cultureId);
    bool mine = pv.owner == sim.player;

    ui.text("PROVINCIA", x, r.y + 10, 0.26f, pal::faint);
    ui.text(ellipsize(G.font, pd.name, 0.56f, wd), x, r.y + 30, 0.56f, pal::text);
    ui.text(pd.region + "  ·  " + terr.name, x, r.y + 70, 0.27f, pal::dim);
    ui.divider(x, r.y + 96, wd);

    Color oc = pv.owner > 0 ? sim.nationColor(pv.owner) : cu.color;
    ui.rect({x, r.y + 110, 9, 9}, oc);
    std::string owner = pv.owner > 0 ? sim.nationName(pv.owner) : "Pueblos independientes";
    ui.text(ellipsize(G.font, owner, 0.34f, wd - 20), x + 16, r.y + 103, 0.34f, pv.owner == sim.player ? pal::gold : pal::text);
    ui.text("Cultura: " + cu.shortName, x, r.y + 132, 0.28f, pal::dim);
    ui.text("Recurso: " + res.name, x, r.y + 154, 0.28f, pal::dim);

    ui.text("Poblacion", x, r.y + 182, 0.26f, pal::faint);
    ui.text(numStr(pv.pop, 1) + "k", x, r.y + 200, 0.40f, pal::text);
    ui.text("Desarrollo", x + 118, r.y + 182, 0.26f, pal::faint);
    ui.text(std::to_string(pd.dev), x + 118, r.y + 200, 0.40f, pal::text);
    ui.text("Lealtad", x + 214, r.y + 182, 0.26f, pal::faint);
    ui.text(numStr(pv.loyalty, 0) + "%", x + 214, r.y + 200, 0.40f, pv.loyalty > 60 ? pal::good : (pv.loyalty < 30 ? pal::bad : pal::war));

    ui.bar({x, r.y + 236, wd, 7}, pv.loyalty / 100.0f, pv.loyalty > 60 ? pal::good : (pv.loyalty < 30 ? pal::bad : pal::war));

    ui.text("EDIFICIOS", x, r.y + 256, 0.26f, pal::faint);
    std::string bld;
    for (auto& b : pv.buildings) {
        int bi = C.findBuilding(b);
        if (bi >= 0) bld += (bld.empty() ? "" : ", ") + C.building(bi).name;
    }
    if (bld.empty()) bld = "Ninguno";
    ui.textBlock(bld, x, r.y + 274, wd, 0.27f, pal::dim, 1, 2);

    ui.text("EJERCITOS", x, r.y + 318, 0.26f, pal::faint);
    int shown = 0;
    for (auto& a : sim.armies) {
        if (a.prov != selectedProvince || a.inTransit()) continue;
        if (shown >= 2) break;
        Rect row(x, r.y + 336 + shown * 38, wd, 34);
        ui.rect(row, Color(16, 18, 21, 180));
        ui.outline(row, sim.nationColor(a.nation).withAlpha(100), 1);
        ui.icon(G.iconFor(a.unit), {row.x + 4, row.y + 3, 28, 28});
        std::string un = a.unit >= 0 ? sim.unitName(a.nation,a.unit) : "?";
        ui.text(un, row.x + 38, row.y + 1, 0.28f, pal::text);
        ui.text(numStr(a.men, 0) + " hombres", row.x + 38, row.y + 17, 0.23f, pal::dim);
        bool selected = (selectedArmyId == a.id);
        if (ui.smallButton(selected ? "Elegido" : "Mover", {row.x + row.w - 66, row.y + 4, 60, 25}, a.nation == sim.player, pal::goldDim)) {
            if (a.nation == sim.player) selectedArmyId = selected ? -1 : a.id;
        }
        ++shown;
    }
    if (shown == 0) ui.text("Sin guarnicion", x, r.y + 338, 0.28f, pal::faint);
    if (shown == 2) {
        int total = 0;
        for (auto& a : sim.armies) if (a.prov == selectedProvince && !a.inTransit()) ++total;
        if (total > 2) ui.text("y " + std::to_string(total - 2) + " mas...", x, r.y + 414, 0.24f, pal::faint);
    }

    float by = r.y + r.h - 84;
    bool canBuildAny = mine;
    if (ui.button("Construir", {x, by, (wd - 8) / 2, 32}, canBuildAny, pal::goldDim, 0.30f)) openModal(Modal::Buildings);
    if (ui.button("Reclutar", {x + (wd - 8) / 2 + 8, by, (wd - 8) / 2, 32}, canBuildAny, pal::goldDim, 0.30f)) openModal(Modal::Recruit);

    if (!mine) {
        float cost = 0;

        const Province& colony = sim.province(selectedProvince);
        if (colony.colonizer) ui.text("Colonizacion: " + std::to_string(colony.colonyMonths) + " meses", x, by - 24, 0.28f, pal::gold);
        std::string label = "Colonizar: 36 meses (" + numStr(cost, 0) + " oro)";
        bool canAfford = sim.canColonize(sim.player, selectedProvince, &cost);
        if (ui.button(label, {x, by + 38, wd, 30}, canAfford, pal::good, 0.28f)) {
            if (sim.colonize(sim.player, selectedProvince)) setStatus("Expedicion enviada a " + pd.name + ".");
        }
        if (ui.hover({x, by + 38, wd, 30}) && !canAfford) ui.tooltip = "Necesitas una provincia vecina, oro y ninguna expedicion activa.";
    } else {
        if (ui.button("Diplomacia", {x, by + 38, wd, 28}, true, pal::goldDim, 0.28f)) openModal(Modal::Diplomacy);
    }
}

// ---------------------------------------------------------------------------
// panel de nacion
// ---------------------------------------------------------------------------
void Game::drawNationPanel() {
    float pw = 320, ph = 604;
    Rect r(P.w - pw - 14, 66, pw, ph);
    ui.panel(r);
    if (sim.player <= 0 || sim.player >= (int)sim.nations.size()) return;
    Nation& n = sim.nations[(size_t)sim.player];
    float x = r.x + 16, wd = r.w - 32;

    ui.rect({r.x, r.y, r.w, 4}, n.color);
    if(ui.smallButton("Gestion nacional",{r.x+16,r.y+85,170,20}))openModal(Modal::NationDetails);
    ui.text("TU NACION", x, r.y + 12, 0.24f, pal::faint);
    drawMedia(G.nationMedia(n,C),{r.x+r.w-78,r.y+12,62,62});
    ui.text(ellipsize(G.font, n.name, 0.52f, wd - 70), x, r.y + 30, 0.52f, pal::text);
    std::string sub = (n.isCityState ? "Ciudad-estado" : C.gov(n.gov).name) + std::string("  ·  ") + C.era(n.era).shortName;
    ui.text(sub, x, r.y + 66, 0.28f, pal::dim);
    ui.divider(x, r.y + 88, wd);

    int ruler = sim.livingRuler(sim.player);
    if (ruler >= 0) {
        Character& c = sim.chars[(size_t)ruler];
        ui.portrait({x, r.y + 100, 64, 64}, c, n.color, true);
        ui.text(ellipsize(G.font, c.name, 0.40f, 190), x + 76, r.y + 98, 0.40f, pal::gold);
        ui.text("Edad " + std::to_string(c.age(sim.abs())) + " anos", x + 76, r.y + 124, 0.26f, pal::dim);
        const TraitDef& t1 = C.trait(c.traitA);
        const TraitDef& t2 = C.trait(c.traitB);
        ui.chip(t1.name, x + 76, r.y + 142, Color(150, 176, 200), 0.22f);
        ui.chip(t2.name, x + 76 + ui.textW(t1.name, 0.22f) + 22, r.y + 142, Color(200, 170, 120), 0.22f);
        float health = clampf(1.1f - (c.age(sim.abs()) - 30) / 60.0f, 0.08f, 1.0f);
        ui.bar({x, r.y + 172, 186, 6}, health, health > 0.55f ? pal::good : (health > 0.3f ? pal::war : pal::bad));
    } else {
        ui.text("Sin gobernante", x, r.y + 104, 0.38f, pal::bad);
        ui.text("El consejo gobierna en interinidad.", x, r.y + 132, 0.26f, pal::dim);
    }

    ui.text("CONSEJO", x, r.y + 194, 0.25f, pal::faint);
    const char* officeNames[3] = {"Canciller", "General", "Sacerdote"};
    float cy = r.y + 212;
    for (int office = 0; office < 3; ++office) {
        int cid = -1;
        for (int id : n.charIds) {
            const Character& c = sim.chars[(size_t)id];
            if (c.alive && c.role == 1 && c.office == office) { cid = id; break; }
        }
        Rect row(x, cy, wd, 26);
        ui.rect(row, Color(16, 18, 22, 120));
        ui.rect({x, cy + 3, 3, 20}, n.color.withAlpha(180));
        ui.text(officeNames[office], x + 10, cy + 2, 0.25f, pal::dim);
        if (cid >= 0) {
            const Character& c = sim.chars[(size_t)cid];
            ui.text(ellipsize(G.font, c.name, 0.29f, 150), x + 84, cy + 2, 0.29f, pal::text);
            ui.textR(std::to_string(c.age(sim.abs())) + " anos", x + wd - 8, cy + 4, 0.24f, pal::faint);
        } else {
            ui.text("vacante", x + 84, cy + 3, 0.26f, pal::bad);
        }
        cy += 30;
    }
    ui.divider(x, r.y + 308, wd);

    ui.text("Estabilidad", x, r.y + 320, 0.26f, pal::dim);
    ui.textR(numStr(n.stability, 0), x + wd, r.y + 320, 0.26f, pal::text);
    ui.bar({x, r.y + 340, wd, 6}, n.stability / 100.0f, n.stability > 55 ? pal::good : (n.stability > 30 ? pal::war : pal::bad));
    ui.text("Legitimidad", x, r.y + 356, 0.26f, pal::dim);
    ui.textR(numStr(n.legitimacy, 0), x + wd, r.y + 356, 0.26f, pal::text);
    ui.bar({x, r.y + 376, wd, 6}, n.legitimacy / 100.0f, pal::blue);

    int provCount = 0;
    for (auto& p : sim.prov) if (p.owner == sim.player) ++provCount;
    ui.text("Provincias", x, r.y + 392, 0.27f, pal::dim);
    ui.textR(std::to_string(provCount), x + wd, r.y + 392, 0.27f, pal::text);
    ui.text("Manode obra", x, r.y + 414, 0.27f, pal::dim);
    ui.textR(numStr(n.manpower, 1) + "k", x + wd, r.y + 414, 0.27f, pal::text);
    ui.text("Ejercito", x, r.y + 436, 0.27f, pal::dim);
    ui.textR(numStr(n.armyMen, 1) + "k hombres", x + wd, r.y + 436, 0.27f, pal::text);
    ui.text("Prestigio", x, r.y + 458, 0.27f, pal::dim);
    ui.textR(numStr(n.prestige, 0), x + wd, r.y + 458, 0.27f, pal::gold);

    if (!n.wars.empty()) {
        std::string wars = "Guerra: ";
        for (size_t i = 0; i < n.wars.size() && i < 3; ++i)
            wars += (i ? ", " : "") + sim.nationName(n.wars[i]);
        ui.text(ellipsize(G.font, wars, 0.26f, wd), x, r.y + 478, 0.26f, pal::war);
    } else {
        ui.text("En paz", x, r.y + 478, 0.26f, pal::faint);
    }

    float by = r.y + 500;
    if (n.isCityState) {
        Rect b(x, by, wd, 36);
        bool can = sim.canFound(sim.player);
        if (ui.button("FUNDAR NACION", b, can, pal::gold, 0.40f)) openModal(Modal::FoundNation);
        if (!can && ui.hover(b)) ui.tooltip = "Necesitas 2 provincias, 60 oro, 5 de prestigio y 3 puntos tecnologicos.";
        by += 42;
    }
    if (ui.button("Diplomacia", {x, by, (wd - 8) / 2, 30}, true, pal::goldDim, 0.29f)) openModal(Modal::Diplomacy);
    if (ui.button("Cronica", {x + (wd - 8) / 2 + 8, by, (wd - 8) / 2, 30}, true, pal::goldDim, 0.29f)) openModal(Modal::Chronicle);
    by += 36;
    if (ui.button("Reclutar consejero (40 oro)", {x, by, wd, 28}, n.gold >= 40, pal::goldDim, 0.28f)) {
        n.gold -= 40;
        int oldestOffice = 0, bestAge = -1;
        for (int id : n.charIds) {
            const Character& c = sim.chars[(size_t)id];
            if (c.alive && c.role == 1 && c.age(sim.abs()) > bestAge) {
                bestAge = c.age(sim.abs());
                oldestOffice = std::max(0, c.office);
            }
        }
        sim.replaceCouncil(sim.player, oldestOffice);
        setStatus("Nuevo consejero nombrado.");
    }
    by += 34;
    if (ui.button("Ayuda y controles (F1)", {x, by, wd, 26}, true, pal::goldDim, 0.26f)) openModal(Modal::Help);

    if (sim.livingRuler(sim.player) < 0 && C.gov(n.gov).election > 0 && !n.isCityState) {
        ui.text("Elecciones pendientes", x, r.y + r.h - 18, 0.24f, pal::war);
    }
}

// ---------------------------------------------------------------------------
// cronica inferior
// ---------------------------------------------------------------------------
void Game::drawLog() {
    Rect r(14, P.h - 148, P.w - 28, 134);
    ui.panel(r, true, true);
    ui.text("CRONICA", r.x + 16, r.y + 10, 0.26f, pal::gold);
    if (ui.smallButton("Ver todo", {r.x + r.w - 92, r.y + 7, 76, 22}, true, pal::goldDim)) openModal(Modal::Chronicle);
    ui.divider(r.x + 16, r.y + 33, r.w - 32, pal::borderSoft);
    int count = std::min<int>(4, (int)sim.log.size());
    for (int i = 0; i < count; ++i) {
        const LogEntry& e = sim.log[sim.log.size() - 1 - (size_t)i];
        Color c = pal::text;
        if (e.kind == 1) c = pal::good;
        else if (e.kind == 2) c = pal::bad;
        else if (e.kind == 3) c = pal::war;
        else if (e.kind == 4) c = pal::gold;
        float y = r.y + 42 + i * 22;
        ui.text(shortDate(e.year, e.month), r.x + 16, y, 0.24f, pal::faint);
        ui.text(ellipsize(G.font, e.text, 0.29f, r.w - 140), r.x + 100, y - 1, 0.29f, c);
    }
}

// ---------------------------------------------------------------------------
// marcadores sobre el globo
// ---------------------------------------------------------------------------
void Game::drawArmyBadge(float x, float y, int unitIdx, int men, Color ring, bool player) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(10, 12, 16, 210);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 24; ++i) {
        float a = i / 24.0f * 2 * PI;
        glVertex2f(x + std::cos(a) * 17, y + std::sin(a) * 17);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    ui.icon(G.iconFor(unitIdx), {x - 13, y - 13, 26, 26});
    glDisable(GL_TEXTURE_2D);
    glColor4ub(ring.r, ring.g, ring.b, 255);
    glLineWidth(player ? 2.2f : 1.4f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 28; ++i) {
        float a = i / 28.0f * 2 * PI;
        glVertex2f(x + std::cos(a) * 17, y + std::sin(a) * 17);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_TEXTURE_2D);
    if (men > 0) {
        std::string s = men >= 1000 ? numStr(men / 1000.0, 1) + "k" : std::to_string(men);
        ui.textC(s, x, y + 18, 0.22f, player ? pal::gold : pal::text, true);
    }
}

void Game::drawLabel(const std::string& text, float x, float y, Color c, float scale, bool center) {
    if (center) ui.textC(text, x, y, scale, c, true);
    else ui.text(text, x, y, scale, c, true);
}

void Game::drawMarkers() {
    struct Group { int prov = 0, nation = 0, unit = 0, men = 0, n = 0; };
    std::map<long long, Group> groups;
    for (auto& a : sim.armies) {
        if (a.men <= 0) continue;
        bool show = a.nation == sim.player || sim.atWar(sim.player, a.nation) || G.cam.dist < 2.55f;
        if (!show) continue;
        long long key = ((long long)a.prov << 20) | a.nation;
        auto& g = groups[key];
        g.prov = a.prov;
        g.nation = a.nation;
        g.men += a.men;
        ++g.n;
        int bi = a.unit;
        if (bi >= 0) {
            if (g.unit == 0 || (a.nation == sim.player && a.unit == sim.player)) g.unit = bi;
        }
    }
    for (auto& kv : groups) {
        Group& g = kv.second;
        if (!sim.validProvince(g.prov)) continue;
        const ProvinceDef& pd = sim.pdef(g.prov);
        Vec3 p = latLonToDir(pd.lat, pd.lon) * 1.018f;
        float sx, sy;
        bool front = false;
        if (!G.project(p, P.w, P.h, sx, sy, front) || !front) continue;
        if (sx < 8 || sx > P.w - 8 || sy < 60 || sy > P.h - 150) continue;
        Color ring = g.nation == sim.player ? pal::gold : (sim.atWar(sim.player, g.nation) ? pal::bad : Color(160, 150, 130));
        drawArmyBadge(sx, sy, g.unit, g.men, ring, g.nation == sim.player);
        if (g.n > 1) {
            glDisable(GL_TEXTURE_2D);
            glColor4ub(226, 190, 105, 240);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(sx + 15, sy - 14);
            for (int i = 0; i <= 16; ++i) {
                float a = i / 16.0f * 2 * PI;
                glVertex2f(sx + 15 + std::cos(a) * 8, sy - 14 + std::sin(a) * 8);
            }
            glEnd();
            glEnable(GL_TEXTURE_2D);
            ui.textC(std::to_string(g.n), sx + 15, sy - 21, 0.22f, Color(20, 16, 10));
        }
    }

    // capitales y nombres de nacion
    std::vector<Rect> used;
    for (auto& n : sim.nations) {
        if (!n.alive || n.id == 0 || n.capital < 0) continue;
        const ProvinceDef& pd = sim.pdef(n.capital);
        Vec3 p = latLonToDir(pd.lat, pd.lon) * 1.012f;
        float sx, sy;
        bool front = false;
        if (!G.project(p, P.w, P.h, sx, sy, front) || !front) continue;
        if (sx < 340 || sx > P.w - 350 || sy < 72 || sy > P.h - 160) continue;
        std::string label = n.name;
        float tw = ui.textW(label, 0.28f);
        Rect lr(sx - tw * 0.5f - 4, sy - 26, tw + 8, 18);
        bool overlap = false;
        for (auto& u : used) if (!(lr.x + lr.w < u.x || lr.x > u.x + u.w || lr.y + lr.h < u.y || lr.y > u.y + u.h)) { overlap = true; break; }
        if (overlap) continue;
        used.push_back(lr);
        glDisable(GL_TEXTURE_2D);
        glColor4ub(n.color.r, n.color.g, n.color.b, 255);
        glBegin(GL_QUADS);
        glVertex2f(sx - 3, sy - 4);
        glVertex2f(sx + 3, sy - 4);
        glVertex2f(sx + 3, sy + 2);
        glVertex2f(sx - 3, sy + 2);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        ui.textC(label, sx, sy - 24, 0.28f, n.color.lighten(0.55f), true);
    }
}

// ---------------------------------------------------------------------------
// modales
// ---------------------------------------------------------------------------
void Game::drawModalFrame(const std::string& title, Rect r) {
    ui.dimScreen();
    ui.panel(r);
    ui.rect({r.x, r.y, r.w, 52}, Color(36, 34, 29, 250));
    ui.rect({r.x, r.y + 51, r.w, 2}, pal::gold.withAlpha(120));
    ui.textC(title, r.x + r.w * 0.5f, r.y + 10, 0.46f, pal::gold, true);
    if (modal != Modal::Elections && modal != Modal::GameOver) {
        if (ui.button("X", {r.x + r.w - 44, r.y + 10, 30, 30}, true, pal::bad, 0.32f)) modal = Modal::None;
    }
}

void Game::drawModal() {
    switch (modal) {
        case Modal::Event: drawEventModal(); break;
        case Modal::FoundNation: drawFoundModal(); break;
        case Modal::Buildings: drawBuildingsModal(); break;
        case Modal::Recruit: drawRecruitModal(); break;
        case Modal::Diplomacy: drawDiplomacyModal(); break;
        case Modal::Chronicle: drawChronicleModal(); break;
        case Modal::Elections: drawElectionsModal(); break;
        case Modal::NationDetails: drawNationDetails(); break;
        case Modal::Negotiation: drawNegotiation(); break;
        case Modal::Resources: drawResourcesModal(); break;
        case Modal::Technology: drawTechnologyModal(); break;
        case Modal::Help: drawHelpModal(); break;
        case Modal::GameOver: drawGameOverModal(); break;
        default: break;
    }
}

void Game::drawEventModal() {
    Rect r(P.w * 0.5f - 290, P.h * 0.5f - 140, 580, 280);
    Color accent = eventKind == 2 ? pal::bad : (eventKind == 4 ? pal::gold : (eventKind == 3 ? pal::war : pal::good));
    drawModalFrame("ACONTECIMIENTO", r);
    ui.text(eventTitle, r.x + 28, r.y + 78, 0.52f, accent, true);
    ui.textBlock(eventText, r.x + 28, r.y + 120, r.w - 56, 0.34f, pal::text, 3, 5);
    if (ui.button("Continuar", {r.x + r.w - 170, r.y + r.h - 62, 140, 36}, true, pal::gold, 0.34f)) modal = Modal::None;
    if (ui.button("Cronica", {r.x + 28, r.y + r.h - 62, 120, 36}, true, pal::goldDim, 0.30f)) modal = Modal::Chronicle;
}

void Game::drawFoundModal() {
    Rect r(P.w * 0.5f - 380, P.h * 0.5f - 280, 760, 560);
    drawModalFrame("FUNDAR UNA NACION", r);
    Nation& n = sim.nations[(size_t)sim.player];
    int provs = 0;
    for (auto& p : sim.prov) if (p.owner == sim.player) ++provs;
    ui.text("Provincias " + std::to_string(provs) + "   ·   Oro " + numStr(n.gold, 0) +
            "   ·   Tecnologia " + numStr(n.tech, 1) + " / 3", r.x + 24, r.y + 66, 0.30f, pal::dim);
    bool can = sim.canFound(sim.player);
    if (!can) ui.text("Requisitos: 2 provincias, 60 oro, 5 de prestigio y 3 puntos tecnologicos.", r.x + 24, r.y + 88, 0.27f, pal::bad);
    else ui.text("Elige el nombre y el gobierno de tu pueblo.", r.x + 24, r.y + 88, 0.27f, pal::good);

    std::vector<int> list;
    for (int i = 0; i < (int)C.nationTemplates.size(); ++i) {
        const NationTemplate& t = C.nationTemplates[(size_t)i];
        if (t.culture != n.culture || t.era > n.era || sim.year < t.earliestYear) continue;
        list.push_back(i);
    }
    float ry = r.y + 118;
    int visible = 5;
    for (int k = 0; k < visible; ++k) {
        int idx = modalScroll + k;
        if (idx >= (int)list.size()) break;
        const NationTemplate& t = C.nationTemplates[(size_t)list[(size_t)idx]];
        bool used = false;
        for (auto& o : sim.nations) if (o.alive && o.id != sim.player && o.name == t.name) used = true;
        Rect row(r.x + 20, ry + k * 78, r.w - 40, 70);
        ui.rect(row, Color(19, 21, 25, 220));
        ui.outline(row, used ? Color(80, 76, 70, 100) : pal::borderSoft, 1);
        if (!drawMedia("nation_"+t.id, {row.x+8,row.y+12,40,40})) ui.rect({row.x + 10, row.y + 14, 8, 40}, t.color);
        ui.text(t.name, row.x + 54, row.y + 8, 0.42f, used ? pal::faint : pal::text);
        ui.text(C.gov(t.gov).name + "  ·  " + t.adj, row.x + 54, row.y + 38, 0.26f, pal::dim);
        ui.textBlock(t.desc, row.x + 54, row.y + 54, row.w - 190, 0.24f, pal::faint, 0, 1);
        bool canThis = can && !used;
        if (used) ui.text("Nombre en uso", row.x + row.w - 130, row.y + 24, 0.24f, pal::bad);
        else if (ui.button("Fundar", {row.x + row.w - 118, row.y + 18, 104, 32}, canThis, pal::gold, 0.28f)) {
            if (sim.foundNation(sim.player, list[(size_t)idx])) {
                modal = Modal::None;
                setStatus("Has fundado " + sim.nations[(size_t)sim.player].name + ".");
            }
        }
    }
    if (list.empty()) ui.text("Tu cultura aun no tiene naciones historicas disponibles.", r.x + 24, r.y + 150, 0.30f, pal::dim);
    if (list.size() > 5) {
        if (ui.smallButton("Subir", {r.x + 24, r.y + r.h - 50, 78, 28}, modalScroll > 0)) modalScroll = std::max(0, modalScroll - 1);
        if (ui.smallButton("Bajar", {r.x + 108, r.y + r.h - 50, 78, 28}, modalScroll + 5 < (int)list.size())) modalScroll = std::min((int)list.size() - 5, modalScroll + 1);
        ui.text(std::to_string(modalScroll + 1) + "-" + std::to_string(std::min<int>((int)list.size(), modalScroll + 5)) +
                " de " + std::to_string(list.size()), r.x + 200, r.y + r.h - 44, 0.26f, pal::faint);
    }
}

void Game::drawBuildingsModal() {
    Rect r(P.w * 0.5f - 380, P.h * 0.5f - 280, 760, 560);
    drawModalFrame("CONSTRUIR", r);
    if (!sim.validProvince(selectedProvince) || sim.prov[(size_t)selectedProvince].owner != sim.player) {
        ui.text("Selecciona una provincia propia.", r.x + 24, r.y + 80, 0.34f, pal::dim);
        return;
    }
    ui.text(sim.provName(selectedProvince), r.x + 24, r.y + 66, 0.36f, pal::text);
    const Nation& n = sim.nations[(size_t)sim.player];
    ui.textR("Oro " + numStr(n.gold, 0) + "  ·  Madera " + numStr(n.wood, 0) + "  ·  Piedra " + numStr(n.stone, 0),
             r.x + r.w - 24, r.y + 69, 0.26f, pal::dim);
    int visible = 5;
    for (int k = 0; k < visible; ++k) {
        int idx = modalScroll + k;
        if (idx >= (int)C.buildings.size()) break;
        const BuildingDef& b = C.building(idx);
        bool built = C.hasBuilding(sim.prov[(size_t)selectedProvince].buildings, b.id);
        std::string why;
        bool can = sim.canBuild(sim.player, selectedProvince, idx, &why);
        Rect row(r.x + 20, r.y + 98 + k * 82, r.w - 40, 74);
        ui.rect(row, built ? Color(22, 30, 22, 220) : Color(19, 21, 25, 220));
        ui.outline(row, built ? pal::good.withAlpha(70) : pal::borderSoft, 1);
        ui.text(b.name, row.x + 14, row.y + 8, 0.36f, built ? pal::good : pal::text);
        ui.text("Era: " + C.era(b.era).shortName + "  ·  " + buildCosts(b), row.x + 14, row.y + 32, 0.25f, pal::dim);
        ui.textBlock(b.desc, row.x + 14, row.y + 50, row.w - 180, 0.24f, pal::faint, 0, 1);
        if (built) {
            ui.text("Construido", row.x + row.w - 126, row.y + 24, 0.26f, pal::good);
        } else if (ui.button("Construir", {row.x + row.w - 122, row.y + 20, 108, 32}, can, pal::gold, 0.28f)) {
            if (sim.build(sim.player, selectedProvince, idx)) setStatus(b.name + " construido en " + sim.provName(selectedProvince) + ".");
        } else if (ui.hover({row.x + row.w - 122, row.y + 20, 108, 32}) && !can && !why.empty()) {
            ui.tooltip = why;
        }
    }
    if (C.buildings.size() > 5) {
        if (ui.smallButton("Subir", {r.x + 24, r.y + r.h - 46, 78, 26}, modalScroll > 0)) modalScroll = std::max(0, modalScroll - 1);
        if (ui.smallButton("Bajar", {r.x + 108, r.y + r.h - 46, 78, 26}, modalScroll + 5 < (int)C.buildings.size())) modalScroll = std::min((int)C.buildings.size() - 5, modalScroll + 1);
    }
}

void Game::drawRecruitModal() {
    Rect r(P.w * 0.5f - 390, P.h * 0.5f - 280, 780, 560);
    drawModalFrame("RECLUTAR EJERCITO", r);
    if (!sim.validProvince(selectedProvince) || sim.prov[(size_t)selectedProvince].owner != sim.player) {
        ui.text("Selecciona una provincia propia.", r.x + 24, r.y + 80, 0.34f, pal::dim);
        return;
    }
    ui.text(sim.provName(selectedProvince), r.x + 24, r.y + 64, 0.36f, pal::text);
    const Nation& n = sim.nations[(size_t)sim.player];
    ui.textR("Oro " + numStr(n.gold, 0) + "  ·  Mano de obra " + numStr(n.manpower, 1) + "k",
             r.x + r.w - 24, r.y + 68, 0.26f, pal::dim);
    if(ui.wheel)modalScroll-=(int)ui.wheel;
    modalScroll=std::clamp(modalScroll,0,std::max(0,(int)C.units.size()-4));
    for (int k = modalScroll; k < std::min(modalScroll+4,(int)C.units.size()); ++k) {
        const UnitDef& u = C.unit(k);
        std::string why;
        bool can = sim.canRecruit(sim.player, selectedProvince, k, &why);
        Rect row(r.x + 20, r.y + 96 + (k-modalScroll) * 100, r.w - 40, 92);
        ui.rect(row, Color(19, 21, 25, 220));
        ui.outline(row, can ? pal::borderSoft : Color(70, 66, 60, 100), 1);
        ui.icon(G.iconFor(k), {row.x + 10, row.y + 18, 56, 56});
        ui.text(sim.unitName(sim.player,(int)(&u-C.units.data())), row.x + 78, row.y + 8, 0.40f, pal::text);
        ui.text("Era: " + C.era(u.era).shortName + "  ·  " + std::to_string(u.men) + " hombres  ·  ATQ " +
                numStr(u.atk, 0) + "  DEF " + numStr(u.defense, 0), row.x + 78, row.y + 34, 0.25f, pal::dim);
        ui.text(unitCosts(u), row.x + 78, row.y + 54, 0.25f, pal::goldDim);
        ui.textBlock(u.desc, row.x + 78, row.y + 70, row.w - 240, 0.23f, pal::faint, 0, 1);
        if (ui.button("Reclutar", {row.x + row.w - 126, row.y + 28, 110, 34}, can, pal::gold, 0.29f)) {
            if (sim.recruit(sim.player, selectedProvince, k)) {
                setStatus(u.plural + " reclutados en " + sim.provName(selectedProvince) + ".");
                modal = Modal::None;
            }
        } else if (ui.hover({row.x + row.w - 126, row.y + 28, 110, 34}) && !can && !why.empty()) {
            ui.tooltip = why;
        }
    }
    if(ui.smallButton("Subir",{r.x+24,r.y+r.h-36,90,26},modalScroll>0))--modalScroll;
    if(ui.smallButton("Bajar",{r.x+124,r.y+r.h-36,90,26},modalScroll+4<(int)C.units.size()))++modalScroll;

}

void Game::drawDiplomacyModal() {
    Rect r(P.w * 0.5f - 380, P.h * 0.5f - 280, 760, 560);
    drawModalFrame("DIPLOMACIA", r);
    const Nation& me = sim.nations[(size_t)sim.player];
    struct Item { int id; float dist; };
    std::vector<Item> items;
    Vec3 capDir = latLonToDir(sim.pdef(me.capital).lat, sim.pdef(me.capital).lon);
    for (auto& n : sim.nations) {
        if (!n.alive || n.id == 0 || n.id == sim.player) continue;
        Vec3 d = latLonToDir(sim.pdef(n.capital).lat, sim.pdef(n.capital).lon);
        float ang = std::acos(clampf(dot(capDir, d), -1, 1));
        items.push_back({n.id, ang});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.dist < b.dist; });
    int visible = 6;
    for (int k = 0; k < visible; ++k) {
        int idx = modalScroll + k;
        if (idx >= (int)items.size()) break;
        const Nation& o = sim.nations[(size_t)items[(size_t)idx].id];
        bool war = sim.atWar(sim.player, o.id);
        int provs = 0;
        for (auto& p : sim.prov) if (p.owner == o.id) ++provs;
        Rect row(r.x + 20, r.y + 92 + k * 72, r.w - 40, 64);
        ui.rect(row, Color(19, 21, 25, 220));
        ui.outline(row, war ? pal::bad.withAlpha(110) : pal::borderSoft, 1);
        drawMedia(G.nationMedia(o,C),{row.x+8,row.y+12,40,40});
        ui.text(o.name, row.x + 60, row.y + 6, 0.38f, pal::text);
        ui.text((o.isCityState ? "Ciudad-estado" : C.gov(o.gov).name) + "  ·  " + std::to_string(provs) +
                " prov.  ·  " + numStr(sim.nationArmyPower(o.id), 1) + " fuerza", row.x + 60, row.y + 32, 0.24f, pal::dim);
        if (war) {
            ui.text("En guerra (" + std::to_string(sim.warMonths(sim.player, o.id)) + " meses)", row.x + 60, row.y + 47, 0.23f, pal::bad);
        } else {
            ui.text("En paz", row.x + 60, row.y + 47, 0.23f, pal::faint);
        }
        if(ui.smallButton("Negociar",{row.x+row.w-280,row.y+16,125,30})) {diplomacyTarget=o.id;openModal(Modal::Negotiation);}
        if (war) {
            if (ui.button("Proponer paz", {row.x + row.w - 142, row.y + 16, 128, 32}, true, pal::good, 0.26f)) {
                if (sim.diplomatic(sim.player,o.id,"peace")) {
                    setStatus("Acuerdo realizado.");
                } else {
                    setStatus(o.name + " rechaza la paz por ahora.");
                }
            }
        } else {
            if (ui.button("Declarar guerra", {row.x + row.w - 156, row.y + 16, 142, 32}, true, pal::bad, 0.25f)) {
                sim.declareWar(sim.player, o.id);
            }
        }
    }
    if (items.size() > 6) {
        if (ui.smallButton("Subir", {r.x + 24, r.y + r.h - 46, 78, 26}, modalScroll > 0)) modalScroll = std::max(0, modalScroll - 1);
        if (ui.smallButton("Bajar", {r.x + 108, r.y + r.h - 46, 78, 26}, modalScroll + 6 < (int)items.size())) modalScroll = std::min((int)items.size() - 6, modalScroll + 1);
    }
}

void Game::drawChronicleModal() {
    Rect r(P.w * 0.5f - 380, P.h * 0.5f - 290, 760, 580);
    drawModalFrame("CRONICA DE LA HUMANIDAD", r);
    int maxShow = 13;
    int total = (int)sim.log.size();
    int start = std::max(0, total - maxShow - chronicleScroll);
    int count = std::min(maxShow, total - start);
    ui.text("Ultimos acontecimientos", r.x + 24, r.y + 64, 0.27f, pal::dim);
    for (int i = 0; i < count; ++i) {
        const LogEntry& e = sim.log[(size_t)(start + count - 1 - i)];
        Color c = pal::text;
        if (e.kind == 1) c = pal::good;
        else if (e.kind == 2) c = pal::bad;
        else if (e.kind == 3) c = pal::war;
        else if (e.kind == 4) c = pal::gold;
        float y = r.y + 90 + i * 31;
        ui.text(shortDate(e.year, e.month), r.x + 24, y, 0.24f, pal::faint);
        ui.text(ellipsize(G.font, e.text, 0.28f, r.w - 180), r.x + 108, y, 0.28f, c);
    }
    if (total > maxShow) {
        if (ui.smallButton("Mas antiguo", {r.x + 24, r.y + r.h - 48, 110, 28}, chronicleScroll < total - maxShow)) chronicleScroll += 6;
        if (ui.smallButton("Mas reciente", {r.x + 142, r.y + r.h - 48, 110, 28}, chronicleScroll > 0)) chronicleScroll = std::max(0, chronicleScroll - 6);
    }
    if (ui.wheel != 0 && ui.hover(r)) {
        chronicleScroll = std::clamp(chronicleScroll + (ui.wheel > 0 ? -3 : 3), 0, std::max(0, total - maxShow));
    }
}

void Game::drawElectionsModal() {
    Rect r(P.w * 0.5f - 440, P.h * 0.5f - 280, 880, 560);
    drawModalFrame("ELECCIONES", r);
    Nation& n = sim.nations[(size_t)sim.player];
    ui.textBlock("El pueblo elige a su gobernante. La reeleccion es posible, pero nadie escapa al tiempo: "
                 "los lideres envejecen y mueren, y no regresan.",
                 r.x + 24, r.y + 64, r.w - 48, 0.28f, pal::dim, 2, 2);
    std::vector<int> cands;
    for (int id : n.charIds) {
        if (!sim.chars[(size_t)id].alive) continue;
        cands.push_back(id);
    }
    std::sort(cands.begin(), cands.end(), [&](int a, int b) {
        const Character& ca = sim.chars[(size_t)a];
        const Character& cb = sim.chars[(size_t)b];
        float va = ca.admin + ca.martial + ca.diplomacy + (ca.role == 0 ? 4 : 0);
        float vb = cb.admin + cb.martial + cb.diplomacy + (cb.role == 0 ? 4 : 0);
        return va > vb;
    });
    if (cands.size() > 4) cands.resize(4);
    for (size_t i = 0; i < cands.size(); ++i) {
        const Character& c = sim.chars[(size_t)cands[i]];
        float cx = r.x + 24 + (i % 2) * ((r.w - 48) / 2 + 4);
        float cy = r.y + 124 + (i / 2) * 196;
        float cw = (r.w - 56) / 2;
        Rect card(cx, cy, cw, 182);
        ui.rect(card, Color(20, 22, 27, 240));
        ui.outline(card, c.role == 0 ? pal::gold : pal::borderSoft, c.role == 0 ? 1.5f : 1.0f);
        ui.portrait({card.x + 16, card.y + 16, 68, 68}, c, n.color, c.role == 0);
        ui.text(c.name, card.x + 96, card.y + 14, 0.38f, pal::text);
        ui.text("Edad " + std::to_string(c.age(sim.abs())) + "  ·  " + (c.role == 0 ? "Gobernante actual" : "Consejero"),
                card.x + 96, card.y + 40, 0.24f, pal::dim);
        ui.text("Admin " + numStr(c.admin, 0) + "   Marcial " + numStr(c.martial, 0) + "   Diplo " + numStr(c.diplomacy, 0),
                card.x + 16, card.y + 94, 0.25f, pal::text);
        ui.chip(C.trait(c.traitA).name, card.x + 16, card.y + 114, Color(150, 176, 200), 0.21f);
        ui.chip(C.trait(c.traitB).name, card.x + 16 + ui.textW(C.trait(c.traitA).name, 0.21f) + 22, card.y + 114, Color(200, 170, 120), 0.21f);
        Rect b(card.x + 16, card.y + 140, cw - 32, 30);
        if (ui.button(c.role == 0 ? "Reelegir" : "Elegir", b, true, pal::gold, 0.28f)) {
            sim.holdElection(sim.player, cands[i]);
            modal = Modal::None;
            setStatus(c.name + " gobernara " + n.name + ".");
        }
    }
    ui.text("La eleccion no puede posponerse.", r.x + 24, r.y + r.h - 32, 0.26f, pal::faint);
}

void Game::drawHelpModal() {
    Rect r(P.w * 0.5f - 390, P.h * 0.5f - 300, 780, 600);
    drawModalFrame("AYUDA Y CONTROLES", r);
    float x = r.x + 28, y = r.y + 68, wd = r.w - 56;
    ui.text("EL GLOBO", x, y, 0.28f, pal::gold);
    ui.textBlock("Arrastra con el boton izquierdo para girar la Tierra. Usa la rueda para acercarte. "
                 "El mapa completo es la Tierra; la partida comienza en Europa, Africa del Norte y Oriente Proximo.",
                 x, y + 22, wd, 0.28f, pal::text, 1, 3);
    y += 88;
    ui.text("ECONOMIA Y ERAS", x, y, 0.28f, pal::gold);
    ui.textBlock("Oro, alimento, madera, piedra y cobre sostienen tu pueblo. Construye granjas, minas, "
                 "puertos y templos. La tecnologia te hara avanzar del Neolitico al Calcolitico, el Bronce, "
                 "el Hierro y la Epoca Clasica. Cada era mejora la produccion y desbloquea unidades.",
                 x, y + 22, wd, 0.28f, pal::text, 1, 4);
    y += 110;
    ui.text("PUEBLOS Y LIDERES", x, y, 0.28f, pal::gold);
    ui.textBlock("Empiezas como una ciudad-estado historica. Coloniza provincias vecinas y funda una nacion "
                 "(Egipto, Sumeria, Siria, Canaán...). Tus gobernantes y consejeros envejecen y mueren: "
                 "jamas vuelven. En las republicas hay elecciones cada 8 anos y la reeleccion no detiene la edad.",
                 x, y + 22, wd, 0.28f, pal::text, 1, 4);
    y += 110;
    ui.text("GUERRA", x, y, 0.28f, pal::gold);
    ui.textBlock("Recluta soldados, lanceros, jinetes (requieren caballos y la Edad del Bronce) y barcos "
                 "(requieren un puerto). Selecciona un ejercito y haz clic en una provincia de destino. "
                 "Las batallas se resuelven al llegar; el terreno y las murallas favorecen al defensor.",
                 x, y + 22, wd, 0.28f, pal::text, 1, 4);
    y += 110;
    ui.text("TECLAS", x, y, 0.28f, pal::gold);
    ui.text("Espacio: pausa   ·   1-4: velocidad   ·   Esc: cerrar   ·   F5: guardar   ·   F9: cargar   ·   F1: ayuda",
            x, y + 22, 0.26f, pal::dim);
    ui.text("Solaris History · Open Source GPLv3 · Motor C++ y contenido Java · Texturas NASA Blue Marble",
            x, r.y + r.h - 40, 0.24f, pal::faint);
    if (ui.button("Entendido", {r.x + r.w - 174, r.y + r.h - 66, 146, 34}, true, pal::gold, 0.32f)) modal = Modal::None;
}

void Game::drawGameOverModal() {
    Rect r(P.w * 0.5f - 290, P.h * 0.5f - 150, 580, 300);
    drawModalFrame("FIN DE UNA ERA", r);
    ui.text("Tu nacion ha caido", r.x + 28, r.y + 76, 0.54f, pal::bad, true);
    ui.textBlock("Las ciudades se desmoronan, los nombres se olvidan y la historia sigue sin ti. "
                 "Puedes comenzar de nuevo en otra ciudad-estado.",
                 r.x + 28, r.y + 120, r.w - 56, 0.30f, pal::text, 2, 3);
    if (ui.button("Nueva partida", {r.x + 28, r.y + r.h - 64, 170, 38}, true, pal::gold, 0.32f)) {
        screen = Screen::Menu;
        modal = Modal::None;
        speed = 0;
        sim.init(C, 0);
    }
    if (ui.button("Salir", {r.x + 214, r.y + r.h - 64, 120, 38}, true, pal::goldDim, 0.32f)) P.running = false;
}

void Game::loadGame() {
    Sim candidate;
    candidate.C = &C;
    if (!candidate.load("solaris_quicksave.sav")) {
        setStatus("No se pudo cargar la partida. Comprueba que usa el mismo mapa.");
        return;
    }
    sim = std::move(candidate);
    screen = Screen::Play;
    selectedProvince = sim.nations[(size_t)sim.player].capital;
    selectedArmyId = -1;
    modal = Modal::None;
    speed = 0;
    tickTimer = 0;
    setStatus("Partida cargada.");
}

bool Game::drawMedia(const std::string& id, Rect r) {
    GLuint texture=G.mediaFor(id);
    if (!texture) return false;
    if (id.rfind("flag_",0)!=0) {
        ui.rect(r,Color(222,215,194));
        ui.outline(r,pal::borderSoft,1);
    }
    ui.icon(texture,r);
    if (ui.hover(r)) {
        auto it=G.mediaCredits.find(id);
        if (it!=G.mediaCredits.end()) {
            const auto& c=it->second;
            ui.tooltip=c.title+" · "+c.license+" · "+(Locale::english?c.note:c.noteEs);
        }
    }
    return true;
}

void Game::drawLanguageFlag(Rect r, bool english) {
    if (drawMedia(english?"flag_en":"flag_es", {r.x,r.y+(r.h-r.w)*0.5f,r.w,r.w})) return;
    if (!english) {
        ui.rect(r, Color(170, 21, 27));
        ui.rect({r.x, r.y+r.h*0.25f, r.w, r.h*0.5f}, Color(241, 191, 0));
    } else {
        ui.rect(r, Color(1, 33, 105));
        auto diagonal = [&](float x1, float y1, float x2, float y2, float thickness, Color color) {
            float dx=x2-x1, dy=y2-y1, length=std::sqrt(dx*dx+dy*dy);
            float nx=-dy/length*thickness/2, ny=dx/length*thickness/2;
            glDisable(GL_TEXTURE_2D); glColor4ub(color.r,color.g,color.b,color.a);
            glBegin(GL_QUADS);
            glVertex2f(x1+nx,y1+ny); glVertex2f(x2+nx,y2+ny);
            glVertex2f(x2-nx,y2-ny); glVertex2f(x1-nx,y1-ny);
            glEnd(); glEnable(GL_TEXTURE_2D);
        };
        // Scissor keeps the diagonal saltire within the flag.
        glEnable(GL_SCISSOR_TEST); glScissor((int)r.x, P.h-(int)(r.y+r.h), (int)r.w, (int)r.h);
        for (int i=0;i<2;++i) {
            Color c = i ? Color(200,16,46) : Color(255,255,255);
            diagonal(r.x,r.y,r.x+r.w,r.y+r.h,r.h*(i?0.08f:0.22f),c);
            diagonal(r.x,r.y+r.h,r.x+r.w,r.y,r.h*(i?0.08f:0.22f),c);
        }
        glDisable(GL_SCISSOR_TEST);
        ui.rect({r.x+r.w*0.4f,r.y,r.w*0.2f,r.h}, Color(255,255,255));
        ui.rect({r.x,r.y+r.h*0.34f,r.w,r.h*0.32f}, Color(255,255,255));
        ui.rect({r.x+r.w*0.44f,r.y,r.w*0.12f,r.h}, Color(200,16,46));
        ui.rect({r.x,r.y+r.h*0.4f,r.w,r.h*0.2f}, Color(200,16,46));
    }
    ui.outline(r, pal::goldDim);
}

void Game::drawTechnologyModal() {
    Rect r(P.w*0.5f-380, P.h*0.5f-280, 760, 560);
    drawModalFrame("Tecnologias", r);
    auto& n=sim.nations[sim.player];
    const char* branches[]={"militar","economica","social"};
    const char* labels[]={"Militar","Economica","Social"};
    for(int j=0;j<3;++j)if(ui.tab(labels[j],{r.x+24+j*230.0f,r.y+58,220,30},technologyTab==j)){technologyTab=j;modalScroll=0;}
    ui.text("Investigacion disponible: "+numStr(n.tech-n.spentTech,1),r.x+24,r.y+96,0.28f,pal::gold);
    std::map<std::string,std::pair<int,int>> positions;
    std::vector<int> list;
    for(int i=0;i<(int)C.technologies.size();++i)if(C.technologies[i].branch==branches[technologyTab])list.push_back(i);
    for(int pass=0;pass<(int)list.size()+1;++pass)for(int i:list) {
        const auto& t=C.technologies[i];if(positions.count(t.id))continue;
        bool local=false;for(int j:list)if(C.technologies[j].id==t.requires)local=true;
        if(local&&!positions.count(t.requires))continue;
        int depth=local?positions[t.requires].first+1:0;
        int column=local?positions[t.requires].second:0;
        for(const auto& p:positions)if(p.second.first==depth&&p.second.second==column)column=1;
        positions[t.id]={depth,column};
    }
    int maxDepth=0;for(const auto& p:positions)maxDepth=std::max(maxDepth,p.second.first);
    if(ui.wheel)modalScroll-=(int)ui.wheel;
    modalScroll=std::clamp(modalScroll,0,std::max(0,maxDepth-3));
    for(int i:list) {
        const auto& t=C.technologies[i];auto place=positions[t.id];int row=place.first-modalScroll;
        if(row<0||row>=4)continue;
        float x=r.x+24+place.second*352,y=r.y+130+row*86;
        auto parent=positions.find(t.requires);
        if(parent!=positions.end()&&parent->second.first>=modalScroll) {
            float px=r.x+45+parent->second.second*352;
            ui.rect({px,y-40,2,40},pal::goldDim);
            ui.rect({std::min(px,x+22),y-5,std::fabs(px-x-22)+2,2},pal::goldDim);
        }
        ui.rect({x,y,336,72},pal::panelSoft);drawMedia("tech_"+t.id,{x+4,y+7,40,40});
        bool known=sim.hasTech(sim.player,t.id);
        ui.text(ellipsize(G.font,t.name,.27f,180),x+50,y+4,.27f,known?pal::good:pal::text);
        ui.text(numStr(t.points,0)+" pt · "+yearLabel(t.year),x+50,y+25,.24f,pal::dim);
        std::string required=t.requires.empty()?"Raiz":t.requires;
        for(const auto& prior:C.technologies)if(prior.id==t.requires)required=prior.name;
        ui.text(ellipsize(G.font,"Requiere: "+required,.23f,280),x+8,y+50,.23f,pal::dim);
        bool ready=!known && sim.year>=t.year && n.tech-n.spentTech>=t.points && (t.requires.empty()||sim.hasTech(sim.player,t.requires));
        if(ui.smallButton(known?"OK":"Investigar",{x+239,y+8,90,30},ready))sim.research(sim.player,i);
    }
    if(ui.smallButton("Subir",{r.x+24,r.y+r.h-38,90,28},modalScroll>0))--modalScroll;
    if(ui.smallButton("Bajar",{r.x+124,r.y+r.h-38,90,28},modalScroll+3<maxDepth))++modalScroll;

}

void Game::drawResourcesModal() {
    Rect r(P.w*0.5f-380,P.h*0.5f-280,760,560);
    drawModalFrame("Recursos",r);
    ui.text("Produccion y comercio de tus provincias",r.x+24,r.y+62,0.30f,pal::dim);
    const int visible=6;
    if(ui.wheel) modalScroll-=(int)ui.wheel;
    modalScroll=std::clamp(modalScroll,0,std::max(0,(int)C.resources.size()-visible));
    for(int k=0;k<visible && modalScroll+k<(int)C.resources.size();++k) {
        const auto& resource=C.resources[modalScroll+k];
        float y=r.y+94+k*67;
        drawMedia("resource_"+resource.id,{r.x+24,y,54,54});
        ui.text(resource.name,r.x+96,y+4,0.36f,pal::text);
        int count=0;
        for(const auto& province:sim.prov)
            if(province.owner==sim.player && C.province(province.def).resourceId==resource.id) ++count;
        ui.text("Provincias propias: "+std::to_string(count)+"   ·   Valor comercial: "+numStr(resource.value,1),r.x+96,y+30,0.27f,pal::dim);
    }
    if(ui.smallButton("Subir",{r.x+24,r.y+r.h-40,90,28},modalScroll>0)) --modalScroll;
    if(ui.smallButton("Bajar",{r.x+124,r.y+r.h-40,90,28},modalScroll+visible<(int)C.resources.size())) ++modalScroll;
}


void Game::drawNegotiation() {
    Rect r(P.w*.5f-380,P.h*.5f-280,760,560);drawModalFrame("Diplomacia avanzada",r);
    if(!sim.nationAlive(diplomacyTarget)){modal=Modal::None;return;}
    const auto& o=sim.nations[diplomacyTarget];
    drawMedia(G.nationMedia(o,C),{r.x+24,r.y+64,60,60});ui.text(o.name,r.x+100,r.y+70,.4f,pal::gold);
    ui.text("Senor: "+sim.nationName(o.overlord),r.x+100,r.y+100,.26f,pal::dim);
    const auto& allies=sim.nations[sim.player].allies;
    ui.text(std::find(allies.begin(),allies.end(),o.id)!=allies.end()?"Alianza activa":"Sin alianza",r.x+100,r.y+125,.25f,pal::dim);
    const char* labels[]={"Paz (6 meses de guerra)","Alianza (5 prestigio)","Romper alianza","Compartir 20 oro","Compartir 20 alimentos","Compartir 20 madera","Titere: 12 meses, 100 oro, fuerza x2","Anexar: titere 10 anos, 200 oro","Liberar titere","Compartir 20 piedra","Compartir 20 cobre"};
    const char* actions[]={"peace","alliance","break","gold","food","wood","puppet","annex","release","stone","copper"};
    for(int i=0;i<11;++i)if(ui.button(labels[i],{r.x+24+(i%2)*355.0f,r.y+150+(i/2)*64.0f,340,46},true,pal::gold,.27f))
        setStatus(sim.diplomatic(sim.player,diplomacyTarget,actions[i])?"Acuerdo realizado.":"La propuesta no cumple los requisitos.");
    if(statusTimer>0)ui.text(ellipsize(G.font,status,.26f,700),r.x+24,r.y+r.h-26,.26f,pal::gold);
}
void Game::drawNationDetails() {
    Rect r(P.w*.5f-420,P.h*.5f-280,840,560);drawModalFrame("Gestion nacional",r);
    const char* tabs[]={"Decisiones","Religion","Cultura","Militar","Consejeros"};
    for(int i=0;i<5;++i)if(ui.tab(tabs[i],{r.x+16+i*162.0f,r.y+58,156,30},nationTab==i))nationTab=i;
    auto& n=sim.nations[sim.player];float x=r.x+24,y=r.y+114;
    if(nationTab==0) {
        ui.text("40 oro por decision; espera de 5 anos.",x,y,.3f,pal::dim);
        const char* names[]={"Festival: +10 estabilidad","Levas: +5 mano de obra, -5 estabilidad","Monarquia","Republica","Teocracia"};
        const char* actions[]={"festival","levy","monarquia","republica","teocracia"};
        for(int i=0;i<5;++i)if(ui.button(names[i],{x,y+44+i*64.0f,650,44}))setStatus(sim.decide(sim.player,actions[i])?"Acuerdo realizado.":"La propuesta no cumple los requisitos.");
        if(statusTimer>0)ui.text(status,x,r.y+r.h-28,.26f,pal::gold);
    } else if(nationTab==1) {
        int i=0;for(const auto& rel:C.religions.array()) {
            float yy=y+i++*74;drawMedia(rel["icon_media"].string(),{x,yy,52,52});
            ui.text(rel["name"].string()+(n.religion==rel["tag"].string()?" *":""),x+66,yy,.32f,pal::text);
            ui.text("Agricultura "+numStr(rel["farm"].number()*100,0)+"% · Militar "+numStr(rel["military"].number()*100,0)+"% · Estabilidad "+numStr(rel["stability"].number()*100,0)+"%",x+66,yy+27,.25f,pal::dim);
            if(ui.smallButton("Adoptar",{r.x+r.w-130,yy,100,30},n.gold>=100 && sim.abs()-n.decisionAt>=60 && n.religion!=rel["tag"].string())){n.gold-=100;n.stability=clampf(n.stability-15,0,100);n.religion=rel["tag"].string();n.decisionAt=sim.abs();}
        }
        ui.text("Conversion: 100 oro, -15 estabilidad; 5 anos de espera.",x,r.y+r.h-38,.27f,pal::war);
    } else if(nationTab==2) {
        drawMedia("culture_"+n.culture,{x,y,96,96});
        ui.text(C.culture(n.culture).name,x+120,y,.38f,pal::gold);
        ui.textBlock(C.culture(n.culture).desc,x+120,y+50,620,.3f,pal::text);
        ui.text("Agricultura "+numStr(C.culture(n.culture).farm*100,0)+"% · Militar "+numStr(C.culture(n.culture).mil*100,0)+"%",x,y+150,.3f,pal::dim);
    } else if(nationTab==3) {
        ui.text("Modernizar ejercitos: 20 oro por unidad; exige tecnologia.",x,y,.3f,pal::dim);
        if(ui.button("Modernizar",{x,y+44,240,36}))for(int i=0;i<(int)sim.armies.size();++i)if(sim.armies[i].nation==sim.player)sim.upgradeArmy(i);
        int row=0;for(const auto& a:sim.armies)if(a.nation==sim.player && row<8)ui.text(sim.unitName(sim.player,a.unit)+" · "+std::to_string(a.men),x,y+100+row++*30,.29f,pal::text);
    } else {
        int row=0;for(int id:n.charIds){const auto& ch=sim.chars[id];if(ch.alive && ch.role==1) {ui.portrait({x,y+row*95,64,64},ch,n.color,false);ui.text(ch.name+" · "+std::to_string(ch.age(sim.abs())),x+85,y+row++*95,.34f,pal::gold);}}
    }
}
} // namespace sol

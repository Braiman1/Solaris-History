// Solaris History - juego: menu, HUD, interaccion y render
#pragma once
#include "content.hpp"
#include "gfx.hpp"
#include "platform.hpp"
#include "sim.hpp"
#include "ui.hpp"
#include <string>

namespace sol {

enum class Screen { Menu, Play };
enum class Modal { None, Event, FoundNation, Buildings, Recruit, Diplomacy, Chronicle, Elections, Help, Technology, Resources, NationDetails, Negotiation, GameOver };

struct Game {
    Platform& P;
    Gfx G;
    Content C;
    Sim sim;
    UI ui;

    Screen screen = Screen::Menu;
    Modal modal = Modal::None;
    int menuSelection = 0;
    int menuPage = 0, menuScroll = 0, creditsScroll = 0;
    bool drawMedia(const std::string& id, Rect r);
    void loadGame();
    void drawLanguageFlag(Rect r, bool english);
    void drawTechnologyModal();
    int technologyTab=0, nationTab=0, diplomacyTarget=0;
    void drawNationDetails();
    void drawNegotiation();
    void drawResourcesModal();
    int selectedProvince = -1;
    int selectedArmyId = -1;
    int hoverProvince = -1;
    int modalScroll = 0;
    int chronicleScroll = 0;
    int speed = 0; // 0 pausa, 1..4
    double tickTimer = 0;
    float time = 0;
    float crashTime = 0;

    bool cameraDrag = false;
    float dragStartX = 0, dragStartY = 0, lastMx = 0, lastMy = 0;
    float dragMoved = 0;

    std::string eventTitle, eventText;
    int eventKind = 0;
    std::string status;
    float statusTimer = 0;
    bool firstPlay = true;

    Game(Platform& platform) : P(platform) {}

    bool init(const std::string& contentDir);
    void update(double dt);
    void render();

private:
    void handleKeys();
    void handlePlayInput();
    void startGame();
    void startNewGame();
    void openModal(Modal m);
    void setStatus(const std::string& s);
    int pickProvince(float mx, float my) const;
    int armyIndexById(int id) const;
    bool isPlayerArmy(int id) const;

    void drawMenu();
    void drawTopBar();
    void drawProvincePanel();
    void drawNationPanel();
    void drawLog();
    void drawMarkers();
    void drawModal();
    void drawEventModal();
    void drawFoundModal();
    void drawBuildingsModal();
    void drawRecruitModal();
    void drawDiplomacyModal();
    void drawChronicleModal();
    void drawElectionsModal();
    void drawHelpModal();
    void drawGameOverModal();

    void drawModalFrame(const std::string& title, Rect r);
    void drawArmyBadge(float x, float y, int unitIdx, int men, Color ring, bool player);
    void drawLabel(const std::string& text, float x, float y, Color c, float scale = 0.31f, bool center = true);
    std::string provinceArmySummary(int provId) const;
};

} // namespace sol

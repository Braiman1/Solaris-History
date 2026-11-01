// Solaris History - simulacion del mundo (tiempo real por meses)
#pragma once
#include "content.hpp"
#include <map>
#include <string>
#include <vector>

namespace sol {

struct Province {
    int def = 0;
    int colonizer = 0, colonyMonths = 0;
    int owner = 0;          // 0 = independiente
    float pop = 1;
    float loyalty = 60;
    float unrest = 0;
    bool occupied = false;
    std::vector<std::string> buildings;
};

struct NationMods {
    float farm = 1, growth = 1, tech = 1, trade = 1, mil = 1, stab = 1, naval = 1, expansion = 1;
    float income = 1, build = 1, legit = 0;
};

struct Nation {
    int id = 0;
    int overlord=0, subjectSince=0, decisionAt=-1000000;
    float spentTech=0;
    std::string religion, countryTag;
    std::vector<int> allies;
    std::vector<std::string> discoveries;
    std::string name, adj, culture, gov;
    Color color;
    bool isCityState = true, player = false, alive = true;
    int capital = -1;
    double gold = 140, food = 60, wood = 40, stone = 30, copper = 10;
    float stability = 60, legitimacy = 60, prestige = 10, tech = 0, manpower = 0;
    int era = 0;
    float warExhaustion = 0;
    int ruler = -1;
    std::vector<int> charIds;
    std::vector<int> wars;
    int electionAt = 0;
    int foundedAt = 0;
    float lastIncome = 0, lastFood = 0, armyMen = 0;
    float score() const;
};

struct Character {
    int id = 0, nation = 0;
    std::string name, culture;
    bool female = false, alive = true;
    int birthAbs = 0, deathAbs = 0;
    int role = 2;           // 0 gobernante, 1 consejero, 2 notable
    int office = -1;        // 0 canciller, 1 general, 2 sacerdote
    int traitA = 0, traitB = 1;
    float admin = 5, martial = 5, diplomacy = 5;
    int age(int abs) const { return (abs - birthAbs) / 12; }
    bool isRuler() const { return role == 0; }
};

struct Army {
    int id = 0, nation = 0, unit = 0;
    int men = 400;
    float morale = 1;
    int prov = 0, dest = -1;
    std::vector<int> path;
    size_t pathIdx = 0;
    float travelLeft = 0, travelTotal = 0;
    bool inTransit() const { return dest >= 0; }
};

struct LogEntry {
    int year = 0, month = 0;
    std::string text;
    int kind = 0; // 0 info, 1 bien, 2 mal, 3 guerra, 4 era
};

struct Sim {
    const Content* C = nullptr;
    int year = -3300, month = 0;
    std::vector<Province> prov;
    std::vector<Nation> nations; // indice = id (0 = independientes)
    std::vector<Character> chars;
    std::vector<Army> armies;
    std::vector<LogEntry> log;
    std::map<long long, int> warStart;
    int player = 0;
    Rng rng;
    int nextArmyId = 1;
    bool gameOver = false;

    std::string alertTitle, alertText;
    int alertKind = 0;
    bool alertPending = false;

    void init(const Content& content, int cityStateIndex);
    void step();

    int abs() const { return absMonth(year, month); }
    const ProvinceDef& pdef(int id) const { return C->province(id); }
    const Province& province(int id) const { return prov[(size_t)id]; }
    bool validProvince(int id) const { return id >= 0 && id < (int)prov.size(); }
    std::string provName(int id) const { return validProvince(id) ? pdef(id).name : "?"; }
    std::string nationName(int id) const;
    std::string nationAdj(int id) const;
    Color nationColor(int id) const;
    Color provinceMapColor(int id) const;
    bool nationAlive(int id) const { return id > 0 && id < (int)nations.size() && nations[(size_t)id].alive; }

    NationMods mods(int nation) const;
    float buildingMod(int provId, int which) const; // 0 food,1 growth,2 defense,3 wood,4 stone,5 copper,6 trade,7 stab,8 legit,9 tech,10 naval,11 manpower

    bool hasTech(int nation,const std::string& id) const;
    bool research(int nation,int tech);
    bool diplomatic(int a,int b,const std::string& action);
    bool decide(int nation,const std::string& action);
    std::string unitName(int nation,int unit) const;
    bool upgradeArmy(int army);
    bool atWar(int a, int b) const;
    void declareWar(int a, int b);
    void makePeace(int a, int b);
    int warMonths(int a, int b) const;

    bool canFound(int nation) const;
    bool foundNation(int nation, int templateIndex);
    bool canColonize(int nation, int provId, float* costOut = nullptr) const;
    bool colonize(int nation, int provId);

    bool canRecruit(int nation, int provId, int unitIdx, std::string* why = nullptr) const;
    bool recruit(int nation, int provId, int unitIdx);
    bool canBuild(int nation, int provId, int buildingIdx, std::string* why = nullptr) const;
    float buildCost(int nation, int buildingIdx, float base) const;
    bool build(int nation, int provId, int buildingIdx);

    void orderMove(int armyId, int dest);
    void cancelMove(int armyId);
    bool findPath(int armyId, int dest, std::vector<int>& path, float& cost) const;

    float armyPower(const Army& a, bool attacking) const;
    float nationArmyPower(int nation) const;
    int armiesIn(int provId) const;
    void battle(int provId, int attacker);
    void capture(int provId, int nation);

    int createCharacter(int nation, int role, int office = -1, int age = -1);
    void replaceCouncil(int nation, int office);
    void holdElection(int nation, int chosenChar);
    void processDeaths();
    void success(int nation);

    // IA
    void aiNation(int nation);
    void aiTurn();

    // eventos / alertas
    void addLog(const std::string& text, int kind = 0);
    void alert(int nation, const std::string& title, const std::string& text, int kind = 0);

    float provinceIncome(int provId, float* foodOut = nullptr) const;
    void recomputeEconomy(int nation);
    int livingRuler(int nation) const;
    int characterIndexById(int id) const { return id >= 0 && id < (int)chars.size() ? id : -1; }
    float characterCap(int nation) const;
    std::vector<int> playerProvinces() const;

    void save(const std::string& path) const;
    bool load(const std::string& path);
};

} // namespace sol

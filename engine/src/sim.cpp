#include "solaris/sim.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>

namespace sol {

// ---------------------------------------------------------------------------
// utilidades internas
// ---------------------------------------------------------------------------
static unsigned long long warKey(int a, int b) {
    int lo = std::min(a, b), hi = std::max(a, b);
    return ((unsigned long long)(unsigned)lo << 32) | (unsigned)hi;
}

static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

std::string Sim::nationName(int id) const {
    if (id <= 0 || id >= (int)nations.size()) return "Pueblos independientes";
    return nations[(size_t)id].alive ? nations[(size_t)id].name : "Reino desaparecido";
}
std::string Sim::nationAdj(int id) const {
    if (id <= 0 || id >= (int)nations.size()) return "independiente";
    return nations[(size_t)id].adj;
}
Color Sim::nationColor(int id) const {
    if (id <= 0 || id >= (int)nations.size()) return Color(130, 120, 100);
    return nations[(size_t)id].color;
}
Color Sim::provinceMapColor(int id) const {
    if (!validProvince(id)) return Color(100, 100, 100);
    const Province& p = prov[(size_t)id];
    if (p.owner > 0 && nationAlive(p.owner)) return nationColor(p.owner);
    return C->culture(pdef(id).cultureId).color;
}

// ---------------------------------------------------------------------------
// inicializacion
// ---------------------------------------------------------------------------
void Sim::init(const Content& content, int cityStateIndex) {
    C = &content;
    year = C->startYear;
    month = 0;
    rng = Rng(0x5EED3300ull + (uint64_t)cityStateIndex * 7919ull);
    prov.clear();
    nations.clear();
    chars.clear();
    armies.clear();
    log.clear();
    warStart.clear();
    nextArmyId = 1;
    gameOver = false;
    alertPending = false;
    alertTitle.clear();
    alertText.clear();

    prov.resize(C->provinces.size());
    for (size_t i = 0; i < prov.size(); ++i) {
        prov[i].def = (int)i;
        prov[i].owner = 0;
        prov[i].pop = C->provinces[i].pop;
        prov[i].loyalty = 60;
        prov[i].unrest = 0;
        prov[i].occupied = false;
    }

    // nacion 0: independientes
    nations.push_back(Nation());

    for (auto& cs : C->cityStates) {
        Nation n;
        n.id = (int)nations.size();
        n.name = cs.name;
        n.adj = cs.name;
        n.culture = cs.culture;
        n.gov = cs.gov;
        n.color = cs.color;
        n.capital = cs.capital;
        n.isCityState = true;
        n.foundedAt = abs();
        n.electionAt = C->gov(cs.gov).election > 0 ? abs() + C->gov(cs.gov).election * 12 : 0;
        n.legitimacy = C->gov(cs.gov).legit;
        n.stability = 60 + C->gov(cs.gov).stab * 100;
        nations.push_back(n);
        int nid = (int)nations.size() - 1;
        if (cs.capital >= 0 && cs.capital < (int)prov.size()) prov[(size_t)cs.capital].owner = nid;
        // personajes
        int ruler = createCharacter(nid, 0, -1, rng.irange(34, 54));
        nations[(size_t)nid].ruler = ruler;
        for (int office = 0; office < 3; ++office) createCharacter(nid, 1, office);
        for (int k = 0; k < 2; ++k) createCharacter(nid, 2);
        // guarnicion inicial
        Army a;
        a.id = nextArmyId++;
        a.nation = nid;
        a.unit = C->findUnit("soldado");
        if (a.unit < 0) a.unit = 0;
        a.men = 600;
        a.prov = cs.capital;
        a.morale = 1;
        armies.push_back(a);
        nations[(size_t)nid].manpower = 4;
    }

    player = cityStateIndex + 1;
    if (player < 1 || player >= (int)nations.size()) player = 1;
    nations[(size_t)player].player = true;

    addLog("Comienza la partida en el " + yearLabel(year) + ". " +
           nations[(size_t)player].name + " da sus primeros pasos.", 4);
    addLog("Elige un territorio vecino y funda tu nacion cuando estes listo.", 0);
}

// ---------------------------------------------------------------------------
// personajes
// ---------------------------------------------------------------------------
int Sim::createCharacter(int nation, int role, int office, int age) {
    if (nation <= 0 || nation >= (int)nations.size()) return -1;
    Nation& n = nations[(size_t)nation];
    const CultureDef& cu = C->culture(n.culture);
    Character ch;
    ch.id = (int)chars.size();
    ch.nation = nation;
    ch.culture = n.culture;
    ch.female = rng.f() < 0.14f;
    ch.role = role;
    ch.office = office;
    ch.alive = true;

    const auto& pool = C->namePools.count(n.culture) ? C->namePools.at(n.culture)
                                                     : C->namePools.begin()->second;
    const std::vector<std::string>& names = ch.female ? pool.second : pool.first;
    std::string name = names[(size_t)rng.irange(0, (int)names.size() - 1)];
    // evitar duplicados vivos
    for (int tries = 0; tries < 8; ++tries) {
        bool dup = false;
        for (auto& other : chars)
            if (other.alive && other.nation == nation && other.name == name) { dup = true; break; }
        if (!dup) break;
        name = names[(size_t)rng.irange(0, (int)names.size() - 1)];
    }
    ch.name = name;

    if (age < 0) age = rng.irange(24, 48);
    ch.birthAbs = abs() - age * 12 - rng.irange(0, 11);
    ch.traitA = rng.irange(0, (int)C->traits.size() - 1);
    do { ch.traitB = rng.irange(0, (int)C->traits.size() - 1); } while (ch.traitB == ch.traitA);
    ch.admin = (float)rng.irange(3, 9) + (role == 0 ? 1.0f : 0.0f);
    ch.martial = (float)rng.irange(3, 9);
    ch.diplomacy = (float)rng.irange(3, 9);

    float life = 52.0f + (float)rng.irange(0, 24);
    life += C->trait(ch.traitA).life + C->trait(ch.traitB).life;
    if (role == 0) life += 2;
    ch.deathAbs = ch.birthAbs + (int)(life * 12) + rng.irange(0, 11);

    chars.push_back(ch);
    nations[(size_t)nation].charIds.push_back(ch.id);
    return ch.id;
}

int Sim::livingRuler(int nation) const {
    if (nation <= 0 || nation >= (int)nations.size()) return -1;
    int r = nations[(size_t)nation].ruler;
    if (r >= 0 && r < (int)chars.size() && chars[(size_t)r].alive && chars[(size_t)r].role == 0) return r;
    return -1;
}

float Sim::characterCap(int nation) const {
    if (nation <= 0) return 0;
    float p = 0;
    for (auto& pr : prov) if (pr.owner == nation) p += pr.pop;
    return p;
}

void Sim::replaceCouncil(int nation, int office) {
    if (nation <= 0 || nation >= (int)nations.size()) return;
    Nation& n = nations[(size_t)nation];
    int oldId = -1;
    for (int cid : n.charIds) {
        Character& c = chars[(size_t)cid];
        if (c.alive && c.role == 1 && c.office == office) { oldId = cid; break; }
    }
    if (oldId >= 0) {
        chars[(size_t)oldId].role = 2;
        chars[(size_t)oldId].office = -1;
    }
    int nid = createCharacter(nation, 1, office, rng.irange(28, 50));
    if (nid >= 0 && nation == player) {
        addLog(chars[(size_t)nid].name + " es nombrado nuevo consejero.", 1);
    }
}

void Sim::holdElection(int nation, int chosenChar) {
    if (nation <= 0 || nation >= (int)nations.size()) return;
    Nation& n = nations[(size_t)nation];
    int chosen = chosenChar;
    if (chosen < 0 || chosen >= (int)chars.size() || !chars[(size_t)chosen].alive ||
        chars[(size_t)chosen].nation != nation) {
        // elegir al mejor vivo
        float best = -1;
        chosen = -1;
        for (int cid : n.charIds) {
            const Character& c = chars[(size_t)cid];
            if (!c.alive) continue;
            float v = c.admin + c.martial + c.diplomacy + (c.role == 0 ? 3.0f : 0.0f);
            if (v > best) { best = v; chosen = cid; }
        }
    }
    if (chosen < 0) {
        chosen = createCharacter(nation, 0, -1, rng.irange(32, 50));
    }
    int old = livingRuler(nation);
    if (old >= 0 && old != chosen) {
        chars[(size_t)old].role = 2;
        chars[(size_t)old].office = -1;
    }
    chars[(size_t)chosen].role = 0;
    chars[(size_t)chosen].office = -1;
    n.ruler = chosen;
    n.legitimacy = clampf(n.legitimacy + 4.0f, 0, 100);
    const GovDef& g = C->gov(n.gov);
    n.electionAt = abs() + g.election * 12;
    addLog("Eleccion en " + n.name + ": " + chars[(size_t)chosen].name + " asume el poder.", 1);
    if (nation == player) alert(nation, "Elecciones", chars[(size_t)chosen].name + " ha sido elegido gobernante de " + n.name + ".", 1);
}

void Sim::processDeaths() {
    int a = abs();
    for (auto& c : chars) {
        if (!c.alive) continue;
        bool die = a >= c.deathAbs;
        if (!die && c.age(a) > 45) {
            float p = (c.age(a) - 45) * 0.00025f;
            if (rng.f() < p) die = true;
        }
        if (!die) continue;
        c.alive = false;
        c.deathAbs = a;
        int nation = c.nation;
        if (nation <= 0 || nation >= (int)nations.size()) continue;
        Nation& n = nations[(size_t)nation];
        bool wasRuler = (c.role == 0);
        bool wasCouncil = (c.role == 1);
        if (nation == player)
            addLog(c.name + " ha muerto a los " + std::to_string(c.age(a)) + " anos.", 2);
        if (wasRuler) {
            n.ruler = -1;
            const GovDef& g = C->gov(n.gov);
            if (g.election > 0 && !n.isCityState) {
                if (nation == player) {
                    // la eleccion se abrira en la interfaz al detectar gobernante ausente
                    n.electionAt = a;
                } else {
                    holdElection(nation, -1);
                }
            } else {
                // sucesion: mejor consejero vivo
                int chosen = -1;
                float best = -1;
                for (int cid : n.charIds) {
                    const Character& o = chars[(size_t)cid];
                    if (!o.alive || o.role != 1) continue;
                    float v = o.admin * 0.5f + o.martial * 0.3f + o.diplomacy * 0.2f;
                    if (o.office == 2 && n.gov == "teocracia") v += 3;
                    if (v > best) { best = v; chosen = cid; }
                }
                if (chosen < 0) chosen = createCharacter(nation, 0, -1, rng.irange(30, 44));
                if (chosen >= 0) {
                    chars[(size_t)chosen].role = 0;
                    chars[(size_t)chosen].office = -1;
                    n.ruler = chosen;
                    n.legitimacy = clampf(n.legitimacy - 6.0f, 0, 100);
                    if (nation == player) {
                        addLog(chars[(size_t)chosen].name + " sucede en el trono de " + n.name + ".", 1);
                        alert(nation, "Sucesion", chars[(size_t)chosen].name + " hereda el gobierno de " + n.name + ".", 1);
                    }
                }
            }
        } else if (wasCouncil) {
            int office = c.office;
            if (office >= 0) {
                if (nation == player)
                    addLog(c.name + " ha muerto. Su puesto en el consejo queda vacante.", 2);
                replaceCouncil(nation, office);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// modificadores
// ---------------------------------------------------------------------------
NationMods Sim::mods(int nid) const {
    NationMods m;
    if (nid <= 0 || nid >= (int)nations.size() || !nations[(size_t)nid].alive) return m;
    const Nation& n = nations[(size_t)nid];
    const CultureDef& cu = C->culture(n.culture);
    float addIncome = 0, addMil = 0, addStab = 0, addTech = 0, addBuild = 0, addExp = 0, addLegit = 0;
    auto applyTrait = [&](const TraitDef& t, float w) {
        addIncome += t.income * w;
        addMil += t.mil * w;
        addStab += t.stab * w;
        addTech += t.tech * w;
        addBuild += t.build * w;
        addExp += t.expansion * w;
        addLegit += t.legit * w;
    };
    int r = livingRuler(nid);
    if (r >= 0) {
        const Character& c = chars[(size_t)r];
        applyTrait(C->trait(c.traitA), 1.0f);
        applyTrait(C->trait(c.traitB), 1.0f);
        addIncome += c.admin * 0.0015f;
        addMil += c.martial * 0.0015f;
        addStab += c.diplomacy * 0.001f;
    }
    int council = 0;
    for (int cid : n.charIds) {
        const Character& c = chars[(size_t)cid];
        if (!c.alive || c.role != 1 || council >= 3) continue;
        applyTrait(C->trait(c.traitA), 0.5f);
        applyTrait(C->trait(c.traitB), 0.5f);
        ++council;
    }
    m.farm = cu.farm;
    m.growth = cu.growth;
    m.tech = cu.tech * (1 + addTech);
    m.trade = cu.trade * (1 + 0.0f);
    m.mil = cu.mil * (1 + addMil);
    m.stab = cu.stab * (1 + addStab);
    m.naval = cu.naval;
    m.expansion = cu.expansion * (1 + addExp);
    m.income = 1 + addIncome;
    m.build = 1 + addBuild;
    m.legit = addLegit;
    return m;
}

float Sim::buildingMod(int provId, int which) const {
    if (!validProvince(provId)) return 0;
    float s = 0;
    for (auto& bid : prov[(size_t)provId].buildings) {
        int idx = C->findBuilding(bid);
        if (idx < 0) continue;
        const BuildingDef& b = C->building(idx);
        switch (which) {
            case 0: s += b.food; break;
            case 1: s += b.growth; break;
            case 2: s += b.defense; break;
            case 3: s += b.woodP; break;
            case 4: s += b.stoneP; break;
            case 5: s += b.copperP; break;
            case 6: s += b.trade; break;
            case 7: s += b.stability; break;
            case 8: s += b.legit; break;
            case 9: s += b.tech; break;
            case 10: s += b.naval; break;
            case 11: s += b.manpower; break;
            default: break;
        }
    }
    return s;
}

// ---------------------------------------------------------------------------
// guerra
// ---------------------------------------------------------------------------
bool Sim::atWar(int a, int b) const {
    if (a <= 0 || b <= 0 || a == b) return false;
    if (a >= (int)nations.size() || b >= (int)nations.size()) return false;
    const auto& w = nations[(size_t)a].wars;
    return std::find(w.begin(), w.end(), b) != w.end();
}

int Sim::warMonths(int a, int b) const {
    auto it = warStart.find((long long)warKey(a, b));
    if (it == warStart.end()) return 0;
    return abs() - it->second;
}

void Sim::declareWar(int a, int b) {
    if (a <= 0 || b <= 0 || a == b || !nationAlive(a) || !nationAlive(b) || atWar(a, b)) return;
    nations[(size_t)a].wars.push_back(b);
    nations[(size_t)b].wars.push_back(a);
    warStart[(long long)warKey(a, b)] = abs();
    nations[(size_t)a].stability = clampf(nations[(size_t)a].stability - 2, 0, 100);
    nations[(size_t)b].stability = clampf(nations[(size_t)b].stability - 2, 0, 100);
    addLog(nations[(size_t)a].name + " declara la guerra a " + nations[(size_t)b].name + ".", 3);
    if (a == player || b == player)
        alert(player, "Guerra", "Has entrado en guerra contra " + nationName(a == player ? b : a) + ".", 2);
}

void Sim::makePeace(int a, int b) {
    if (!atWar(a, b)) return;
    auto removeAll = [](std::vector<int>& v, int x) { v.erase(std::remove(v.begin(), v.end(), x), v.end()); };
    removeAll(nations[(size_t)a].wars, b);
    removeAll(nations[(size_t)b].wars, a);
    warStart.erase((long long)warKey(a, b));
    addLog("Paz entre " + nations[(size_t)a].name + " y " + nations[(size_t)b].name + ".", 1);
    if (a == player || b == player)
        alert(player, "Paz", "Se ha firmado la paz con " + nationName(a == player ? b : a) + ".", 1);
}

// ---------------------------------------------------------------------------
// fundacion y expansion
// ---------------------------------------------------------------------------
bool Sim::canFound(int nation) const {
    if (nation <= 0 || nation >= (int)nations.size()) return false;
    const Nation& n = nations[(size_t)nation];
    if (!n.alive || !n.isCityState) return false;
    int provinces = 0;
    for (auto& p : prov) if (p.owner == nation) ++provinces;
    return provinces >= 2 && n.gold >= 60.0 && n.prestige >= 5.0f;
}

bool Sim::foundNation(int nation, int templateIndex) {
    if (nation <= 0 || nation >= (int)nations.size()) return false;
    if (templateIndex < 0 || templateIndex >= (int)C->nationTemplates.size()) return false;
    if (!canFound(nation)) return false;
    const NationTemplate& t = C->nationTemplates[(size_t)templateIndex];
    Nation& n = nations[(size_t)nation];
    if (t.culture != n.culture) return false;
    if (t.era > n.era) return false;
    for (auto& o : nations)
        if (o.alive && o.id != nation && o.name == t.name) return false;

    n.name = t.name;
    n.adj = t.adj;
    n.color = t.color;
    n.gov = t.gov;
    n.isCityState = false;
    n.foundedAt = abs();
    const GovDef& g = C->gov(n.gov);
    n.legitimacy = clampf(n.legitimacy + 8, 0, 100);
    n.stability = clampf(n.stability + g.stab * 100, 0, 100);
    n.prestige += 10;
    n.gold -= 60;
    if (g.election > 0) n.electionAt = abs() + g.election * 12;
    else n.electionAt = 0;
    addLog("Nace " + n.name + " (" + g.name + ") bajo el gobierno de " +
           (livingRuler(nation) >= 0 ? chars[(size_t)livingRuler(nation)].name : "su pueblo") + ".", 4);
    if (nation == player) {
        alert(nation, "Nacion fundada", "Has fundado " + n.name + ". La historia de tu pueblo comienza ahora.", 1);
    }
    return true;
}

bool Sim::canColonize(int nation, int provId, float* costOut) const {
    if (nation <= 0 || !nationAlive(nation) || !validProvince(provId)) return false;
    if (prov[(size_t)provId].owner != 0) return false;
    bool adjacent = false;
    for (auto& p : prov) {
        if (p.owner != nation) continue;
        for (int a : pdef(p.def).adj) if (a == provId) { adjacent = true; break; }
        for (int a : pdef(p.def).seaAdj) if (a == provId) { adjacent = true; break; }
        if (adjacent) break;
    }
    if (!adjacent) return false;
    const Nation& n = nations[(size_t)nation];
    float cost = 45.0f;
    if (pdef(provId).cultureId != n.culture) cost *= 1.7f;
    cost *= (1.0f - (mods(nation).expansion - 1.0f) * 0.25f);
    cost = clampf(cost, 28.0f, 130.0f);
    if (costOut) *costOut = cost;
    return n.gold >= cost;
}

bool Sim::colonize(int nation, int provId) {
    float cost = 0;
    if (!canColonize(nation, provId, &cost)) return false;
    Nation& n = nations[(size_t)nation];
    n.gold -= cost;
    Province& p = prov[(size_t)provId];
    p.owner = nation;
    p.pop *= 0.88f;
    p.loyalty = 55;
    p.unrest = 8;
    addLog(n.name + " funda un asentamiento en " + pdef(provId).name + ".", 1);
    if (nation == player)
        alert(nation, "Nuevo territorio", "Has fundado un asentamiento en " + pdef(provId).name + ".", 1);
    return true;
}

// ---------------------------------------------------------------------------
// economia
// ---------------------------------------------------------------------------
float Sim::provinceIncome(int provId, float* foodOut) const {
    if (!validProvince(provId)) {
        if (foodOut) *foodOut = 0;
        return 0;
    }
    const Province& p = prov[(size_t)provId];
    if (p.owner <= 0 || !nationAlive(p.owner)) {
        if (foodOut) *foodOut = 0;
        return 0;
    }
    const ProvinceDef& pd = pdef(provId);
    const TerrainDef& t = C->terrainOfProvince(provId);
    const ResourceDef& r = C->resourceOfProvince(provId);
    const Nation& n = nations[(size_t)p.owner];
    NationMods m = mods(p.owner);
    float stabilityF = 0.65f + n.stability / 180.0f;
    float loyaltyF = 0.55f + p.loyalty / 220.0f;
    float eraF = C->era(n.era).prod;
    if (foodOut)
        *foodOut = p.pop * 0.50f * t.food * m.farm * (1 + buildingMod(provId, 0)) * eraF * stabilityF;
    float gold = (p.pop * pd.dev * 0.38f + r.value * 1.7f) *
                 (1 + buildingMod(provId, 6)) * m.trade * m.income * eraF * stabilityF * loyaltyF;
    return gold;
}

void Sim::recomputeEconomy(int nation) {
    if (nation <= 0 || nation >= (int)nations.size()) return;
    Nation& n = nations[(size_t)nation];
    double gold = 0, food = 0, wood = 0, stone = 0, copper = 0, upkeep = 0;
    float pop = 0;
    for (auto& p : prov) {
        if (p.owner != nation) continue;
        pop += p.pop;
        float f = 0;
        gold += provinceIncome(p.def, &f);
        food += f;
        const ProvinceDef& pd = pdef(p.def);
        const ResourceDef& r = C->resourceOfProvince(p.def);
        float base = 0.6f + pd.dev * 0.7f;
        wood += base * (r.kind == "wood" ? 2.3f : 1.0f) * (1 + buildingMod(p.def, 3));
        stone += base * (r.kind == "stone" ? 2.0f : 1.0f) * (1 + buildingMod(p.def, 4));
        copper += base * 0.55f * (r.kind == "copper" ? 2.4f : 1.0f) * (1 + buildingMod(p.def, 5));
    }
    float armiesMen = 0;
    for (auto& a : armies) {
        if (a.nation != nation) continue;
        armiesMen += a.men / 1000.0f;
        int u = a.unit;
        if (u >= 0 && u < (int)C->units.size())
            upkeep += C->unit(u).upkeep * (a.men / (float)C->unit(u).men);
    }
    float need = pop * 0.36f + armiesMen * 0.15f;

    n.lastIncome = (float)(gold - upkeep);
    n.lastFood = (float)(food - need);
    n.armyMen = armiesMen;
    n.gold = std::max(0.0, n.gold + gold - upkeep);
    n.food = std::max(0.0, n.food + food - need);
    n.wood = std::min(9999.0, n.wood + wood);
    n.stone = std::min(9999.0, n.stone + stone);
    n.copper = std::min(9999.0, n.copper + copper);

    // mano de obra
    float capMp = pop * 0.15f;
    n.manpower = clampf(n.manpower + capMp * 0.02f + 0.15f, 0, capMp);

    // tecnologia
    float techGain = 0;
    for (auto& p : prov) if (p.owner == nation) techGain += p.pop * 0.014f;
    techGain += 0.30f;
    NationMods m = mods(nation);
    techGain *= m.tech * (1 + buildingMod(n.capital, 9) * 0.5f);
    techGain *= (0.75f + n.stability / 250.0f);
    n.tech += techGain;

    // estabilidad y legitimidad
    float targetStab = 55.0f + n.legitimacy * 0.15f;
    targetStab += buildingMod(n.capital, 7) * 100.0f;
    if (!n.wars.empty()) targetStab -= 8.0f;
    if (n.lastFood < 0) targetStab -= 12.0f;
    n.stability += (clampf(targetStab, 5, 95) - n.stability) * 0.12f;
    n.stability = clampf(n.stability, 0, 100);

    const GovDef& g = C->gov(n.gov);
    float targetLegit = g.legit + m.legit + n.prestige * 0.05f;
    n.legitimacy += (clampf(targetLegit, 0, 100) - n.legitimacy) * 0.03f;
    n.legitimacy = clampf(n.legitimacy, 0, 100);

    // prestigio
    int provs = 0;
    for (auto& p : prov) if (p.owner == nation) ++provs;
    n.prestige += provs * 0.01f;
    if (!n.wars.empty()) n.warExhaustion += 0.25f;
    else n.warExhaustion = std::max(0.0f, n.warExhaustion - 0.4f);

    // elecciones
    if (g.election > 0 && !n.isCityState && n.electionAt > 0 && abs() >= n.electionAt) {
        if (nation == player) {
            // la interfaz abre el modal de elecciones
        } else {
            holdElection(nation, -1);
        }
    }

    // nueva era
    while (n.era + 1 < (int)C->eras.size() && n.tech >= C->eras[(size_t)n.era + 1].tech) {
        ++n.era;
        n.prestige += 10;
        addLog(n.name + " entra en la " + C->era(n.era).name + ".", 4);
        if (nation == player)
            alert(nation, "Nueva era", "Tu pueblo ha entrado en la " + C->era(n.era).name + ". Nuevas unidades y edificios disponibles.", 4);
    }

    // crecimiento de poblacion
    bool foodOk = n.lastFood >= -0.01f;
    for (auto& p : prov) {
        if (p.owner != nation) continue;
        float growth = p.pop * 0.0012f * m.growth * (1 + buildingMod(p.def, 1));
        growth *= (0.80f + n.stability / 250.0f);
        if (!foodOk) growth = -p.pop * 0.0025f;
        float cap = 6.0f + pdef(p.def).dev * 10.0f + buildingMod(p.def, 0) * 25.0f;
        p.pop = clampf(p.pop + growth, 0.4f, cap);
        // descontento y lealtad
        bool sameCulture = pdef(p.def).cultureId == n.culture;
        float unrestTarget = sameCulture ? 2.0f : 12.0f;
        if (!n.wars.empty()) unrestTarget += 4.0f;
        unrestTarget -= buildingMod(p.def, 7) * 80.0f;
        unrestTarget -= n.stability * 0.10f;
        p.unrest += (clampf(unrestTarget, 0, 60) - p.unrest) * 0.10f;
        float loyaltyTarget = sameCulture ? 90.0f : 58.0f;
        loyaltyTarget -= p.unrest * 0.5f;
        p.loyalty += (clampf(loyaltyTarget, 5, 100) - p.loyalty) * 0.012f;
        p.loyalty = clampf(p.loyalty, 0, 100);
        if (p.loyalty < 14 && p.pop > 2.5f && rng.f() < 0.004f) {
            addLog(pdef(p.def).name + " se rebela y abandona " + n.name + ".", 2);
            p.owner = 0;
            p.loyalty = 100;
            p.unrest = 0;
            if (nation == player)
                alert(nation, "Rebelion", "Has perdido " + pdef(p.def).name + " por una rebelion.", 2);
        }
    }
}

// ---------------------------------------------------------------------------
// unidades y construccion
// ---------------------------------------------------------------------------
bool Sim::canRecruit(int nation, int provId, int unitIdx, std::string* why) const {
    auto fail = [&](const std::string& s) { if (why) *why = s; return false; };
    if (nation <= 0 || !nationAlive(nation)) return fail("Nacion invalida");
    if (!validProvince(provId) || prov[(size_t)provId].owner != nation) return fail("No controlas esa provincia");
    if (unitIdx < 0 || unitIdx >= (int)C->units.size()) return fail("Unidad invalida");
    const UnitDef& u = C->unit(unitIdx);
    const Nation& n = nations[(size_t)nation];
    if (n.era < u.era) return fail("Requiere " + C->era(u.era).name);
    if (u.requires == "caballos") {
        bool horses = false;
        for (auto& p : prov) if (p.owner == nation && pdef(p.def).resourceId == "caballos") { horses = true; break; }
        if (!horses) return fail("Necesitas caballos en tu territorio");
    }
    if (u.naval) {
        if (!pdef(provId).coastal) return fail("Solo en provincias costeras");
        if (!C->hasBuilding(prov[(size_t)provId].buildings, "puerto")) return fail("Requiere un puerto");
    }
    if (prov[(size_t)provId].pop < 1.5f) return fail("Poblacion insuficiente");
    if (n.gold < u.gold) return fail("Oro insuficiente");
    if (n.food < u.food) return fail("Alimento insuficiente");
    if (n.wood < u.wood) return fail("Madera insuficiente");
    if (n.stone < u.stone) return fail("Piedra insuficiente");
    if (n.manpower < u.men / 1000.0f) return fail("Mano de obra insuficiente");
    return true;
}

bool Sim::recruit(int nation, int provId, int unitIdx) {
    if (!canRecruit(nation, provId, unitIdx)) return false;
    const UnitDef& u = C->unit(unitIdx);
    Nation& n = nations[(size_t)nation];
    n.gold -= u.gold;
    n.food -= u.food;
    n.wood -= u.wood;
    n.stone -= u.stone;
    n.manpower -= u.men / 1000.0f;
    prov[(size_t)provId].pop = std::max(0.5f, prov[(size_t)provId].pop - 0.12f);
    Army a;
    a.id = nextArmyId++;
    a.nation = nation;
    a.unit = unitIdx;
    a.men = u.men;
    a.morale = 1.0f;
    a.prov = provId;
    armies.push_back(a);
    addLog(n.name + " recluta " + u.plural + " en " + pdef(provId).name + ".", 1);
    return true;
}

float Sim::buildCost(int nation, int buildingIdx, float base) const {
    if (nation <= 0) return base;
    float m = mods(nation).build;
    return std::max(1.0f, base * m);
}

bool Sim::canBuild(int nation, int provId, int buildingIdx, std::string* why) const {
    auto fail = [&](const std::string& s) { if (why) *why = s; return false; };
    if (nation <= 0 || !nationAlive(nation)) return fail("Nacion invalida");
    if (!validProvince(provId) || prov[(size_t)provId].owner != nation) return fail("No controlas esa provincia");
    if (buildingIdx < 0 || buildingIdx >= (int)C->buildings.size()) return fail("Edificio invalido");
    const BuildingDef& b = C->building(buildingIdx);
    const Nation& n = nations[(size_t)nation];
    if (n.era < b.era) return fail("Requiere " + C->era(b.era).name);
    if (C->hasBuilding(prov[(size_t)provId].buildings, b.id)) return fail("Ya construido");
    if (n.gold < buildCost(nation, buildingIdx, b.gold)) return fail("Oro insuficiente");
    if (n.wood < buildCost(nation, buildingIdx, b.wood)) return fail("Madera insuficiente");
    if (n.stone < buildCost(nation, buildingIdx, b.stone)) return fail("Piedra insuficiente");
    return true;
}

bool Sim::build(int nation, int provId, int buildingIdx) {
    if (!canBuild(nation, provId, buildingIdx)) return false;
    const BuildingDef& b = C->building(buildingIdx);
    Nation& n = nations[(size_t)nation];
    n.gold -= buildCost(nation, buildingIdx, b.gold);
    n.wood -= buildCost(nation, buildingIdx, b.wood);
    n.stone -= buildCost(nation, buildingIdx, b.stone);
    prov[(size_t)provId].buildings.push_back(b.id);
    addLog(n.name + " construye " + b.name + " en " + pdef(provId).name + ".", 1);
    return true;
}

// ---------------------------------------------------------------------------
// ejercitos
// ---------------------------------------------------------------------------
float Sim::armyPower(const Army& a, bool attacking) const {
    if (a.unit < 0 || a.unit >= (int)C->units.size()) return 0;
    const UnitDef& u = C->unit(a.unit);
    float stat = attacking ? u.atk : (u.atk * 0.4f + u.defense * 0.8f);
    float eraF = 1.0f + (nationAlive(a.nation) ? nations[(size_t)a.nation].era * 0.06f : 0);
    float mil = nationAlive(a.nation) ? mods(a.nation).mil : 1.0f;
    return (a.men / 1000.0f) * stat * a.morale * mil * eraF;
}

float Sim::nationArmyPower(int nation) const {
    float p = 0;
    for (auto& a : armies) if (a.nation == nation) p += armyPower(a, false);
    return p;
}

int Sim::armiesIn(int provId) const {
    int n = 0;
    for (auto& a : armies) if (a.prov == provId && !a.inTransit()) ++n;
    return n;
}

static float legCost(const Content& C, int unitIdx, int from, int to, bool naval) {
    const ProvinceDef& pd = C.province(to);
    const TerrainDef& t = C.terrainOfProvince(to);
    float mult = t.move;
    bool isSea = false;
    for (int x : C.province(from).seaAdj) if (x == to) isSea = true;
    if (isSea) mult = naval ? 1.2f : 2.4f;
    const UnitDef& u = C.unit(unitIdx);
    return std::max(0.45f, mult / std::max(0.3f, u.speed));
}

bool Sim::findPath(int armyId, int destId, std::vector<int>& path, float& cost) const {
    if (armyId < 0 || armyId >= (int)armies.size()) return false;
    const Army& a = armies[(size_t)armyId];
    if (!validProvince(destId) || !validProvince(a.prov)) return false;
    if (destId == a.prov) { path.clear(); cost = 0; return true; }
    const UnitDef& u = C->unit(a.unit);
    bool naval = u.naval;
    bool hasPort = C->hasBuilding(prov[(size_t)a.prov].buildings, "puerto");

    const int N = (int)prov.size();
    const float INF = 1e9f;
    std::vector<float> dist((size_t)N, INF);
    std::vector<int> prev((size_t)N, -1);
    std::vector<bool> done((size_t)N, false);
    dist[(size_t)a.prov] = 0;
    for (int it = 0; it < N; ++it) {
        int best = -1;
        float bd = INF;
        for (int i = 0; i < N; ++i) if (!done[(size_t)i] && dist[(size_t)i] < bd) { bd = dist[(size_t)i]; best = i; }
        if (best < 0) break;
        if (best == destId) break;
        done[(size_t)best] = true;
        const ProvinceDef& pd = pdef(best);
        auto relax = [&](int to) {
            float c = legCost(*C, a.unit, best, to, naval);
            float nd = dist[(size_t)best] + c;
            if (nd < dist[(size_t)to]) { dist[(size_t)to] = nd; prev[(size_t)to] = best; }
        };
        for (int to : pd.adj) relax(to);
        if (naval || hasPort) for (int to : pd.seaAdj) relax(to);
    }
    if (dist[(size_t)destId] >= INF) return false;
    path.clear();
    int cur = destId;
    while (cur >= 0 && cur != a.prov) { path.push_back(cur); cur = prev[(size_t)cur]; }
    if (cur != a.prov) return false;
    path.push_back(a.prov);
    std::reverse(path.begin(), path.end());
    cost = dist[(size_t)destId];
    return true;
}

void Sim::orderMove(int armyId, int dest) {
    if (armyId < 0 || armyId >= (int)armies.size()) return;
    Army& a = armies[(size_t)armyId];
    if (a.inTransit()) return;
    std::vector<int> path;
    float cost = 0;
    if (!findPath(armyId, dest, path, cost)) return;
    if (path.size() < 2) return;
    a.path = path;
    a.pathIdx = 0;
    a.dest = dest;
    a.travelTotal = cost;
    a.travelLeft = legCost(*C, a.unit, path[0], path[1], C->unit(a.unit).naval);
}

void Sim::cancelMove(int armyId) {
    if (armyId < 0 || armyId >= (int)armies.size()) return;
    Army& a = armies[(size_t)armyId];
    a.dest = -1;
    a.path.clear();
    a.pathIdx = 0;
    a.travelLeft = a.travelTotal = 0;
}

void Sim::battle(int provId, int attackerNation) {
    if (!validProvince(provId) || attackerNation <= 0) return;
    Province& p = prov[(size_t)provId];
    int defenderNation = p.owner;
    if (defenderNation == attackerNation) return;
    if (defenderNation > 0 && !atWar(attackerNation, defenderNation)) return;

    const TerrainDef& terr = C->terrainOfProvince(provId);
    float atkPower = 0, defPower = 0;
    std::vector<int> attackers, defenders;
    for (int i = 0; i < (int)armies.size(); ++i) {
        Army& a = armies[(size_t)i];
        if (a.prov != provId || a.inTransit()) continue;
        if (a.nation == attackerNation) {
            atkPower += armyPower(a, true);
            attackers.push_back(i);
        } else if (defenderNation > 0 && a.nation == defenderNation) {
            defPower += armyPower(a, false);
            defenders.push_back(i);
        }
    }
    float fort = 1.0f + buildingMod(provId, 2) + (terr.defense - 1.0f);
    defPower *= std::max(1.0f, fort);
    if (defenderNation == 0) {
        defPower += p.pop * (0.55f + pdef(provId).dev * 0.18f) * (0.7f + terr.defense * 0.4f);
    }
    // los barcos no capturan provincias
    bool onlyShips = true;
    for (int i : attackers) if (!C->unit(armies[(size_t)i].unit).naval) onlyShips = false;
    if (onlyShips) return;

    float roll = rng.range(0.90f, 1.10f);
    atkPower *= roll;
    defPower *= rng.range(0.90f, 1.10f);
    if (atkPower <= 0.01f) return;
    if (defPower <= 0.01f) {
        // sin defensa: ocupacion directa
        for (int i : attackers) armies[(size_t)i].morale = clampf(armies[(size_t)i].morale + 0.05f, 0, 1.2f);
        capture(provId, attackerNation);
        return;
    }
    float ratio = atkPower / (atkPower + defPower);
    bool attackerWins = ratio > 0.5f;

    auto applyLosses = [&](std::vector<int>& list, float ratioLoss) {
        for (int i : list) {
            Army& a = armies[(size_t)i];
            float loss = ratioLoss * rng.range(0.8f, 1.2f);
            a.men = (int)(a.men * (1.0f - clampf(loss, 0.05f, 0.85f)));
            a.morale = clampf(a.morale - 0.12f, 0.15f, 1.2f);
        }
    };
    if (attackerWins) {
        applyLosses(attackers, 0.22f * (1.0f - ratio));
        applyLosses(defenders, 0.62f);
        int lost = 0;
        for (int i : defenders) lost += armies[(size_t)i].men;
        addLog("Batalla de " + pdef(provId).name + ": " + nationName(attackerNation) + " vence a " +
               (defenderNation == 0 ? std::string("las milicias locales") : nationName(defenderNation)) + ".", 3);
        if (attackerNation == player || defenderNation == player)
            alert(player, "Batalla", "Victoria en " + pdef(provId).name + ". Bajas enemigas: " + intStr(lost) + ".", 3);
        capture(provId, attackerNation);
        if (defenderNation > 0) nations[(size_t)defenderNation].warExhaustion += 4;
        nations[(size_t)attackerNation].warExhaustion += 2;
    } else {
        applyLosses(attackers, 0.58f);
        applyLosses(defenders, 0.24f * ratio);
        addLog("Batalla de " + pdef(provId).name + ": " + nationName(attackerNation) + " es rechazado.", 3);
        if (attackerNation == player || defenderNation == player)
            alert(player, "Batalla", "Derrota en " + pdef(provId).name + ". Tus tropas se retiran.", 2);
        // retirada del atacante
        for (int i : attackers) {
            Army& a = armies[(size_t)i];
            int retreat = -1;
            for (int n : pdef(a.prov).adj) if (prov[(size_t)n].owner == attackerNation) { retreat = n; break; }
            if (retreat < 0)
                for (int n : pdef(a.prov).seaAdj) if (prov[(size_t)n].owner == attackerNation) { retreat = n; break; }
            a.dest = -1;
            a.path.clear();
            if (retreat >= 0) a.prov = retreat;
            else a.men = 0;
        }
        nations[(size_t)attackerNation].warExhaustion += 3;
        if (defenderNation > 0) nations[(size_t)defenderNation].warExhaustion += 1;
    }
    // limpiar unidades destruidas
    for (auto& a : armies) if (a.men < 60) a.men = 0;
    armies.erase(std::remove_if(armies.begin(), armies.end(), [](const Army& a) { return a.men <= 0; }), armies.end());
}

void Sim::capture(int provId, int nation) {
    if (!validProvince(provId) || nation <= 0 || !nationAlive(nation)) return;
    Province& p = prov[(size_t)provId];
    int old = p.owner;
    if (old == nation) return;
    p.owner = nation;
    p.loyalty = 25;
    p.unrest = 25;
    p.pop = std::max(0.4f, p.pop * 0.90f);
    nations[(size_t)nation].prestige += 5;
    addLog(nationName(nation) + " toma " + pdef(provId).name + ".", 3);

    if (old > 0 && nationAlive(old)) {
        Nation& o = nations[(size_t)old];
        if (o.capital == provId) {
            int newCap = -1;
            for (auto& q : prov) if (q.owner == old) { newCap = q.def; break; }
            if (newCap >= 0) {
                o.capital = newCap;
                if (old == player) addLog("La capital de " + o.name + " se traslada a " + pdef(newCap).name + ".", 2);
            } else {
                o.alive = false;
                o.wars.clear();
                addLog(o.name + " ha sido destruida.", 3);
                if (old == player) {
                    alert(player, "Fin de la partida", "Tu nacion ha caido. La historia continua sin ti.", 2);
                    gameOver = true;
                }
            }
        }
        // si ya no tiene provincias
        bool any = false;
        for (auto& q : prov) if (q.owner == old) { any = true; break; }
        if (!any && o.alive) {
            o.alive = false;
            addLog(o.name + " ha sido destruida.", 3);
            if (old == player) { alert(player, "Fin de la partida", "Tu nacion ha caido.", 2); gameOver = true; }
        }
        if (!o.alive) {
            for (auto& other : nations) {
                other.wars.erase(std::remove(other.wars.begin(), other.wars.end(), old), other.wars.end());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// movimiento
// ---------------------------------------------------------------------------
static void moveArmies(Sim& sim) {
    for (auto& a : sim.armies) {
        if (!a.inTransit()) continue;
        if (a.pathIdx + 1 >= a.path.size()) { sim.cancelMove(a.id); continue; }
        a.travelLeft -= 1.0f;
        if (a.travelLeft > 0) continue;
        ++a.pathIdx;
        int next = a.path[a.pathIdx];
        a.prov = next;
        bool last = (a.pathIdx + 1 >= a.path.size());
        int owner = sim.prov[(size_t)next].owner;
        bool hostile = (owner != 0 && owner != a.nation && sim.atWar(a.nation, owner));
        bool wild = (owner == 0);
        if (hostile || wild) {
            bool naval = sim.C->unit(a.unit).naval;
            if (!naval) {
                a.dest = -1;
                a.path.clear();
                sim.battle(next, a.nation);
                continue;
            }
        }
        if (last) {
            a.dest = -1;
            a.path.clear();
            a.pathIdx = 0;
            a.travelLeft = a.travelTotal = 0;
        } else {
            a.travelLeft = legCost(*sim.C, a.unit, a.prov, a.path[a.pathIdx + 1], sim.C->unit(a.unit).naval);
        }
        if (a.men <= 0) a.men = 0;
    }
    sim.armies.erase(std::remove_if(sim.armies.begin(), sim.armies.end(), [](const Army& a) { return a.men <= 0; }), sim.armies.end());
}

// ---------------------------------------------------------------------------
// IA
// ---------------------------------------------------------------------------
void Sim::aiNation(int nation) {
    Nation& n = nations[(size_t)nation];
    if (!n.alive || n.player) return;
    NationMods m = mods(nation);

    // fundar nacion
    if (n.isCityState && n.gold >= 60 && n.prestige >= 5) {
        int provs = 0;
        for (auto& p : prov) if (p.owner == nation) ++provs;
        if (provs >= 2 && rng.f() < 0.10f) {
            std::vector<int> valid;
            for (int i = 0; i < (int)C->nationTemplates.size(); ++i) {
                const auto& t = C->nationTemplates[(size_t)i];
                if (t.culture != n.culture || t.era > n.era) continue;
                bool used = false;
                for (auto& o : nations) if (o.alive && o.id != nation && o.name == t.name) used = true;
                if (!used) valid.push_back(i);
            }
            if (!valid.empty()) foundNation(nation, valid[(size_t)rng.irange(0, (int)valid.size() - 1)]);
        }
    }

    // colonizar
    if (rng.f() < 0.10f) {
        std::vector<int> targets;
        for (auto& p : prov) {
            if (p.owner != 0) continue;
            float cost = 0;
            if (canColonize(nation, p.def, &cost) && n.gold > cost + 45) targets.push_back(p.def);
        }
        if (!targets.empty()) {
            int best = targets[(size_t)rng.irange(0, (int)targets.size() - 1)];
            colonize(nation, best);
        }
    }

    // reclutar
    float myPower = nationArmyPower(nation);
    float neighborPower = 0;
    for (auto& p : prov) {
        if (p.owner <= 0 || p.owner == nation) continue;
        bool border = false;
        for (auto& q : prov) {
            if (q.owner != nation) continue;
            for (int a : pdef(q.def).adj) if (a == p.def) border = true;
            for (int a : pdef(q.def).seaAdj) if (a == p.def) border = true;
        }
        if (border) neighborPower = std::max(neighborPower, nationArmyPower(p.owner));
    }
    if (n.gold > 70 && n.manpower > 1.2f && myPower < 8.0f + neighborPower * 0.7f && rng.f() < 0.16f) {
        int unit = C->findUnit("soldado");
        int provId = n.capital;
        std::string why;
        if (!canRecruit(nation, provId, unit, &why)) {
            for (auto& p : prov) if (p.owner == nation && canRecruit(nation, p.def, unit, &why)) { provId = p.def; break; }
        }
        if (canRecruit(nation, provId, unit, &why)) recruit(nation, provId, unit);
    }

    // guerra y paz
    if (n.wars.empty()) {
        bool grace = (abs() - absMonth(C->startYear, 0)) < 60;
        if (!grace && n.gold > 70 && rng.f() < 0.008f) {
            int target = 0;
            float bestScore = 1e9f;
            for (auto& p : prov) {
                if (p.owner <= 0 || p.owner == nation || !nationAlive(p.owner) || atWar(nation, p.owner)) continue;
                bool border = false;
                for (auto& q : prov) {
                    if (q.owner != nation) continue;
                    for (int a : pdef(q.def).adj) if (a == p.def) border = true;
                    for (int a : pdef(q.def).seaAdj) if (a == p.def) border = true;
                }
                if (!border) continue;
                float score = nationArmyPower(p.owner);
                if (score < bestScore) { bestScore = score; target = p.owner; }
            }
            if (target > 0 && myPower > bestScore * 1.45f) declareWar(nation, target);
        }
    } else {
        // paz por agotamiento
        bool longWar = false;
        for (int w : n.wars) if (warMonths(nation, w) > 24) { longWar = true; break; }
        if (longWar && n.warExhaustion > 26 && rng.f() < 0.10f) makePeace(nation, n.wars[(size_t)rng.irange(0, (int)n.wars.size() - 1)]);
    }

    // mover ejercitos
    int idle = 0;
    for (auto& a : armies) if (a.nation == nation && !a.inTransit()) ++idle;
    if (idle > 0 && !n.wars.empty()) {
        std::vector<int> targets;
        for (auto& p : prov) {
            if (p.owner <= 0 || p.owner == nation) continue;
            if (!atWar(nation, p.owner)) continue;
            // solo objetivos alcanzables desde territorio propio
            bool border = false;
            for (auto& q : prov) {
                if (q.owner != nation) continue;
                for (int a : pdef(q.def).adj) if (a == p.def) border = true;
                for (int a : pdef(q.def).seaAdj) if (a == p.def) border = true;
            }
            if (border) targets.push_back(p.def);
        }
        if (!targets.empty()) {
            for (auto& a : armies) {
                if (a.nation != nation || a.inTransit()) continue;
                int t = targets[(size_t)rng.irange(0, (int)targets.size() - 1)];
                orderMove(a.id, t);
            }
        }
    }
}

void Sim::aiTurn() {
    for (int i = 1; i < (int)nations.size(); ++i) {
        if (!nations[(size_t)i].alive || nations[(size_t)i].player) continue;
        aiNation(i);
    }
}

// ---------------------------------------------------------------------------
// eventos
// ---------------------------------------------------------------------------
void Sim::addLog(const std::string& text, int kind) {
    LogEntry e;
    e.year = year;
    e.month = month;
    e.text = text;
    e.kind = kind;
    log.push_back(e);
    if (log.size() > 500) log.erase(log.begin(), log.begin() + 100);
}

void Sim::alert(int nation, const std::string& title, const std::string& text, int kind) {
    if (nation != player) return;
    alertTitle = title;
    alertText = text;
    alertKind = kind;
    alertPending = true;
}

// ---------------------------------------------------------------------------
// paso del tiempo
// ---------------------------------------------------------------------------
void Sim::step() {
    if (gameOver) return;
    ++month;
    if (month >= 12) { month = 0; ++year; }

    for (int i = 1; i < (int)nations.size(); ++i) {
        if (!nations[(size_t)i].alive) continue;
        recomputeEconomy(i);
    }

    processDeaths();
    moveArmies(*this);

    // eventos historicos para el jugador
    if (nationAlive(player) && rng.f() < 0.012f) {
        std::vector<int> own;
        for (auto& p : prov) if (p.owner == player) own.push_back(p.def);
        if (!own.empty()) {
            int pid = own[(size_t)rng.irange(0, (int)own.size() - 1)];
            int ev = rng.irange(0, 3);
            if (ev == 0) {
                nations[(size_t)player].food = std::max(0.0, nations[(size_t)player].food - 25);
                addLog("Una sequia golpea " + pdef(pid).name + ". Alimento perdido.", 2);
            } else if (ev == 1) {
                nations[(size_t)player].food += 20;
                addLog("Excelente cosecha en " + pdef(pid).name + " (+20 alimento).", 1);
            } else if (ev == 2 && pdef(pid).terrainId == "valle") {
                nations[(size_t)player].food += 35;
                nations[(size_t)player].stability = clampf(nations[(size_t)player].stability + 2, 0, 100);
                addLog("La crecida del rio fertiliza " + pdef(pid).name + " (+35 alimento).", 1);
            } else {
                prov[(size_t)pid].pop = std::max(0.5f, prov[(size_t)pid].pop * 0.98f);
                addLog("Un temblor de tierra sacude " + pdef(pid).name + ".", 2);
            }
        }
    }

    // IA (cada dos meses para dar tiempo al jugador)
    if (abs() % 2 == 0) aiTurn();

    // elecciones pendientes del jugador
    if (nationAlive(player) && !nations[(size_t)player].isCityState) {
        const GovDef& g = C->gov(nations[(size_t)player].gov);
        if (g.election > 0 && livingRuler(player) < 0 && nations[(size_t)player].electionAt <= abs()) {
            // la interfaz mostrara el modal de elecciones
        }
    }
}

std::vector<int> Sim::playerProvinces() const {
    std::vector<int> v;
    for (auto& p : prov) if (p.owner == player) v.push_back(p.def);
    return v;
}

float Nation::score() const {
    return prestige + tech * 0.2f + era * 25.0f;
}

// ---------------------------------------------------------------------------
// guardado binario
// ---------------------------------------------------------------------------
template <typename T> static void wr(FILE* f, const T& v) { fwrite(&v, sizeof(T), 1, f); }
template <typename T> static bool rd(FILE* f, T& v) { return fread(&v, sizeof(T), 1, f) == 1; }
static void wrStr(FILE* f, const std::string& s) {
    uint32_t n = (uint32_t)s.size();
    wr(f, n);
    if (n) fwrite(s.data(), 1, n, f);
}
static bool rdStr(FILE* f, std::string& s) {
    uint32_t n = 0;
    if (!rd(f, n)) return false;
    if (n > 100000) return false;
    s.resize(n);
    if (n) return fread(&s[0], 1, n, f) == n;
    return true;
}

void Sim::save(const std::string& path) const {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return;
    const char magic[4] = {'S', 'L', 'R', 'S'};
    fwrite(magic, 1, 4, f);
    uint32_t ver = 1;
    wr(f, ver);
    int32_t y = year, mo = month, pl = player, na = nextArmyId, go = gameOver ? 1 : 0;
    wr(f, y); wr(f, mo); wr(f, pl); wr(f, na); wr(f, go);
    uint64_t seed = rng.s;
    wr(f, seed);
    uint32_t np = (uint32_t)prov.size();
    wr(f, np);
    for (auto& p : prov) {
        wr(f, p.def); wr(f, p.owner); wr(f, p.pop); wr(f, p.loyalty); wr(f, p.unrest);
        uint8_t occ = p.occupied ? 1 : 0;
        wr(f, occ);
        uint32_t nb = (uint32_t)p.buildings.size();
        wr(f, nb);
        for (auto& b : p.buildings) wrStr(f, b);
    }
    uint32_t nn = (uint32_t)nations.size();
    wr(f, nn);
    for (auto& n : nations) {
        wr(f, n.id); wrStr(f, n.name); wrStr(f, n.adj); wrStr(f, n.culture); wrStr(f, n.gov);
        wr(f, n.color); uint8_t b1 = n.isCityState, b2 = n.player, b3 = n.alive;
        wr(f, b1); wr(f, b2); wr(f, b3);
        wr(f, n.capital); wr(f, n.gold); wr(f, n.food); wr(f, n.wood); wr(f, n.stone); wr(f, n.copper);
        wr(f, n.stability); wr(f, n.legitimacy); wr(f, n.prestige); wr(f, n.tech); wr(f, n.manpower);
        wr(f, n.era); wr(f, n.warExhaustion); wr(f, n.ruler); wr(f, n.electionAt); wr(f, n.foundedAt);
        uint32_t nc = (uint32_t)n.charIds.size(); wr(f, nc);
        for (int x : n.charIds) wr(f, x);
        uint32_t nw = (uint32_t)n.wars.size(); wr(f, nw);
        for (int x : n.wars) wr(f, x);
    }
    uint32_t nch = (uint32_t)chars.size();
    wr(f, nch);
    for (auto& c : chars) {
        wr(f, c.id); wr(f, c.nation); wrStr(f, c.name); wrStr(f, c.culture);
        uint8_t fe = c.female ? 1 : 0, al = c.alive ? 1 : 0;
        wr(f, fe); wr(f, al);
        wr(f, c.birthAbs); wr(f, c.deathAbs); wr(f, c.role); wr(f, c.office);
        wr(f, c.traitA); wr(f, c.traitB); wr(f, c.admin); wr(f, c.martial); wr(f, c.diplomacy);
    }
    uint32_t nar = (uint32_t)armies.size();
    wr(f, nar);
    for (auto& a : armies) {
        wr(f, a.id); wr(f, a.nation); wr(f, a.unit); wr(f, a.men); wr(f, a.morale);
        wr(f, a.prov); wr(f, a.dest); wr(f, a.pathIdx); wr(f, a.travelLeft); wr(f, a.travelTotal);
        uint32_t psz = (uint32_t)a.path.size(); wr(f, psz);
        for (int x : a.path) wr(f, x);
    }
    uint32_t nl = (uint32_t)log.size();
    wr(f, nl);
    for (auto& e : log) {
        wr(f, e.year); wr(f, e.month); wr(f, e.kind); wrStr(f, e.text);
    }
    fclose(f);
}

bool Sim::load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "SLRS", 4) != 0) { fclose(f); return false; }
    uint32_t ver = 0;
    if (!rd(f, ver) || ver != 1) { fclose(f); return false; }
    int32_t y, mo, pl, na, go;
    if (!rd(f, y) || !rd(f, mo) || !rd(f, pl) || !rd(f, na) || !rd(f, go)) { fclose(f); return false; }
    year = y; month = mo; player = pl; nextArmyId = na; gameOver = go != 0;
    uint64_t seed; if (!rd(f, seed)) { fclose(f); return false; }
    rng.s = seed;
    uint32_t np = 0;
    if (!rd(f, np) || np > 100000) { fclose(f); return false; }
    prov.resize(np);
    for (auto& p : prov) {
        uint8_t occ = 0;
        if (!rd(f, p.def) || !rd(f, p.owner) || !rd(f, p.pop) || !rd(f, p.loyalty) || !rd(f, p.unrest) || !rd(f, occ)) { fclose(f); return false; }
        p.occupied = occ != 0;
        uint32_t nb = 0; if (!rd(f, nb) || nb > 1000) { fclose(f); return false; }
        p.buildings.resize(nb);
        for (auto& b : p.buildings) if (!rdStr(f, b)) { fclose(f); return false; }
    }
    uint32_t nn = 0;
    if (!rd(f, nn) || nn > 10000) { fclose(f); return false; }
    nations.resize(nn);
    for (auto& n : nations) {
        uint8_t b1, b2, b3;
        if (!rd(f, n.id) || !rdStr(f, n.name) || !rdStr(f, n.adj) || !rdStr(f, n.culture) || !rdStr(f, n.gov) ||
            !rd(f, n.color) || !rd(f, b1) || !rd(f, b2) || !rd(f, b3)) { fclose(f); return false; }
        n.isCityState = b1 != 0; n.player = b2 != 0; n.alive = b3 != 0;
        if (!rd(f, n.capital) || !rd(f, n.gold) || !rd(f, n.food) || !rd(f, n.wood) || !rd(f, n.stone) || !rd(f, n.copper) ||
            !rd(f, n.stability) || !rd(f, n.legitimacy) || !rd(f, n.prestige) || !rd(f, n.tech) || !rd(f, n.manpower) ||
            !rd(f, n.era) || !rd(f, n.warExhaustion) || !rd(f, n.ruler) || !rd(f, n.electionAt) || !rd(f, n.foundedAt)) { fclose(f); return false; }
        uint32_t nc = 0; if (!rd(f, nc) || nc > 10000) { fclose(f); return false; }
        n.charIds.resize(nc);
        for (auto& x : n.charIds) if (!rd(f, x)) { fclose(f); return false; }
        uint32_t nw = 0; if (!rd(f, nw) || nw > 1000) { fclose(f); return false; }
        n.wars.resize(nw);
        for (auto& x : n.wars) if (!rd(f, x)) { fclose(f); return false; }
    }
    uint32_t nch = 0;
    if (!rd(f, nch) || nch > 100000) { fclose(f); return false; }
    chars.resize(nch);
    for (auto& c : chars) {
        uint8_t fe, al;
        if (!rd(f, c.id) || !rd(f, c.nation) || !rdStr(f, c.name) || !rdStr(f, c.culture) || !rd(f, fe) || !rd(f, al) ||
            !rd(f, c.birthAbs) || !rd(f, c.deathAbs) || !rd(f, c.role) || !rd(f, c.office) || !rd(f, c.traitA) ||
            !rd(f, c.traitB) || !rd(f, c.admin) || !rd(f, c.martial) || !rd(f, c.diplomacy)) { fclose(f); return false; }
        c.female = fe != 0; c.alive = al != 0;
    }
    uint32_t nar = 0;
    if (!rd(f, nar) || nar > 100000) { fclose(f); return false; }
    armies.resize(nar);
    for (auto& a : armies) {
        uint32_t psz = 0;
        if (!rd(f, a.id) || !rd(f, a.nation) || !rd(f, a.unit) || !rd(f, a.men) || !rd(f, a.morale) ||
            !rd(f, a.prov) || !rd(f, a.dest) || !rd(f, a.pathIdx) || !rd(f, a.travelLeft) || !rd(f, a.travelTotal) ||
            !rd(f, psz) || psz > 10000) { fclose(f); return false; }
        a.path.resize(psz);
        for (auto& x : a.path) if (!rd(f, x)) { fclose(f); return false; }
    }
    uint32_t nl = 0;
    if (!rd(f, nl) || nl > 10000) { fclose(f); return false; }
    log.resize(nl);
    for (auto& e : log) {
        if (!rd(f, e.year) || !rd(f, e.month) || !rd(f, e.kind) || !rdStr(f, e.text)) { fclose(f); return false; }
    }
    fclose(f);
    alertPending = false;
    return true;
}

} // namespace sol

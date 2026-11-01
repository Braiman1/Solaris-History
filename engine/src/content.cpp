#include "solaris/content.hpp"
#include <windows.h>
#include <cstdio>

namespace sol {

static Color readColor(const Json& j, Color def = Color(180, 180, 180)) {
    if (!j.isArray() || j.size() < 3) return def;
    return Color(j[0].integer(), j[1].integer(), j[2].integer());
}

static std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char last = a.back();
    if (last == '/' || last == '\\') return a + b;
    return a + "/" + b;
}

bool Content::load(const std::string& contentDir) {
    dir = contentDir;
    auto loadJson = [&](const char* name, Json& out) {
        std::string txt = readTextFile(joinPath(dir, name));
        if (txt.empty()) {
            std::string err = "No se pudo abrir ";
            err += name;
            MessageBoxA(nullptr, err.c_str(), "Solaris History - contenido", MB_ICONERROR | MB_OK);
            return false;
        }
        std::string perr;
        out = Json::parse(txt, &perr);
        if (out.isNull() && !perr.empty()) {
            std::string err = std::string("Error JSON en ") + name + ": " + perr;
            MessageBoxA(nullptr, err.c_str(), "Solaris History - contenido", MB_ICONERROR | MB_OK);
            return false;
        }
        return true;
    };

    Json jWorld, jCult, jUnits, jBuildings, jEras, jLeaders, jNations;
    if (!loadJson("world.json", jWorld)) return false;
    if (!loadJson("cultures.json", jCult)) return false;
    if (!loadJson("units.json", jUnits)) return false;
    if (!loadJson("buildings.json", jBuildings)) return false;
    if (!loadJson("eras.json", jEras)) return false;
    if (!loadJson("leaders.json", jLeaders)) return false;
    if (!loadJson("nations.json", jNations)) return false;

    startYear = jWorld["meta"]["start_year"].integer(-3300);

    // Los objetos de terreno/recurso se leen mediante la lista de claves conocidas.
    {
        Json jt = jWorld["terrains"];
        // claves conocidas usadas por el contenido
        const char* terr[] = {"valle", "llanura", "colina", "montana", "desierto", "estepa",
                              "bosque", "costa", "isla", "oasis", "marisma"};
        for (auto* id : terr) {
            const Json& t = jt[id];
            if (t.isNull()) continue;
            TerrainDef d;
            d.id = id;
            d.name = t["name"].string(id);
            d.food = (float)t["food"].number(1);
            d.defense = (float)t["defense"].number(1);
            d.move = (float)t["move"].number(1);
            d.color = readColor(t["color"]);
            terrainIndex[d.id] = (int)terrains.size();
            terrains.push_back(d);
        }
    }
    {
        Json jr = jWorld["resources"];
        const char* res[] = {"trigo", "cebada", "pescado", "ganado", "madera", "cedro", "piedra",
                             "cobre", "estano", "oro", "plata", "sal", "obsidiana", "vino",
                             "aceite", "lino", "incienso", "caballos"};
        for (auto* id : res) {
            const Json& r = jr[id];
            if (r.isNull()) continue;
            ResourceDef d;
            d.id = id;
            d.name = r["name"].string(id);
            d.kind = r["kind"].string("trade");
            d.value = (float)r["value"].number(1);
            d.color = readColor(r["color"]);
            resourceIndex[d.id] = (int)resources.size();
            resources.push_back(d);
        }
    }

    for (auto& j : jCult.array()) {
        CultureDef d;
        d.id = j["id"].string();
        d.name = j["name"].string(d.id);
        d.shortName = j["short"].string(d.name);
        d.desc = j["desc"].string();
        d.color = readColor(j["color"]);
        const Json& m = j["mods"];
        d.farm = (float)m["farm"].number(1);
        d.growth = (float)m["growth"].number(1);
        d.tech = (float)m["tech"].number(1);
        d.trade = (float)m["trade"].number(1);
        d.mil = (float)m["mil"].number(1);
        d.stab = (float)m["stab"].number(1);
        d.naval = (float)m["naval"].number(1);
        d.expansion = (float)m["expansion"].number(1);
        cultureIndex[d.id] = (int)cultures.size();
        cultures.push_back(d);
    }

    for (auto& j : jUnits.array()) {
        UnitDef d;
        d.id = j["id"].string();
        d.name = j["name"].string(d.id);
        d.plural = j["plural"].string(d.name);
        d.icon = j["icon"].string(d.id);
        d.requires = j["requires"].string();
        d.era = j["era"].integer(0);
        d.men = j["men"].integer(800);
        d.gold = (float)j["gold"].number(25);
        d.food = (float)j["food"].number(0);
        d.wood = (float)j["wood"].number(0);
        d.stone = (float)j["stone"].number(0);
        d.atk = (float)j["atk"].number(10);
        d.defense = (float)j["defense"].number(10);
        d.upkeep = (float)j["upkeep"].number(1);
        d.speed = (float)j["speed"].number(1);
        d.naval = j["naval"].boolean(false);
        d.desc = j["desc"].string();
        unitIndex[d.id] = (int)units.size();
        units.push_back(d);
    }

    for (auto& j : jBuildings.array()) {
        BuildingDef d;
        d.id = j["id"].string();
        d.name = j["name"].string(d.id);
        d.desc = j["desc"].string();
        d.era = j["era"].integer(0);
        const Json& c = j["cost"];
        d.gold = (float)c["gold"].number(0);
        d.wood = (float)c["wood"].number(0);
        d.stone = (float)c["stone"].number(0);
        const Json& m = j["mods"];
        d.food = (float)m["food"].number(0);
        d.growth = (float)m["growth"].number(0);
        d.defense = (float)m["defense"].number(0);
        d.woodP = (float)m["wood"].number(0);
        d.stoneP = (float)m["stone"].number(0);
        d.copperP = (float)m["copper"].number(0);
        d.trade = (float)m["trade"].number(0);
        d.stability = (float)m["stability"].number(0);
        d.legit = (float)m["legit"].number(0);
        d.tech = (float)m["tech"].number(0);
        d.naval = (float)m["naval"].number(0);
        d.manpower = (float)m["manpower"].number(0);
        buildingIndex[d.id] = (int)buildings.size();
        buildings.push_back(d);
    }

    for (auto& j : jEras.array()) {
        EraDef d;
        d.id = j["id"].integer(0);
        d.name = j["name"].string();
        d.shortName = j["short"].string(d.name);
        d.desc = j["desc"].string();
        d.tech = (float)j["tech"].number(0);
        d.prod = (float)j["prod"].number(1);
        eras.push_back(d);
    }

    const Json& govsJson = jLeaders["governments"];
    const char* govIds[] = {"teocracia", "monarquia", "republica"};
    for (auto* id : govIds) {
        const Json& g = govsJson[id];
        if (g.isNull()) continue;
        GovDef d;
        d.id = id;
        d.name = g["name"].string(id);
        d.desc = g["desc"].string();
        d.legit = (float)g["legit"].number(60);
        d.stab = (float)g["stab"].number(0);
        d.election = g["election"].integer(0);
        govIndex[d.id] = (int)govs.size();
        govs.push_back(d);
    }

    for (auto& j : jLeaders["traits"].array()) {
        TraitDef d;
        d.id = j["id"].string();
        d.name = j["name"].string(d.id);
        d.desc = j["desc"].string();
        const Json& m = j["mods"];
        d.income = (float)m["income"].number(0);
        d.mil = (float)m["mil"].number(0);
        d.stab = (float)m["stab"].number(0);
        d.legit = (float)m["legit"].number(0);
        d.expansion = (float)m["expansion"].number(0);
        d.tech = (float)m["tech"].number(0);
        d.build = (float)m["build"].number(0);
        d.life = (float)m["life"].number(0);
        traitIndex[d.id] = (int)traits.size();
        traits.push_back(d);
    }

    for (auto& j : jNations.array()) {
        NationTemplate d;
        d.id = j["id"].string();
        d.name = j["name"].string(d.id);
        d.culture = j["culture"].string();
        d.capitalName = j["capital"].string();
        d.gov = j["gov"].string("monarquia");
        d.adj = j["adj"].string(d.name);
        d.desc = j["desc"].string();
        d.era = j["era"].integer(0);
        d.color = readColor(j["color"]);
        nationTemplates.push_back(d);
    }

    for (auto& j : jWorld["city_states"].array()) {
        CityStateDef d;
        d.id = j["id"].integer();
        d.name = j["name"].string();
        d.culture = j["culture"].string();
        d.gov = j["gov"].string("monarquia");
        d.desc = j["desc"].string();
        d.capital = j["capital"].integer();
        d.playable = j["playable"].boolean(false);
        d.color = readColor(j["color"]);
        cityStates.push_back(d);
    }

    for (auto& j : jWorld["provinces"].array()) {
        ProvinceDef d;
        d.id = j["id"].integer();
        d.name = j["name"].string();
        d.region = j["region"].string();
        d.terrainId = j["terrain"].string("llanura");
        d.resourceId = j["resource"].string("trigo");
        d.cultureId = j["culture"].string("semita");
        d.lat = (float)j["lat"].number();
        d.lon = (float)j["lon"].number();
        d.pop = (float)j["pop"].number(1);
        d.dev = j["dev"].integer(1);
        d.coastal = j["coastal"].boolean(false);
        for (auto& k : j["adj"].array()) d.adj.push_back(k.integer());
        for (auto& k : j["sea_adj"].array()) d.seaAdj.push_back(k.integer());
        provinces.push_back(d);
    }
    // ordenar por id y comprobar
    std::sort(provinces.begin(), provinces.end(), [](const ProvinceDef& a, const ProvinceDef& b) { return a.id < b.id; });
    for (size_t i = 0; i < provinces.size(); ++i) {
        if (provinces[i].id != (int)i) {
            MessageBoxA(nullptr, "Los ids de provincia no son consecutivos", "Solaris History", MB_ICONERROR);
            return false;
        }
    }

    // nombres por cultura
    for (auto& c : cultures) {
        const Json& pool = jLeaders["names"][c.id];
        auto& dest = namePools[c.id];
        if (pool.isArray() && pool.size() >= 2) {
            for (auto& n : pool[0].array()) dest.first.push_back(n.string());
            for (auto& n : pool[1].array()) dest.second.push_back(n.string());
        }
        if (dest.first.empty()) dest.first.push_back("Ari");
        if (dest.second.empty()) dest.second.push_back("Ana");
    }
    return true;
}

const CultureDef& Content::culture(const std::string& id) const {
    auto it = cultureIndex.find(id);
    if (it != cultureIndex.end()) return cultures[(size_t)it->second];
    return cultures[0];
}
const CultureDef& Content::cultureOfProvince(int provinceId) const {
    return culture(provinces[(size_t)provinceId].cultureId);
}
const TerrainDef& Content::terrainOfProvince(int provinceId) const {
    auto it = terrainIndex.find(provinces[(size_t)provinceId].terrainId);
    if (it != terrainIndex.end()) return terrains[(size_t)it->second];
    return terrains[0];
}
const ResourceDef& Content::resourceOfProvince(int provinceId) const {
    auto it = resourceIndex.find(provinces[(size_t)provinceId].resourceId);
    if (it != resourceIndex.end()) return resources[(size_t)it->second];
    return resources[0];
}
const GovDef& Content::gov(const std::string& id) const {
    auto it = govIndex.find(id);
    if (it != govIndex.end()) return govs[(size_t)it->second];
    return govs[0];
}
int Content::findUnit(const std::string& id) const {
    auto it = unitIndex.find(id);
    return it == unitIndex.end() ? -1 : it->second;
}
int Content::findBuilding(const std::string& id) const {
    auto it = buildingIndex.find(id);
    return it == buildingIndex.end() ? -1 : it->second;
}
int Content::findProvince(const std::string& name) const {
    for (auto& p : provinces) if (p.name == name) return p.id;
    return -1;
}
bool Content::hasBuilding(const std::vector<std::string>& list, const std::string& id) const {
    for (auto& b : list) if (b == id) return true;
    return false;
}

std::string findContentDir() {
    std::string exe = ".";
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n > 0) {
        exe = buf;
        size_t pos = exe.find_last_of("\\/");
        if (pos != std::string::npos) exe = exe.substr(0, pos);
    }
    const char* rel[] = {"/content", "/../content", "/../../content", "/../../../content"};
    for (auto* r : rel) {
        std::string p = exe + r;
        if (!readTextFile(p + "/world.json").empty()) return p;
    }
    if (!readTextFile("content/world.json").empty()) return "content";
    return exe + "/content";
}

} // namespace sol

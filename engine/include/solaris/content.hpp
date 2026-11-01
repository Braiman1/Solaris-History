// Solaris History - definiciones de contenido historico
#pragma once
#include "core.hpp"
#include "json.hpp"
#include <map>
#include <string>
#include <vector>

namespace sol {

struct TerrainDef {
    std::string id, name;
    float food = 1, defense = 1, move = 1;
    Color color;
};

struct ResourceDef {
    std::string id, name, kind;
    float value = 1;
    Color color;
};

struct CultureDef {
    std::string id, name, shortName, desc;
    Color color;
    float farm = 1, growth = 1, tech = 1, trade = 1, mil = 1, stab = 1, naval = 1, expansion = 1;
};

struct UnitDef {
    std::string id, name, plural, icon, requires;
    int era = 0, men = 800;
    float gold = 25, food = 10, wood = 0, stone = 0;
    float atk = 10, defense = 10, upkeep = 1, speed = 1;
    bool naval = false;
    std::string desc;
};

struct BuildingDef {
    std::string id, name, desc;
    int era = 0;
    float gold = 0, wood = 0, stone = 0;
    float food = 0, growth = 0, defense = 0, woodP = 0, stoneP = 0, copperP = 0;
    float trade = 0, stability = 0, legit = 0, tech = 0, naval = 0, manpower = 0;
};

struct EraDef {
    int id = 0;
    std::string name, shortName, desc;
    float tech = 0, prod = 1;
};

struct GovDef {
    std::string id, name, desc;
    float legit = 60, stab = 0;
    int election = 0;
};

struct TraitDef {
    std::string id, name, desc;
    float income = 0, mil = 0, stab = 0, legit = 0, expansion = 0, tech = 0, build = 0, life = 0;
};

struct NationTemplate {
    std::string id, name, culture, capitalName, gov, adj, desc;
    int era = 0;
    Color color;
};

struct CityStateDef {
    int id = 0;
    std::string name, culture, gov, desc;
    int capital = 0;
    bool playable = false;
    Color color;
};

struct ProvinceDef {
    int id = 0;
    std::string name, region, terrainId, resourceId, cultureId;
    float lat = 0, lon = 0, pop = 1;
    int dev = 1;
    bool coastal = false;
    std::vector<int> adj, seaAdj;
};

struct Content {
    std::string dir;
    int startYear = -3300;
    std::map<std::string, int> terrainIndex, resourceIndex, cultureIndex, unitIndex,
                               buildingIndex, govIndex, traitIndex;
    std::vector<TerrainDef> terrains;
    std::vector<ResourceDef> resources;
    std::vector<CultureDef> cultures;
    std::vector<UnitDef> units;
    std::vector<BuildingDef> buildings;
    std::vector<EraDef> eras;
    std::vector<GovDef> govs;
    std::vector<TraitDef> traits;
    std::vector<NationTemplate> nationTemplates;
    std::vector<CityStateDef> cityStates;
    std::vector<ProvinceDef> provinces;
    std::map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>> namePools;

    bool load(const std::string& contentDir);

    const ProvinceDef& province(int id) const { return provinces[(size_t)id]; }
    const CultureDef& culture(const std::string& id) const;
    const CultureDef& cultureOfProvince(int provinceId) const;
    const TerrainDef& terrainOfProvince(int provinceId) const;
    const ResourceDef& resourceOfProvince(int provinceId) const;
    const UnitDef& unit(int idx) const { return units[(size_t)idx]; }
    const BuildingDef& building(int idx) const { return buildings[(size_t)idx]; }
    const EraDef& era(int idx) const { return eras[(size_t)std::clamp(idx, 0, (int)eras.size() - 1)]; }
    const GovDef& gov(const std::string& id) const;
    const TraitDef& trait(int idx) const { return traits[(size_t)std::clamp(idx, 0, (int)traits.size() - 1)]; }
    int findUnit(const std::string& id) const;
    int findBuilding(const std::string& id) const;
    int findProvince(const std::string& name) const;
    bool hasBuilding(const std::vector<std::string>& list, const std::string& id) const;
};

std::string findContentDir();

} // namespace sol

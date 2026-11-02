#include "solaris/sim.hpp"
#include <algorithm>
#include <queue>
#include <cmath>

namespace sol {
namespace {
long long contactKey(int a,int b) { if(a>b)std::swap(a,b);return ((long long)a<<32)|(unsigned)b; }
float distanceKm(const ProvinceDef& a,const ProvinceDef& b) {
    float longitude=std::remainder(b.lon-a.lon,360.f)*DEG;
    return 6371*std::hypot(longitude*std::cos((a.lat+b.lat)*.5f*DEG),(b.lat-a.lat)*DEG);
}
}
bool Sim::knowsProvince(int nation,int province) const {
    if(nation<=0||nation>=(int)nations.size()||!validProvince(province))return false;
    const auto& known=nations[nation].knownProvinces;
    return prov[province].owner==nation || ((size_t)province<known.size() && known[province]!=0);
}
bool Sim::knowsNation(int nation,int other) const {
    if(nation<=0||nation>=(int)nations.size()||other<=0||other>=(int)nations.size())return false;
    if(nation==other)return true;
    for(const auto& p:prov)if(p.owner==other && knowsProvince(nation,p.def))return true;
    return false;
}
int Sim::knownCount(int nation) const {
    int count=0;for(const auto& p:prov)if(knowsProvince(nation,p.def))++count;return count;
}
void Sim::revealArea(int nation,int center,float radius) {
    if(!nationAlive(nation)||!validProvince(center))return;
    auto& known=nations[nation].knownProvinces;known.resize(prov.size(),0);
    for(const auto& p:C->provinces)if(p.id==center || distanceKm(pdef(center),p)<=radius)known[p.id]=1;
}
void Sim::initializeKnowledge() {
    for(auto& n:nations) {
        n.knownProvinces.assign(prov.size(),0);
        if(!nationAlive(n.id))continue;
        revealArea(n.id,n.capital,350);
        // Previously established diplomatic ties remain contacts when old saves migrate.
        for(int ally:n.allies)if(nationAlive(ally))revealArea(n.id,nations[ally].capital,0);
        for(int enemy:n.wars)if(nationAlive(enemy))revealArea(n.id,nations[enemy].capital,0);
    }
    observeTerritory();
}
void Sim::observeTerritory() {
    for(const auto& p:prov)if(nationAlive(p.owner)) {
        auto& known=nations[p.owner].knownProvinces;known.resize(prov.size(),0);known[p.def]=1;
        // Owning land reveals its immediate frontier, not a complete continent.
        for(int id:pdef(p.def).adj)known[id]=1;
        for(int id:pdef(p.def).seaAdj)known[id]=1;
    }
    for(const auto& a:armies)if(a.men>0&&nationAlive(a.nation))revealArea(a.nation,a.prov,100);
}
int Sim::relation(int a,int b) const {
    if(!nationAlive(a)||!nationAlive(b))return -100;
    if(a==b)return 100;
    if(atWar(a,b))return -100;
    auto found=relations.find(contactKey(a,b));return found==relations.end()?25:found->second;
}
int Sim::nextExplorationStep(const Explorer& e) const {
    if(!nationAlive(e.nation)||!validProvince(e.province))return -1;
    std::vector<int> parent(prov.size(),-1);std::queue<int> pending;pending.push(e.province);parent[e.province]=e.province;
    int goal=-1;
    while(!pending.empty()&&goal<0) {
        int current=pending.front();pending.pop();
        const auto& edges=e.naval?pdef(current).exploreSeaAdj:pdef(current).adj;
        for(int to:edges) {
            if(!validProvince(to)||parent[to]>=0||(prov[to].owner && atWar(e.nation,prov[to].owner)))continue;
            if(e.naval&&!pdef(to).coastal)continue;
            parent[to]=current;
            if(!knowsProvince(e.nation,to)){goal=to;break;}
            pending.push(to);
        }
    }
    if(goal<0)return -1;
    while(parent[goal]!=e.province)goal=parent[goal];
    return goal;
}
bool Sim::canExplore(int nation,int source,bool naval) const {
    if(!nationAlive(nation)||!validProvince(source)||prov[source].owner!=nation)return false;
    const auto& n=nations[nation];
    for(const auto& e:explorers)if(e.nation==nation&&e.naval==naval)return false;
    if(n.gold<(naval?80:40)||n.food<(naval?20:10))return false;
    if(naval && (!pdef(source).coastal||!hasTech(nation,"vela")||!C->hasBuilding(prov[source].buildings,"puerto")||n.wood<20))return false;
    Explorer e;e.nation=nation;e.province=source;e.naval=naval;
    return nextExplorationStep(e)>=0;
}
bool Sim::explore(int nation,int source,bool naval) {
    if(!canExplore(nation,source,naval))return false;
    auto& n=nations[nation];n.gold-=naval?80:40;n.food-=naval?20:10;if(naval)n.wood-=20;
    Explorer e;e.nation=nation;e.province=source;e.naval=naval;explorers.push_back(e);
    if(nation==player)addLog(naval?"Parte una expedicion maritima de exploracion.":"Parte una expedicion terrestre de exploracion.",1);
    return true;
}
bool Sim::exchangeMaps(int receiver,int giver) {
    if(!nationAlive(receiver)||!nationAlive(giver)||receiver==giver||(!knowsNation(receiver,giver)&&!knowsNation(giver,receiver))||atWar(receiver,giver)||relation(receiver,giver)<60)return false;
    auto& known=nations[receiver].knownProvinces;known.resize(prov.size(),0);
    bool changed=false;
    for(const auto& p:prov)if(knowsProvince(giver,p.def)&&!knowsProvince(receiver,p.def)){known[p.def]=1;changed=true;}
    if(changed&&receiver==player)addLog("Llegan nuevos mapas diplomaticos.",1);
    return changed;
}
void Sim::explorationTick() {
    for(auto& e:explorers) {
        if(!nationAlive(e.nation)){e.monthsLeft=0;continue;}
        if(--e.monthsLeft<0)continue;
        if(e.target>=0 && prov[e.target].owner && atWar(e.nation,prov[e.target].owner)){e.target=-1;e.travelLeft=0;}
        if(e.target>=0 && --e.travelLeft<=0) {
            e.province=e.target;e.target=-1;
            bool discovered=!knowsProvince(e.nation,e.province);
            revealArea(e.nation,e.province,e.naval?75:100);
            if(discovered&&e.nation==player)addLog("Exploradores descubren "+pdef(e.province).name+".",1);
        }
        if(e.monthsLeft==0) {
            if(e.nation==player)addLog("La expedicion de exploracion ha regresado.",1);
            continue;
        }
        if(e.target<0) {
            e.target=nextExplorationStep(e);
            if(e.target<0){e.monthsLeft=0;if(e.nation==player)addLog("No quedan rutas de exploracion accesibles.",0);continue;}
            e.travelLeft=e.naval?2:std::clamp((int)std::ceil(C->terrainOfProvince(e.target).move*C->biomeOfProvince(e.target).move),1,4);
        }
    }
    explorers.erase(std::remove_if(explorers.begin(),explorers.end(),[](const Explorer& e){return e.monthsLeft<=0;}),explorers.end());
    // The AI learns by the same routes and diplomatic rules as the player.
    for(int i=1;i<(int)nations.size();++i)if(nationAlive(i)&&i!=player&&nations[i].gold>250) {
        if(rng.f()<.025f)explore(i,nations[i].capital,false);
        if(rng.f()<.01f)for(const auto& p:prov)if(p.owner==i&&explore(i,p.def,true))break;
        if(rng.f()<.02f)for(int j=1;j<(int)nations.size();++j)if(i!=j&&nationAlive(j)&&knowsNation(i,j)&&!atWar(i,j)) {
            if(relation(i,j)>=60)exchangeMaps(i,j);else diplomatic(i,j,"improve");break;
        }
    }
}
} // namespace sol

#include "solaris/sim.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>
using namespace sol;
int main() {
    Content c;if(!c.load(findContentDir()))return 1;
    int count=0,fail=0;auto ck=[&](bool ok,const char* label){++count;if(!ok){++fail;std::printf("FAIL %s\n",label);}};
    ck(c.provinces.size()==515&&c.cityStates.size()==46,"map expansion");ck(c.biomes.size()==9&&c.cultures.size()==27,"biomes and cultures");
    bool refs=true;
    for(const auto& p:c.provinces) {
        refs&=c.biomeIndex.count(p.biomeId)&&!p.continent.empty();
        for(int other:p.adj)refs&=other>=0&&other<(int)c.provinces.size()&&std::find(c.province(other).adj.begin(),c.province(other).adj.end(),p.id)!=c.province(other).adj.end();
    }
    ck(refs,"province biome and reciprocal adjacency references");
    Vec3 start=lunarPosition(0),quarter=lunarPosition(22.5f),full=lunarPosition(90);
    ck(std::fabs(length(start)-1.65f)<.0001f&&std::fabs(length(quarter)-1.65f)<.0001f,"constant lunar orbital radius");
    ck(length(start-quarter)>2 && length(start-full)<.001f,"Moon revolves around Earth in world space");
    Sim s;s.init(c,0);int a=s.player,cap=s.nations[a].capital,balkh=c.findProvince("Balkh");
    ck(s.knowsProvince(a,cap)&&s.knownCount(a)<(int)c.provinces.size()/4,"initial map is local");ck(!s.knowsProvince(a,balkh),"another continent is hidden");
    int distant=0;for(const auto& n:s.nations)if(n.capital==balkh)distant=n.id;
    ck(distant>0&&!s.knowsNation(a,distant),"undiscovered tribe is not a diplomatic contact");
    ck(!s.diplomatic(a,distant,"alliance"),"cannot negotiate with hidden nations");
    auto edges=c.provinces[cap].adj;c.provinces[cap].adj.push_back(balkh);std::vector<int> path;float cost=0;
    ck(!s.findPath(0,balkh,path,cost),"armies cannot target unknown land even through a forged connection");c.provinces[cap].adj=edges;
    size_t events=s.log.size();s.addLog("Un evento en Bactria",1,distant,balkh);ck(s.log.size()==events,"hidden countries and provinces do not leak through event history");
    s.addLog("Un evento local",1,a,cap);ck(s.log.size()==events+1,"local history remains visible");
    Sim news;news.init(c,0);events=news.log.size();news.nations[2].gov="republica";news.holdElection(2,-1);
    ck(news.log.size()==events,"foreign elections do not reveal unknown countries");
    news.year=-3000;news.nations[2].tech=3;news.nations[2].gold=1000;news.nations[2].prestige=100;
    int assigned=1;for(auto& p:news.prov)if(!p.owner && c.province(p.def).cultureId==news.nations[2].culture && assigned<4){p.owner=2;++assigned;}
    bool formed=false;for(int i=0;i<(int)c.nationTemplates.size();++i)if(news.foundNation(2,i)){formed=true;break;}
    ck(formed&&news.log.size()==events,"foreign foundations do not reveal unknown countries");
    s.nations[a].gold=1000;s.nations[a].food=1000;
    int known=s.knownCount(a);double gold=s.nations[a].gold;
    ck(s.explore(a,cap,false),"land expedition starts");ck(s.nations[a].gold==gold-40&&s.nations[a].food==990,"land exploration costs paid");
    ck(!s.explore(a,cap,false),"only one land expedition per nation");
    std::vector<int> owners;for(const auto& p:s.prov)owners.push_back(p.owner);
    for(int i=0;i<16;++i)s.explorationTick();
    ck(s.knownCount(a)>known,"explorers reveal new provinces over time");
    bool unchanged=true;for(size_t i=0;i<s.prov.size();++i)unchanged&=s.prov[i].owner==owners[i];ck(unchanged,"exploration never colonizes or conquers");
    ck(s.explorers.size()>0,"expedition remains active before thirty six months");
    s.save("exploration_test.sav");Sim loaded;loaded.C=&c;ck(loaded.load("exploration_test.sav"),"new save loads");
    ck(loaded.knownCount(a)==s.knownCount(a)&&loaded.explorers.size()==s.explorers.size(),"maps and active expeditions persist");
    for(int i=16;i<36;++i)s.explorationTick();bool active=false;for(const auto& e:s.explorers)active|=e.nation==a&&!e.naval;ck(!active,"expedition ends after thirty six months");
    std::remove("exploration_test.sav");
    Sim maps;maps.init(c,0);int b=2;
    maps.revealArea(a,maps.nations[b].capital,0);maps.revealArea(b,maps.nations[a].capital,0);maps.revealArea(b,balkh,0);
    maps.nations[a].gold=1000;ck(!maps.exchangeMaps(a,b),"poor relations block map sharing");
    ck(maps.diplomatic(a,b,"improve"),"relations can be improved");ck(!maps.diplomatic(a,b,"improve"),"annual diplomatic cooldown");maps.year++;
    ck(maps.diplomatic(a,b,"improve"),"second diplomatic mission");maps.year++;ck(maps.diplomatic(a,b,"improve"),"third mission reaches good relations");
    ck(maps.relation(a,b)==70,"relationship value is symmetric and bounded");ck(!maps.knowsProvince(a,balkh),"diplomatic contact alone does not reveal maps");
    gold=maps.nations[a].gold;ck(maps.diplomatic(a,b,"request_maps"),"request maps with good relations");ck(maps.knowsProvince(a,balkh)&&maps.nations[a].gold==gold-25,"received map reveals only donor discoveries and costs gold");
    int unknown=c.findProvince("Afanasievo");ck(!maps.knowsProvince(a,unknown),"map sharing does not reveal the whole world");
    maps.revealArea(a,unknown,0);ck(maps.diplomatic(a,b,"share_maps")&&maps.knowsProvince(b,unknown),"give maps to a friendly nation");
    maps.save("exploration_maps.sav");Sim m;m.C=&c;ck(m.load("exploration_maps.sav")&&m.relation(a,b)==70,"relations persist");ck(!m.diplomatic(a,b,"improve"),"diplomatic cooldown persists");std::remove("exploration_maps.sav");
    maps.declareWar(a,b);ck(!maps.exchangeMaps(a,b)&&maps.relation(a,b)==-100,"war blocks map sharing");
    Sim sea;int sailor=5;sea.init(c,sailor);a=sea.player;cap=sea.nations[a].capital;sea.nations[a].gold=1000;sea.nations[a].food=1000;
    ck(!sea.explore(a,cap,true),"sea exploration requires navigation and port");
    int port=-1;for(const auto& p:sea.prov)if(c.province(p.def).coastal&&!c.province(p.def).exploreSeaAdj.empty()){port=p.def;break;}
    ck(port>=0,"coastal exploration routes exist");sea.prov[port].owner=a;sea.prov[port].buildings.push_back("puerto");sea.nations[a].discoveries.push_back("vela");sea.nations[a].knownProvinces.assign(c.provinces.size(),0);sea.nations[a].knownProvinces[port]=1;
    ck(sea.explore(a,port,true),"navigation and port enable sea expedition");known=sea.knownCount(a);for(int i=0;i<3;++i)sea.explorationTick();ck(sea.knownCount(a)>known,"sea explorer discovers coastal provinces");
    Sim daily;daily.init(c,0);daily.year=-300;daily.nations[1].gold=1000;daily.nations[1].food=1000;
    ck(daily.explore(1,daily.nations[1].capital,false),"daily-calendar expedition starts");
    daily.step();ck(daily.explorers.front().monthsLeft==36,"daily calendar does not accelerate exploration");
    for(int i=1;i<31;++i)daily.step();ck(daily.explorers.front().monthsLeft==35,"exploration advances at month boundary");
    auto e=sea.explorers.front();ck(c.province(e.province).coastal,"sea exploration remains on coastal routes");
    Sim biome;biome.init(c,0);a=biome.player;cap=biome.nations[a].capital;float fertile=0,dry=0;biome.provinceIncome(cap,&fertile);
    std::string previous=c.provinces[cap].biomeId;c.provinces[cap].biomeId="desierto";biome.provinceIncome(cap,&dry);ck(dry<fertile,"biomes affect agricultural production");c.provinces[cap].biomeId=previous;
    std::printf("%d exploration checks, %d failures\n",count,fail);return fail?1:0;
}

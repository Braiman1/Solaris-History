#include "solaris/sim.hpp"
#include <cstdio>
#include <cmath>
using namespace sol;
int main(){
 Content c;if(!c.load(findContentDir()))return 1;
 int checks=0,failures=0;auto ck=[&](bool ok,const char* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}};
 auto faithIndex=[&](const char* tag){for(int i=0;i<(int)c.religions.size();++i)if(c.religions[i]["tag"].string()==tag)return i;return -1;};
 ck(c.startYear==-3300,"production start remains 3300 BC");ck(c.provinces.size()==256,"expanded map");ck(c.cityStates.size()==25,"European tribes");
 Sim s;s.init(c,0);int a=s.player,cap=s.nations[a].capital;s.nations[a].gold=1000;
 ck(s.requiredProvinces(a)==4,"river valley foundation");
 int distant=c.findProvince("Cashel");c.provinces[cap].adj.push_back(distant);
 ck(!s.canColonize(a,distant),"forged long-range edge cannot bypass capital radius");c.provinces[cap].adj.pop_back();
 int nearby=-1;for(int id:c.provinces[cap].adj)if(s.canColonize(a,id)){nearby=id;break;}
 ck(nearby>=0,"local expansion remains possible");
 s.capture(nearby,a);ck(s.prov[nearby].owner==0,"armies cannot bypass colonization by claiming neutral territory");
 ck(s.colonize(a,nearby),"start local expedition");ck(s.prov[nearby].colonyMonths==180,"same culture takes fifteen years");
 ck(!s.canColonize(a,nearby),"one expedition only");s.nations[a].nextColonyAt=s.abs()+60;s.prov[nearby].colonizer=0;s.prov[nearby].colonyMonths=0;
 ck(!s.canColonize(a,nearby),"post-colony cooldown");s.nations[a].nextColonyAt=-1000000;
 s.prov[cap].settledAt=s.abs();ck(!s.canColonize(a,nearby),"new settlements cannot chain expansion");
 int greek=0;for(int i=0;i<(int)c.cityStates.size();++i)if(c.cityStates[i].culture=="egeo")greek=i;
 Sim sea;sea.init(c,greek);int sailor=sea.player;cap=sea.nations[sailor].capital;int island=c.findProvince("Malia");
 auto land=c.provinces[cap].adj;auto routes=c.provinces[cap].seaAdj;c.provinces[cap].adj.clear();c.provinces[cap].seaAdj={island};sea.nations[sailor].gold=1000;
 ck(!sea.canColonize(sailor,island),"sea expansion requires technology and port");sea.nations[sailor].discoveries.push_back("vela");ck(!sea.canColonize(sailor,island),"sailing alone is insufficient");sea.prov[cap].buildings.push_back("puerto");ck(sea.canColonize(sailor,island),"port and sailing permit short crossing");
 c.provinces[cap].adj=land;c.provinces[cap].seaAdj=routes;
 Sim calendar;calendar.init(c,0);calendar.year=-301;calendar.month=11;calendar.step();ck(calendar.year==-300&&calendar.day==1,"switch to daily calendar");double gold=calendar.nations[1].gold;calendar.step();ck(calendar.day==2&&calendar.month==0,"daily advancement");ck(calendar.nations[1].gold==gold,"economy is not paid every day");calendar.year=-296;calendar.month=1;calendar.day=1;ck(calendar.daysInMonth()==29,"leap year");for(int i=0;i<29;++i)calendar.step();ck(calendar.month==2&&calendar.day==1,"February rollover");
 Sim extinct;extinct.init(c,0);extinct.year=-1000;extinct.nations[1].religion="jud";extinct.historicalTick();ck(extinct.judaismAppeared,"Judaism tracked");extinct.nations[1].religion="nil";extinct.year=-1;extinct.historicalTick();ck(extinct.judaismLost,"pre-zero extinction latched");extinct.year=0;extinct.historicalTick();extinct.nations[1].religion="jud";extinct.nations[1].gold=1000;ck(!extinct.adoptReligion(1,faithIndex("chr")),"extinction blocks Christianity");
 Sim lineage;lineage.init(c,0);lineage.year=-1;lineage.nations[1].religion="jud";lineage.historicalTick();lineage.year=0;lineage.historicalTick();lineage.nations[1].gov="teocracia";lineage.nations[1].gold=1000;
 ck(!lineage.adoptReligion(1,faithIndex("nat")),"theocracy cannot switch faith family");ck(!lineage.adoptReligion(1,faithIndex("chr")),"Christianity cannot appear before its historical date");lineage.year=30;ck(lineage.adoptReligion(1,faithIndex("chr")),"Judaism can evolve into Christianity");
 extinct.day=12;extinct.save("frontier_test.sav");Sim loaded;loaded.C=&c;ck(loaded.load("frontier_test.sav"),"version four load");ck(loaded.day==12&&loaded.judaismLost&&!loaded.christianRoot,"calendar and religious history persisted");std::remove("frontier_test.sav");
 Sim laws;laws.init(c,0);laws.nations[1].gov="republica";laws.nations[1].isCityState=false;laws.nations[1].gold=1000;
 ck(laws.setLaw(1,"term2"),"two year term law");laws.holdElection(1,-1);ck(laws.nations[1].electionAt==laws.abs()+24,"two year election scheduling");laws.year+=5;ck(laws.setLaw(1,"term4"),"four year term law");laws.holdElection(1,-1);ck(laws.nations[1].electionAt==laws.abs()+48,"four year administration");laws.year+=5;laws.nations[1].gov="monarquia";ck(laws.setLaw(1,"limited"),"limited monarchy");ck(!laws.nations[1].absolutePower,"royal power is constrained");
 Sim integrate;integrate.init(c,0);int pid=integrate.nations[1].capital;auto& p=integrate.prov[pid];p.localCulture="megalitico";p.localReligion="nat";p.assimilation=.4f;integrate.recomputeEconomy(1);ck(std::fabs(p.assimilation-.4f)<.0001,"coexistence preserves minorities");integrate.nations[1].coexist=false;p.assimilation=.99999f;integrate.recomputeEconomy(1);ck(p.assimilation==1&&p.localCulture==integrate.nations[1].culture,"assimilation completes");integrate.pendingRebellions.push_back({pid,1,"religion"});integrate.processRebellions();ck(p.owner==1,"fully assimilated province cannot rebel");
 p.assimilation=0;p.originalOwner=2;integrate.pendingRebellions.push_back({pid,1,"independence"});integrate.processRebellions();ck(p.owner==2&&p.rebelCause=="independence","independence restores former state");
 Sim aid;aid.init(c,0);aid.nations[1].isCityState=false;aid.nations[1].countryTag="rom";aid.nations[1].culture="italico";aid.nations[1].gold=1000;int tribal=14;
 ck(aid.diplomatic(1,tribal,"civilize"),"cultural sponsorship");for(int i=0;i<119;++i)aid.historicalTick();ck(aid.nations[tribal].culture!="italico","education takes ten years");aid.historicalTick();ck(aid.nations[tribal].culture=="italico","Roman sponsorship creates Latin culture");ck(aid.hasStatecraft(tribal),"sponsor supplies state institutions");
 Sim israel;israel.init(c,3);int semite=israel.player;israel.nations[semite].tech=3;israel.nations[semite].gold=1000;israel.nations[semite].prestige=100;
 int samaria=c.findProvince("Samaria");israel.prov[samaria].owner=semite;int count=2;for(auto& province:israel.prov)if(province.owner==0&&c.province(province.def).cultureId=="semita"&&count<4){province.owner=semite;++count;}
 int option=-1;for(int i=0;i<(int)c.nationTemplates.size();++i)if(c.nationTemplates[i].tag=="isr")option=i;
 ck(option>=0,"Israel exists");israel.year=-1100;ck(!israel.foundNation(semite,option),"Israel date enforced");israel.year=-1000;ck(israel.foundNation(semite,option),"Israel formation with homeland");ck(israel.nations[semite].religion=="jud","Israel establishes Judaism");
 std::printf("%d frontier checks, %d failures\n",checks,failures);return failures?1:0;
}

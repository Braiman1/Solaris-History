#include "solaris/sim.hpp"
#include <cstdio>
using namespace sol;
int main(){
 Content c;if(!c.load(findContentDir()))return 1;
 int failures=0,count=0;auto ck=[&](bool b,const char* s){++count;if(!b){++failures;std::printf("FAIL %s\n",s);}};
 Sim s;s.init(c,0);int a=s.player,b=2;
 ck(s.diplomatic(a,b,"alliance"),"alliance");s.declareWar(a,b);ck(!s.atWar(a,b),"ally cannot be attacked");
 ck(s.diplomatic(a,b,"gold"),"share resources");ck(s.nations[b].gold==160,"recipient gains 20 gold");
 ck(s.diplomatic(a,b,"break"),"break alliance");s.declareWar(a,b);
 ck(!s.diplomatic(a,b,"peace"),"early peace rejected");s.month=6;ck(s.diplomatic(a,b,"peace"),"peace after six months");
 s.nations[a].tech=5000;s.year=0;
 int agriculture=0,copper=-1;for(int i=0;i<(int)c.technologies.size();++i)if(c.technologies[i].id=="cobre")copper=i;
 ck(!s.research(a,copper),"prerequisite required");ck(s.research(a,agriculture),"root research");ck(!s.research(a,agriculture),"no duplicate research");
 ck(s.decide(a,"republica"),"government decision");ck(!s.decide(a,"festival"),"decision cooldown");
 s.nations[a].religion="nat";s.save("strategy_test.sav");Sim loaded;loaded.C=&c;ck(loaded.load("strategy_test.sav"),"version 4 load");
 ck(loaded.hasTech(a,"agricultura"),"research persisted");ck(loaded.nations[a].religion=="nat","religion persisted");ck(loaded.nations[a].decisionAt==s.nations[a].decisionAt,"cooldown persisted");std::remove("strategy_test.sav");
 Sim t;t.init(c,0);for(auto& army:t.armies)army.men=army.nation==a?10000:1;
 t.nations[a].gold=500;t.declareWar(a,b);t.year++;ck(t.diplomatic(a,b,"puppet"),"puppet after war");
 ck(!t.diplomatic(a,b,"annex"),"annexation cooldown");t.year+=10;ck(t.diplomatic(a,b,"annex"),"annex after ten years");ck(!t.nationAlive(b),"annexed state dissolved");
 ck(!t.upgradeArmy(0),"upgrade needs research");t.nations[a].discoveries.push_back("bronce");ck(t.upgradeArmy(0),"researched army upgrade");
 ck(t.armies[0].unit==c.findUnit("guerrero_bronce"),"upgraded stats index");
 ck(t.unitName(a,0)!=t.unitName(2,0),"culture specific unit names");
 auto before=t.nationColor(a);t.nations[a].gov="republica";auto after=t.nationColor(a);ck(before.r!=after.r||before.g!=after.g,"ideology changes shade");
 auto farm=t.mods(a).farm;t.nations[a].religion="nat";ck(t.mods(a).farm!=farm,"religion affects economy");
 Sim allied;allied.init(c,0);ck(allied.diplomatic(2,3,"alliance"),"defensive alliance setup");allied.declareWar(1,2);ck(allied.atWar(3,1),"defensive partner joins");
 bool rom=false;for(const auto& country:c.nationTemplates)if(country.tag=="rom")rom=true;ck(rom,"Rome country tag");
 ck(c.nationTemplates.size()==54,"country files loaded");ck(c.technologies.size()==27,"technology through year zero");
 std::printf("%d strategy checks, %d failures\n",count,failures);return failures?1:0;
}

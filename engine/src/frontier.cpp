#include "solaris/sim.hpp"
#include <algorithm>
#include <cmath>

namespace sol {
int Sim::daysInMonth() const {
    static const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<0||month>11)return 31;
    bool leap=year%4==0 && (year%100!=0||year%400==0);
    return days[month]+(month==1&&leap?1:0);
}
bool Sim::hasStatecraft(int nation) const {
    return nationAlive(nation) && (nations[nation].statecraft || (hasTech(nation,"escritura")&&hasTech(nation,"leyes")));
}
int Sim::requiredProvinces(int nation) const {
    if(!nationAlive(nation))return 5;
    const auto& region=pdef(nations[nation].capital).region;
    return region=="Egipto"||region=="Mesopotamia"||region=="Levante"?4:5;
}
bool Sim::colonyConnected(int nation,int provId) const {
    if(!nationAlive(nation)||!validProvince(provId))return false;
    const auto& n=nations[nation];
    auto km=[&](int from,int to) {
        const auto& a=pdef(from);const auto& b=pdef(to);
        float x=(b.lon-a.lon)*DEG*std::cos((a.lat+b.lat)*.5f*DEG),y=(b.lat-a.lat)*DEG;
        return 6371*std::sqrt(x*x+y*y);
    };
    if(km(n.capital,provId)>900)return false;
    bool adjacent=false;
    for(const auto& source:prov) {
        if(source.owner!=nation || abs()-source.settledAt<120 || source.assimilation<.5f)continue;
        if(km(source.def,provId)>225)continue;
        for(int target:pdef(source.def).adj)if(target==provId)adjacent=true;
        if(hasTech(nation,"vela") && C->hasBuilding(source.buildings,"puerto"))
            for(int target:pdef(source.def).seaAdj)if(target==provId && km(source.def,provId)<=150)adjacent=true;
    }
    return adjacent;
}
int Sim::colonyDuration(int nation,int province) const {
    return pdef(province).cultureId==nations[nation].culture?180:300;
}
bool Sim::setLaw(int nation,const std::string& law) {
    if(!nationAlive(nation))return false;
    auto& n=nations[nation];
    if(n.gold<20||abs()-n.decisionAt<60)return false;
    if(law=="term2"||law=="term4"||law=="term8") {
        if(n.gov!="republica")return false;
        n.electionYears=law=="term2"?2:law=="term4"?4:8;
        if(n.gov=="republica")n.electionAt=n.electionAt>abs()?std::min(n.electionAt,abs()+n.electionYears*12):abs()+n.electionYears*12;
    } else if(law=="absolute"||law=="limited") {
        if(n.gov!="monarquia")return false;
        n.absolutePower=law=="absolute";
    } else if(law=="coexist"||law=="assimilate") n.coexist=law=="coexist";
    else return false;
    n.gold-=20;n.decisionAt=abs();return true;
}
bool Sim::adoptReligion(int nation,int index) {
    if(!nationAlive(nation)||index<0||index>=(int)C->religions.size())return false;
    auto& n=nations[nation];const auto& faith=C->religions[(size_t)index];
    std::string target=faith["tag"].string(),parent=faith["parent"].string();
    if(target==n.religion||year<faith["start_year"].integer(-3300)||n.gold<100||abs()-n.decisionAt<60)return false;
    if(n.gov=="teocracia"&&parent!=n.religion)return false;
    if(!parent.empty()&&parent!=n.religion)return false;
    if((target=="chr"||target=="cat"||target=="ort"||target=="pro") && (!rootChecked||!christianRoot||judaismLost))return false;
    n.gold-=100;n.stability=clampf(n.stability-15,0,100);n.religion=target;n.decisionAt=abs();
    if(target=="jud")judaismAppeared=true;
    return true;
}
void Sim::historicalTick() {
    bool judaism=false;
    for(const auto& n:nations)if(n.alive&&n.religion=="jud")judaism=true;
    for(const auto& p:prov)if(p.localReligion=="jud"&&p.pop>0)judaism=true;
    if(judaism)judaismAppeared=true;
    if(year<0&&judaismAppeared&&!judaism)judaismLost=true;
    if(year>=0&&!rootChecked){christianRoot=judaism&&!judaismLost;rootChecked=true;}
    for(int i=1;i<(int)nations.size();++i) {
        if(!nationAlive(i))continue;
        auto& n=nations[i];
        if(year>=-1200&&n.isCityState&&(n.culture=="megalitico"||n.culture=="danubiano")) {
            const auto& region=pdef(n.capital).region;
            if(region=="Galia"||region=="Britania"||region=="Irlanda"||region=="Caledonia"||region=="Danubio"||region=="Helvecia")n.culture="celta";
        }
        if(n.sponsor) {
            if(!nationAlive(n.sponsor)||atWar(i,n.sponsor)){n.sponsor=0;n.education=0;}
            else if(++n.education>=120) {
                n.culture=nations[n.sponsor].countryTag=="rom"?"italico":nations[n.sponsor].culture;
                n.tech+=3;n.statecraft=true;n.sponsor=0;addLog(n.name+" adopta la cultura de su patrocinador.",1, n.id);
            }
        }
        if(!n.player&&n.religion=="jud"&&year>=0) {
            for(int f=0;f<(int)C->religions.size();++f)if(C->religions[f]["tag"].string()=="chr")adoptReligion(i,f);
        }
        // A historical reformation is conditional on controlling its homeland.
        if(!n.player&&n.culture=="semita"&&year>=-1000&&n.countryTag!="isr") {
            for(int t=0;t<(int)C->nationTemplates.size();++t)if(C->nationTemplates[t].tag=="isr" && foundNation(i,t))break;
        }
    }
}
void Sim::processRebellions() {
    for(const auto& event:pendingRebellions) {
        if(!validProvince(event.province))continue;
        auto& p=prov[event.province];
        if(p.owner!=event.owner||p.assimilation>=1)continue;
        int rebel=p.originalOwner;
        bool restore=event.cause=="independence"&&rebel>0&&rebel<(int)nations.size()&&rebel!=event.owner;
        if(!restore) {
            Nation state;state.id=(int)nations.size();state.name=pdef(p.def).name+" (rebeldes)";
            state.culture=p.localCulture;state.religion=p.localReligion;state.gov=event.cause=="religion"?"teocracia":nations[event.owner].gov=="republica"?"monarquia":"republica";
            state.isCityState=true;state.capital=p.def;state.foundedAt=abs();state.color=C->culture(state.culture).color;state.gold=60;
            state.electionAt=state.gov=="republica"?abs()+state.electionYears*12:0;
            nations.push_back(state);rebel=state.id;
        }
        auto& state=nations[rebel];state.alive=true;state.overlord=0;
        if(!validProvince(state.capital)||prov[state.capital].owner!=rebel)state.capital=p.def;
        state.allies.clear();
        for(auto& n:nations)n.allies.erase(std::remove(n.allies.begin(),n.allies.end(),rebel),n.allies.end());
        capture(p.def,rebel);
        for(auto& army:armies)if(army.nation==event.owner && army.prov==p.def) {
            int retreat=nations[event.owner].capital;
            if(validProvince(retreat)&&prov[retreat].owner==event.owner) {army.prov=retreat;army.dest=-1;army.path.clear();army.morale*=.8f;}
            else army.men=0;
        }
        p.owner=rebel;p.controller=rebel;p.assimilation=.5f;p.loyalty=70;p.unrest=0;p.rebelCause=event.cause;
        if(livingRuler(rebel)<0)state.ruler=createCharacter(rebel,0,-1,40);
        Army army;army.id=nextArmyId++;army.nation=rebel;army.unit=C->findUnit("soldado");army.men=400;army.prov=p.def;armies.push_back(army);
        declareWar(rebel,event.owner);
        std::string cause=event.cause=="religion"?"religiosa":event.cause=="ideology"?"ideologica":event.cause=="hunger"?"hambre":"independencia";
        addLog(pdef(p.def).name+" · Rebelion: "+cause,2, rebel, p.def);
    }
    pendingRebellions.clear();
}
} // namespace sol

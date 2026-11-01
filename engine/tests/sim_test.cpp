// Solaris History - pruebas de simulacion sin interfaz grafica
#include "solaris/content.hpp"
#include "solaris/sim.hpp"
#include "solaris/locale.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace sol;

static int failures = 0;
static int checks = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::printf("  [FALLO] %s\n", what.c_str());
    } else {
        std::printf("  [ok]    %s\n", what.c_str());
    }
}

int main() {
    Content C;
    std::string dir = findContentDir();
    if (!C.load(dir)) {
        std::printf("No se pudo cargar el contenido en %s\n", dir.c_str());
        return 1;
    }
    std::printf("Contenido: %s\n", dir.c_str());
    check(C.provinces.size() == 184, "184 provincias cargadas");
    check(C.cityStates.size() == 13, "13 grupos iniciales cargados");
    check(C.nationTemplates.size() == 46, "46 naciones fundables cargadas");

    check(Locale::load(dir+"/locales.json"), "catalogo bilingue cargado");
    Locale::english=true;
    check(Locale::text("Cargar partida")=="Load game", "menu traducido al ingles");
    check(Locale::text("Colonizacion: 18 meses")=="Colonization: 18 months", "progreso dinamico traducido");
    check(Locale::text("Tecnologias")=="Technologies", "traduccion respeta palabras completas");
    Locale::english=false;
    check(Locale::text("Cargar partida")=="Cargar partida", "cambio de idioma conserva el espanol");
    check(C.resourceIndex.count("estano")==1, "estano reconocido por el motor");
    check(C.resourceIndex.count("papiro")==1, "nuevo recurso papiro reconocido");
    Sim sim;
    sim.init(C, 0);
    float farmBefore=sim.mods(sim.player).farm;
    sim.nations[sim.player].tech=1;
    check(sim.research(sim.player,0), "investigacion manual de agricultura");
    check(sim.mods(sim.player).farm>farmBefore, "la tecnologia agricola mejora la produccion");
    sim.nations[sim.player].tech=0;
    const int player = sim.player;
    const int capital = sim.nations[(size_t)player].capital;
    check(player == 1, "el jugador es la primera ciudad-estado");
    check(sim.prov[(size_t)capital].owner == player, "la capital pertenece al jugador");
    check(sim.nations[(size_t)player].isCityState, "el jugador empieza como ciudad-estado");
    check(sim.livingRuler(player) >= 0, "el jugador tiene gobernante vivo");
    check(sim.armiesIn(capital) >= 1, "hay guarnicion en la capital");

    // colonizacion de un vecino libre
    int target = -1;
    for (int a : C.province(capital).adj) {
        if (sim.prov[(size_t)a].owner == 0) { target = a; break; }
    }
    check(target >= 0, "existe una provincia vecina libre");
    sim.nations[(size_t)player].gold = 500;
    sim.nations[(size_t)player].prestige = 100;
    check(sim.canColonize(player, target), "se puede colonizar el vecino");
    check(sim.colonize(player, target), "colonizacion ejecutada");
    check(sim.prov[(size_t)target].owner == 0, "la expedicion no concede territorio instantaneo");
    check(!sim.colonize(player, target), "no se puede duplicar una expedicion");
    sim.save("sim_test_colony.sav");
    Sim pending;
    pending.C = &C;
    check(pending.load("sim_test_colony.sav"), "se carga una expedicion activa");
    check(pending.prov[(size_t)target].colonyMonths == 36, "se conserva el progreso colonial");
    std::remove("sim_test_colony.sav");
    for (int i=0;i<35;++i) sim.step();
    check(sim.prov[(size_t)target].owner == 0, "el territorio sigue libre tras 35 meses");
    sim.step();
    check(sim.prov[(size_t)target].colonyMonths == 0, "la expedicion termina al mes 36");
    check(sim.prov[(size_t)target].owner == player, "el nuevo asentamiento pertenece al jugador");

    // fundacion de nacion
    int tmpl = -1;
    for (int i = 0; i < (int)C.nationTemplates.size(); ++i) {
        if (C.nationTemplates[(size_t)i].culture == sim.nations[(size_t)player].culture
            && C.nationTemplates[(size_t)i].era <= sim.nations[(size_t)player].era) { tmpl = i; break; }
    }
    check(tmpl >= 0, "hay una nacion fundable para la cultura del jugador");
    sim.nations[(size_t)player].gold = 500;
    sim.nations[(size_t)player].prestige = 100;
    sim.nations[(size_t)player].tech = 2.99f;
    check(!sim.canFound(player), "menos de 3 puntos impide fundar");
    sim.nations[(size_t)player].tech = 3;
    check(sim.canFound(player), "se cumplen los requisitos de fundacion");
    check(sim.foundNation(player, tmpl), "fundacion ejecutada");
    check(!sim.nations[(size_t)player].isCityState, "el jugador ya no es ciudad-estado");
    check(sim.nations[(size_t)player].name == C.nationTemplates[(size_t)tmpl].name, "el nombre de la nacion cambio");

    // construccion y reclutamiento
    sim.nations[(size_t)player].gold = 500;
    sim.nations[(size_t)player].wood = 200;
    sim.nations[(size_t)player].stone = 200;
    sim.nations[(size_t)player].food = 200;
    sim.nations[(size_t)player].manpower = 20;
    int granja = C.findBuilding("granja");
    check(granja >= 0, "existe el edificio granja");
    check(sim.build(player, capital, granja), "granja construida");
    check(C.hasBuilding(sim.prov[(size_t)capital].buildings, "granja"), "la granja queda registrada");

    int soldado = C.findUnit("soldado");
    int armiesBefore = (int)sim.armies.size();
    check(sim.recruit(player, capital, soldado), "soldado reclutado");
    check((int)sim.armies.size() == armiesBefore + 1, "el ejercito aparece en la lista");

    // guerra y movimiento
    int enemy = 2;
    check(enemy != player && sim.nationAlive(enemy), "hay una nacion rival viva");
    sim.declareWar(player, enemy);
    check(sim.atWar(player, enemy), "la guerra esta declarada");
    int enemyCapital = target; // A reachable owned neighbor tests movement without assuming transit rights across neutral states.
    int armyId = (int)sim.armies.size() - 1;
    std::vector<int> path;
    float cost = 0;
    bool pathOk = sim.findPath(armyId, enemyCapital, path, cost);
    check(pathOk, "se encontro una ruta militar");
    if (pathOk) {
        sim.orderMove(armyId, enemyCapital);
        int guard = 0;
        while (sim.armies[(size_t)armyId].inTransit() && guard++ < 60) sim.step();
        check(guard < 60, "el ejercito llega a su destino");
    }
    sim.makePeace(player, enemy);
    check(!sim.atWar(player, enemy), "la paz se firmo");

    // sucesion y elecciones
    Nation& n = sim.nations[(size_t)player];
    n.gov = "republica";
    n.electionAt = sim.abs();
    int candidate = sim.createCharacter(player, 2, -1, 40);
    check(candidate >= 0, "se creo un candidato");
    if (candidate >= 0) {
        sim.holdElection(player, candidate);
        check(n.ruler == candidate, "el candidato gano la eleccion");
    }
    n.gov = "monarquia";
    // muerte del gobernante y sustitucion
    int ruler = sim.livingRuler(player);
    check(ruler >= 0, "hay gobernante antes de la muerte");
    if (ruler >= 0) {
        sim.chars[(size_t)ruler].deathAbs = sim.abs() - 1;
        sim.processDeaths();
        check(!sim.chars[(size_t)ruler].alive, "el gobernante muere");
        int newRuler = sim.livingRuler(player);
        check(newRuler >= 0, "el consejo o la sucesion nombra un nuevo gobernante");
    }

    // Later Latin states cannot exist in 3300 BC.
    Sim latin;
    int latinIndex = 0;
    for (size_t i=0;i<C.cityStates.size();++i) if(C.cityStates[i].culture=="italico") latinIndex=(int)i;
    latin.init(C,latinIndex);
    int lp=latin.player;
    latin.nations[lp].tech=3; latin.nations[lp].gold=500; latin.nations[lp].prestige=100;
    for (auto& p : latin.prov) if (C.province(p.def).cultureId=="italico") {p.owner=lp;break;}
    int rome=-1;
    for(size_t i=0;i<C.nationTemplates.size();++i) if(C.nationTemplates[i].id=="roma") rome=(int)i;
    check(rome>=0, "Roma tiene una plantilla separada del grupo latino");
    check(!latin.foundNation(lp,rome), "Roma bloqueada antes de 753 a.C.");
    latin.year=-753;
    check(latin.foundNation(lp,rome), "Roma disponible desde su fecha tradicional");

    // La prueba economica usa una red de alianzas para aislarla de conquistas de la IA.
    sim.nations[player].prestige=1000;
    for(int other=1;other<(int)sim.nations.size();++other)if(other!=player && sim.nationAlive(other)){sim.makePeace(player,other);sim.diplomatic(player,other,"alliance");}
    // economia a largo plazo
    n.gov = "monarquia";
    n.electionAt = 0;
    n.gold = 1000;
    n.food = 1000;
    for (int i = 0; i < 240; ++i) sim.step();
    check(sim.year > -3300, "el tiempo avanza");
    check(sim.prov.size() == C.provinces.size(), "el mundo conserva sus provincias");
    check(sim.nationAlive(player), "el jugador sigue vivo tras 20 anos");

    // guardado y carga
    const std::string savePath = "sim_test_save.sav";
    int savedGold = (int)sim.nations[(size_t)player].gold;
    std::string savedName = sim.nations[(size_t)player].name;
    sim.save(savePath);
    Sim loaded;
    loaded.C = &C;
    check(loaded.load(savePath), "la partida se carga");
    check(loaded.player == player, "el jugador se conserva");
    check(loaded.year == sim.year && loaded.month == sim.month, "la fecha se conserva");
    check(loaded.prov.size() == sim.prov.size(), "las provincias se conservan");
    check(loaded.nations[(size_t)player].name == savedName, "el nombre de la nacion se conserva");
    check((int)loaded.nations[(size_t)player].gold == savedGold, "el oro se conserva");
    check(loaded.armies.size() == sim.armies.size(), "los ejercitos se conservan");
    check(loaded.chars.size() == sim.chars.size(), "los personajes se conservan");
    std::remove(savePath.c_str());

    std::printf("\n%d comprobaciones, %d fallos\n", checks, failures);
    return failures == 0 ? 0 : 1;
}

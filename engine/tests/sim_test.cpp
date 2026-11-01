// Solaris History - pruebas de simulacion sin interfaz grafica
#include "solaris/content.hpp"
#include "solaris/sim.hpp"
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
    check(C.provinces.size() == 164, "164 provincias cargadas");
    check(C.cityStates.size() == 12, "12 ciudades-estado cargadas");
    check(C.nationTemplates.size() == 42, "42 naciones fundables cargadas");

    Sim sim;
    sim.init(C, 0);
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
    check(sim.prov[(size_t)target].owner == player, "el nuevo asentamiento pertenece al jugador");

    // fundacion de nacion
    int tmpl = -1;
    for (int i = 0; i < (int)C.nationTemplates.size(); ++i) {
        if (C.nationTemplates[(size_t)i].culture == sim.nations[(size_t)player].culture
            && C.nationTemplates[(size_t)i].era <= sim.nations[(size_t)player].era) { tmpl = i; break; }
    }
    check(tmpl >= 0, "hay una nacion fundable para la cultura del jugador");
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
    int enemyCapital = sim.nations[(size_t)enemy].capital;
    int armyId = sim.armies.back().id;
    std::vector<int> path;
    float cost = 0;
    bool pathOk = sim.findPath(armyId, enemyCapital, path, cost);
    check(pathOk, "se encontro una ruta militar");
    if (pathOk) {
        sim.orderMove(armyId, enemyCapital);
        int guard = 0;
        while (sim.armies.back().inTransit() && guard++ < 60) sim.step();
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

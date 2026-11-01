package solaris.tools;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Comprobaciones de integridad y coherencia del contenido. */
public final class Validator {
    public static final class Result {
        public final List<String> errors = new ArrayList<>();
        public final List<String> warnings = new ArrayList<>();

        public boolean ok() {
            return errors.isEmpty();
        }

        public void error(String msg) {
            errors.add(msg);
        }

        public void warn(String msg) {
            warnings.add(msg);
        }
    }

    private Validator() {
    }

    public static Result validate(Content c) {
        Result r = new Result();
        validateProvinces(c, r);
        validateCultures(c, r);
        validateUnits(c, r);
        validateBuildings(c, r);
        validateEras(c, r);
        validateGovernments(c, r);
        validateTraitsAndNames(c, r);
        validateNations(c, r);
        validateCityStates(c, r);
        validateEconomy(c, r);
        return r;
    }

    private static void validateProvinces(Content c, Result r) {
        if (c.provinces.isEmpty()) {
            r.error("world.json no contiene provincias");
            return;
        }
        Set<Integer> ids = new HashSet<>();
        Set<String> names = new HashSet<>();
        int expected = 0;
        for (Map<String, Object> p : c.provinces) {
            int id = Json.integer(p, "id", -1);
            String name = Json.str(p, "name", "(sin nombre)");
            if (id < 0) {
                r.error("Provincia '" + name + "': id invalido " + id);
                continue;
            }
            if (!ids.add(id)) r.error("Provincia '" + name + "': id duplicado " + id);
            if (!names.add(name)) r.error("Provincia duplicada: '" + name + "'");
            if (id != expected++) {
                r.warn("Los ids de provincia no son consecutivos cerca de " + name + " (id " + id + ", esperado " + (expected - 1) + ")");
            }
            double lat = Json.number(p, "lat", 999);
            double lon = Json.number(p, "lon", 999);
            if (lat < -90 || lat > 90) r.error("Provincia '" + name + "': latitud fuera de rango (" + lat + ")");
            if (lon < -180 || lon > 180) r.error("Provincia '" + name + "': longitud fuera de rango (" + lon + ")");
            String terrain = Json.str(p, "terrain");
            String resource = Json.str(p, "resource");
            String culture = Json.str(p, "culture");
            if (!c.terrains.containsKey(terrain)) r.error("Provincia '" + name + "': terreno desconocido '" + terrain + "'");
            if (!c.resources.containsKey(resource)) r.error("Provincia '" + name + "': recurso desconocido '" + resource + "'");
            if (!c.cultureById.containsKey(culture)) r.error("Provincia '" + name + "': cultura desconocida '" + culture + "'");
        }
        if (expected == 0) r.error("No se pudo comprobar la secuencia de ids de provincia");

        // adyacencia simetrica
        for (Map<String, Object> p : c.provinces) {
            int id = Json.integer(p, "id", -1);
            String name = Json.str(p, "name", "?");
            checkAdj(c, r, p, id, name, false);
            checkAdj(c, r, p, id, name, true);
        }
    }

    private static void checkAdj(Content c, Result r, Map<String, Object> p, int id, String name, boolean sea) {
        String key = sea ? "sea_adj" : "adj";
        Set<Integer> seen = new HashSet<>();
        for (Object o : Json.list(p, key)) {
            if (!(o instanceof Number n)) {
                r.error("Provincia '" + name + "': entrada no numerica en " + key);
                continue;
            }
            int q = (int) Math.round(n.doubleValue());
            if (q == id) {
                r.error("Provincia '" + name + "': se referencia a si misma en " + key);
                continue;
            }
            Map<String, Object> other = c.province(q);
            if (other == null) {
                r.error("Provincia '" + name + "': vecino inexistente " + q + " en " + key);
                continue;
            }
            if (!seen.add(q)) {
                r.warn("Provincia '" + name + "': vecino repetido " + q + " en " + key);
            }
            boolean back = false;
            for (Object b : Json.list(other, key)) {
                if (b instanceof Number bn && (int) Math.round(bn.doubleValue()) == id) { back = true; break; }
            }
            if (!back) {
                r.warn("Adyacencia " + key + " no simetrica: " + name + " -> " + Json.str(other, "name", "?"));
            }
        }
    }

    private static void validateCultures(Content c, Result r) {
        if (c.cultures.isEmpty()) r.error("cultures.json esta vacio");
        Set<String> ids = new HashSet<>();
        for (Map<String, Object> cu : c.cultures) {
            String id = Json.str(cu, "id");
            if (id.isBlank()) r.error("Cultura sin id");
            if (!ids.add(id)) r.error("Cultura duplicada: '" + id + "'");
            if (Json.str(cu, "name").isBlank()) r.error("Cultura '" + id + "': falta el nombre");
            colorCheck(c, r, cu, "Cultura '" + id + "'");
            Map<String, Object> mods = Json.map(cu, "mods");
            if (mods.isEmpty()) r.warn("Cultura '" + id + "': sin modificadores");
        }
    }

    private static void validateUnits(Content c, Result r) {
        if (c.units.isEmpty()) r.error("units.json esta vacio");
        Set<String> ids = new HashSet<>();
        for (Map<String, Object> u : c.units) {
            String id = Json.str(u, "id");
            if (id.isBlank()) r.error("Unidad sin id");
            if (!ids.add(id)) r.error("Unidad duplicada: '" + id + "'");
            int era = Json.integer(u, "era", -1);
            if (era < 0 || era >= c.eras.size()) r.error("Unidad '" + id + "': era invalida " + era);
            if (Json.integer(u, "men", 0) <= 0) r.error("Unidad '" + id + "': hombres debe ser > 0");
            if (Json.number(u, "atk", 0) <= 0) r.error("Unidad '" + id + "': ataque debe ser > 0");
            if (Json.number(u, "defense", 0) <= 0) r.error("Unidad '" + id + "': defensa debe ser > 0");
            if (Json.number(u, "gold", 0) < 0 || Json.number(u, "food", 0) < 0) {
                r.error("Unidad '" + id + "': costes negativos");
            }
            String icon = Json.str(u, "icon", id);
            Path raw = c.dir.resolve("textures").resolve(icon + ".raw");
            Path png = c.dir.resolve("textures").resolve(icon + ".png");
            if (!Files.isRegularFile(raw) && !Files.isRegularFile(png)) {
                r.warn("Unidad '" + id + "': no se encontro el icono '" + icon + ".(raw|png)'");
            }
            if (Json.bool(u, "naval", false) && Json.integer(u, "era", 0) < 0) {
                r.error("Unidad naval '" + id + "': era invalida");
            }
        }
    }

    private static void validateBuildings(Content c, Result r) {
        Set<String> ids = new HashSet<>();
        for (Map<String, Object> b : c.buildings) {
            String id = Json.str(b, "id");
            if (!ids.add(id)) r.error("Edificio duplicado: '" + id + "'");
            int era = Json.integer(b, "era", -1);
            if (era < 0 || era >= c.eras.size()) r.error("Edificio '" + id + "': era invalida " + era);
            Map<String, Object> cost = Json.map(b, "cost");
            for (String k : List.of("gold", "wood", "stone")) {
                if (Json.number(cost, k, 0) < 0) r.error("Edificio '" + id + "': coste negativo de " + k);
            }
            if (Json.number(cost, "gold", 0) == 0 && Json.number(cost, "wood", 0) == 0 && Json.number(cost, "stone", 0) == 0) {
                r.warn("Edificio '" + id + "': coste cero");
            }
            if (Json.map(b, "mods").isEmpty()) r.warn("Edificio '" + id + "': sin modificadores");
        }
    }

    private static void validateEras(Content c, Result r) {
        if (c.eras.isEmpty()) {
            r.error("eras.json esta vacio");
            return;
        }
        int expected = 0;
        double lastTech = -1;
        int lastStart = Integer.MIN_VALUE;
        for (Map<String, Object> e : c.eras) {
            int id = Json.integer(e, "id", -1);
            if (id != expected) r.error("Era con id " + id + ", esperado " + expected);
            expected++;
            double tech = Json.number(e, "tech", 0);
            if (tech < lastTech) r.warn("Las eras no estan ordenadas por tecnologia");
            lastTech = tech;
            if (e.containsKey("start")) {
                int start = Json.integer(e, "start", 0);
                if (start < lastStart) r.warn("Las eras no estan ordenadas por ano de inicio");
                lastStart = start;
            }
            if (Json.number(e, "prod", 1) <= 0) r.error("Era '" + Json.str(e, "name") + "': produccion no positiva");
        }
        if (Json.integer(c.meta(), "start_year", -3300) != Json.integer(c.eras.get(0), "start", -3300)) {
            r.warn("El ano inicial del meta no coincide con el de la primera era");
        }
    }

    private static void validateGovernments(Content c, Result r) {
        for (String g : List.of("teocracia", "monarquia", "republica")) {
            if (!c.governments.containsKey(g)) r.error("Falta el gobierno '" + g + "' en leaders.json");
        }
        for (Map.Entry<String, Map<String, Object>> e : c.governments.entrySet()) {
            int election = Json.integer(e.getValue(), "election", 0);
            if (election < 0) r.error("Gobierno '" + e.getKey() + "': periodo electoral negativo");
        }
    }

    private static void validateTraitsAndNames(Content c, Result r) {
        Set<String> ids = new HashSet<>();
        for (Map<String, Object> t : c.traits) {
            String id = Json.str(t, "id");
            if (!ids.add(id)) r.error("Rasgo duplicado: '" + id + "'");
            if (Json.str(t, "name").isBlank()) r.error("Rasgo '" + id + "': falta nombre");
            if (Json.map(t, "mods").isEmpty()) r.warn("Rasgo '" + id + "': sin modificadores");
        }
        for (Map<String, Object> cu : c.cultures) {
            String id = Json.str(cu, "id");
            List<String> male = c.namePool(id, false);
            List<String> female = c.namePool(id, true);
            if (male.size() < 3) r.warn("Cultura '" + id + "': menos de 3 nombres masculinos");
            if (female.size() < 3) r.warn("Cultura '" + id + "': menos de 3 nombres femeninos");
        }
    }

    private static void validateNations(Content c, Result r) {
        Set<String> ids = new HashSet<>();
        Set<String> names = new HashSet<>();
        for (Map<String, Object> n : c.nations) {
            String id = Json.str(n, "id");
            String name = Json.str(n, "name");
            if (!ids.add(id)) r.error("Nacion duplicada (id): '" + id + "'");
            if (!names.add(name)) r.error("Nacion duplicada (nombre): '" + name + "'");
            String culture = Json.str(n, "culture");
            if (!c.cultureById.containsKey(culture)) r.error("Nacion '" + name + "': cultura desconocida '" + culture + "'");
            String gov = Json.str(n, "gov");
            if (!c.governments.containsKey(gov)) r.error("Nacion '" + name + "': gobierno desconocido '" + gov + "'");
            String capital = Json.str(n, "capital");
            if (c.province(capital) == null) r.error("Nacion '" + name + "': capital inexistente '" + capital + "'");
            int era = Json.integer(n, "era", 0);
            if (era < 0 || era >= c.eras.size()) r.error("Nacion '" + name + "': era invalida " + era);
            colorCheck(c, r, n, "Nacion '" + name + "'");
        }
        if (c.nations.isEmpty()) r.error("nations.json esta vacio");
    }

    private static void validateCityStates(Content c, Result r) {
        Set<Integer> ids = new HashSet<>();
        Set<Integer> capitals = new HashSet<>();
        int playable = 0;
        for (Map<String, Object> cs : c.cityStates) {
            int id = Json.integer(cs, "id", -1);
            String name = Json.str(cs, "name", "?");
            if (id <= 0) r.error("Ciudad-estado '" + name + "': id debe ser > 0");
            if (!ids.add(id)) r.error("Ciudad-estado '" + name + "': id duplicado " + id);
            int capital = Json.integer(cs, "capital", -1);
            Map<String, Object> p = c.province(capital);
            if (p == null) {
                r.error("Ciudad-estado '" + name + "': provincia capital inexistente " + capital);
            } else {
                if (!capitals.add(capital)) r.error("Ciudad-estado '" + name + "': capital compartida con otra ciudad");
                String pname = Json.str(p, "name");
                if (!pname.equals(name) && !Json.str(cs, "prov", "").equals(pname)) {
                    r.warn("Ciudad-estado '" + name + "': la capital se llama '" + pname + "'");
                }
            }
            String culture = Json.str(cs, "culture");
            if (!c.cultureById.containsKey(culture)) r.error("Ciudad-estado '" + name + "': cultura desconocida '" + culture + "'");
            String gov = Json.str(cs, "gov");
            if (!c.governments.containsKey(gov)) r.error("Ciudad-estado '" + name + "': gobierno desconocido '" + gov + "'");
            if (Json.bool(cs, "playable", false)) playable++;
            colorCheck(c, r, cs, "Ciudad-estado '" + name + "'");
        }
        if (playable == 0) r.error("No hay ninguna ciudad-estado jugable");
        if (playable < 4) r.warn("Solo hay " + playable + " ciudades-estado jugables");
        if (c.cityStates.size() < 6) r.warn("Hay menos de 6 ciudades-estado iniciales");
        // los ids deben ser consecutivos desde 1 para que el motor indexe naciones
        for (int i = 0; i < c.cityStates.size(); i++) {
            int id = Json.integer(c.cityStates.get(i), "id", -1);
            if (id != i + 1) r.warn("Los ids de ciudad-estado no son consecutivos: indice " + i + ", id " + id);
        }
    }

    private static void validateEconomy(Content c, Result r) {
        // coste minimo de reclutamiento frente al oro inicial
        double minGold = Double.MAX_VALUE;
        double minTotal = Double.MAX_VALUE;
        for (Map<String, Object> u : c.units) {
            double gold = Json.number(u, "gold", 0);
            minGold = Math.min(minGold, gold);
            minTotal = Math.min(minTotal, gold + Json.number(u, "food", 0) * 0.5 + Json.number(u, "wood", 0) * 0.5);
        }
        if (minGold == Double.MAX_VALUE) return;
        if (minGold > 140) r.error("Ninguna unidad es asequible con el oro inicial (140)");
        if (minTotal > 200) r.warn("El coste combinado de la unidad mas barata es muy alto: " + minTotal);

        // ingresos teoricos de una provincia media
        double totalIncome = 0;
        for (Map<String, Object> p : c.provinces) {
            totalIncome += estimateProvinceIncome(c, p);
        }
        double avg = totalIncome / Math.max(1, c.provinces.size());
        if (avg < 0.5) r.warn("Ingreso medio por provincia muy bajo: " + String.format("%.2f", avg) + " oro/mes");
        if (avg > 8) r.warn("Ingreso medio por provincia muy alto: " + String.format("%.2f", avg) + " oro/mes");

        // cada cultura debe tener alguna nacion fundable
        Set<String> culturesWithNation = new HashSet<>();
        for (Map<String, Object> n : c.nations) culturesWithNation.add(Json.str(n, "culture"));
        for (Map<String, Object> cu : c.cultures) {
            String id = Json.str(cu, "id");
            boolean hasProvinces = c.provinces.stream().anyMatch(p -> Json.str(p, "culture").equals(id));
            if (hasProvinces && !culturesWithNation.contains(id)) {
                r.warn("La cultura '" + id + "' tiene provincias pero ninguna nacion fundable");
            }
        }
    }

    public static double estimateProvinceIncome(Content c, Map<String, Object> p) {
        Map<String, Object> terrain = c.terrains.get(Json.str(p, "terrain"));
        Map<String, Object> resource = c.resources.get(Json.str(p, "resource"));
        Map<String, Object> culture = c.cultureById.get(Json.str(p, "culture"));
        double pop = Json.number(p, "pop", 1);
        double dev = Json.integer(p, "dev", 1);
        double food = terrain == null ? 1 : Json.number(terrain, "food", 1);
        double value = resource == null ? 1 : Json.number(resource, "value", 1);
        double trade = culture == null ? 1 : Json.number(Json.map(culture, "mods"), "trade", 1);
        return (pop * 0.10 + dev * 0.05) * food * (0.8 + value * 0.35) * trade;
    }

    private static void colorCheck(Content c, Result r, Map<String, Object> obj, String label) {
        Object col = Json.obj(obj).get("color");
        if (!(col instanceof List<?> list) || list.size() < 3) {
            r.error(label + ": color ausente o invalido");
            return;
        }
        for (int i = 0; i < 3; i++) {
            if (!(list.get(i) instanceof Number n) || n.doubleValue() < 0 || n.doubleValue() > 255) {
                r.error(label + ": componente de color fuera de rango");
                return;
            }
        }
    }
}

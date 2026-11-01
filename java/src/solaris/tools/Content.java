package solaris.tools;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Carga y expone el contenido historico de Solaris History. */
public final class Content {
    public final Path dir;
    public final Map<String, Object> world;
    public final Map<String, Object> leaders;
    public final List<Map<String, Object>> provinces = new ArrayList<>();
    public final List<Map<String, Object>> cityStates = new ArrayList<>();
    public final List<Map<String, Object>> cultures = new ArrayList<>();
    public final List<Map<String, Object>> nations = new ArrayList<>();
    public final List<Map<String, Object>> units = new ArrayList<>();
    public final List<Map<String, Object>> buildings = new ArrayList<>();
    public final List<Map<String, Object>> eras = new ArrayList<>();
    public final Map<String, Map<String, Object>> terrains = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> resources = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> governments = new LinkedHashMap<>();
    public final List<Map<String, Object>> traits = new ArrayList<>();
    public final Map<String, Map<String, Object>> provinceByName = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> cultureById = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> unitById = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> buildingById = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> nationById = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> nationByName = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> cityStateByName = new LinkedHashMap<>();
    public final Map<String, Map<String, Object>> traitById = new LinkedHashMap<>();

    private Content(Path dir) throws IOException {
        this.dir = dir;
        this.world = loadObject(dir.resolve("world.json"));
        this.leaders = loadObject(dir.resolve("leaders.json"));

        for (Map<String, Object> t : loadList(dir.resolve("cultures.json"))) cultures.add(t);
        for (Object tag : Json.arr(Json.parse(Files.readString(dir.resolve("countries/index.json"), StandardCharsets.UTF_8)))) {
            Map<String,Object> country=loadObject(dir.resolve("countries/" + tag + ".json"));
            country.put("culture",Json.str(country,"culture_tag"));
            nations.add(country);
        }
        for (Map<String, Object> t : loadList(dir.resolve("units.json"))) units.add(t);
        for (Map<String, Object> t : loadList(dir.resolve("buildings.json"))) buildings.add(t);
        for (Map<String, Object> t : loadList(dir.resolve("eras.json"))) eras.add(t);

        for (Map.Entry<String, Object> e : Json.map(world, "terrains").entrySet()) {
            terrains.put(e.getKey(), Json.obj(e.getValue()));
        }
        for (Map.Entry<String, Object> e : Json.map(world, "resources").entrySet()) {
            resources.put(e.getKey(), Json.obj(e.getValue()));
        }
        for (Object o : Json.list(world, "provinces")) {
            Map<String, Object> p = Json.obj(o);
            provinces.add(p);
            provinceByName.put(Json.str(p, "name"), p);
        }
        for (Object o : Json.list(world, "city_states")) {
            Map<String, Object> c = Json.obj(o);
            cityStates.add(c);
            cityStateByName.put(Json.str(c, "name"), c);
        }
        for (Map.Entry<String, Object> e : Json.map(leaders, "governments").entrySet()) {
            governments.put(e.getKey(), Json.obj(e.getValue()));
        }
        for (Object o : Json.list(leaders, "traits")) {
            Map<String, Object> t = Json.obj(o);
            traits.add(t);
            traitById.put(Json.str(t, "id"), t);
        }

        for (Map<String, Object> c : cultures) cultureById.put(Json.str(c, "id"), c);
        for (Map<String, Object> u : units) unitById.put(Json.str(u, "id"), u);
        for (Map<String, Object> b : buildings) buildingById.put(Json.str(b, "id"), b);
        for (Map<String, Object> n : nations) {
            nationById.put(Json.str(n, "id"), n);
            nationByName.put(Json.str(n, "name"), n);
        }
    }

    public static Content load(Path dir) throws IOException {
        if (!Files.isDirectory(dir)) {
            throw new IOException("No existe la carpeta de contenido: " + dir.toAbsolutePath());
        }
        return new Content(dir);
    }

    private static Map<String, Object> loadObject(Path path) throws IOException {
        String text = Files.readString(path, StandardCharsets.UTF_8);
        Object value = Json.parse(text);
        if (!(value instanceof Map<?, ?>)) {
            throw new IOException(path.getFileName() + ": se esperaba un objeto JSON");
        }
        return Json.obj(value);
    }

    private static List<Map<String, Object>> loadList(Path path) throws IOException {
        String text = Files.readString(path, StandardCharsets.UTF_8);
        Object value = Json.parse(text);
        if (!(value instanceof List<?>)) {
            throw new IOException(path.getFileName() + ": se esperaba una lista JSON");
        }
        List<Map<String, Object>> out = new ArrayList<>();
        for (Object o : Json.arr(value)) out.add(Json.obj(o));
        return out;
    }

    public Map<String, Object> meta() {
        return Json.map(world, "meta");
    }

    public int startYear() {
        return Json.integer(meta(), "start_year", -3300);
    }

    public Map<String, Object> province(String name) {
        return provinceByName.get(name);
    }

    public Map<String, Object> province(int id) {
        if (id < 0) return null;
        for (Map<String, Object> p : provinces) {
            if (Json.integer(p, "id", -1) == id) return p;
        }
        return null;
    }

    public Map<String, Object> cityState(int id) {
        for (Map<String, Object> c : cityStates) {
            if (Json.integer(c, "id", -1) == id) return c;
        }
        return null;
    }

    public List<String> namePool(String cultureId, boolean female) {
        Map<String, Object> names = Json.map(leaders, "names");
        Object pool = names.get(cultureId);
        if (!(pool instanceof List<?> list) || list.size() < 2) return List.of();
        List<String> out = new ArrayList<>();
        for (Object o : Json.arr(list.get(female ? 1 : 0))) out.add(String.valueOf(o));
        return out;
    }

    public String colorText(Object obj) {
        Object c = Json.obj(obj).get("color");
        if (!(c instanceof List<?> list) || list.size() < 3) return "?";
        StringBuilder sb = new StringBuilder("#");
        for (int i = 0; i < 3; i++) {
            int v = (int) Math.round(((Number) list.get(i)).doubleValue());
            sb.append(String.format("%02X", Math.max(0, Math.min(255, v))));
        }
        return sb.toString();
    }
}

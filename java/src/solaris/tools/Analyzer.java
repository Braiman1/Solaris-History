package solaris.tools;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;

/** Informes, auditoria de balance y exportacion del contenido. */
public final class Analyzer {
    private Analyzer() {
    }

    public static String stats(Content c) {
        StringBuilder sb = new StringBuilder();
        int playable = 0;
        for (Map<String, Object> cs : c.cityStates) if (Json.bool(cs, "playable", false)) playable++;
        int coastal = 0, landLinks = 0, seaLinks = 0;
        for (Map<String, Object> p : c.provinces) {
            if (Json.bool(p, "coastal", false)) coastal++;
            landLinks += Json.list(p, "adj").size();
            seaLinks += Json.list(p, "sea_adj").size();
        }
        sb.append("SOLARIS HISTORY - CONTENIDO\n");
        sb.append("===========================\n");
        sb.append(String.format(Locale.ROOT, "Ano inicial: %d  |  Provincias: %d  |  Ciudades-estado: %d (%d jugables)%n",
                c.startYear(), c.provinces.size(), c.cityStates.size(), playable));
        sb.append(String.format(Locale.ROOT, "Naciones fundables: %d  |  Culturas: %d  |  Unidades: %d  |  Edificios: %d  |  Eras: %d%n",
                c.nations.size(), c.cultures.size(), c.units.size(), c.buildings.size(), c.eras.size()));
        sb.append(String.format(Locale.ROOT, "Provincias costeras: %d  |  Enlaces terrestres: %d  |  Enlaces maritimos: %d%n%n",
                coastal, landLinks / 2, seaLinks / 2));

        sb.append("Provincias por region\n");
        for (Map.Entry<String, Integer> e : count(c.provinces, "region").entrySet()) {
            sb.append(String.format(Locale.ROOT, "  %-22s %3d%n", e.getKey(), e.getValue()));
        }
        sb.append("\nProvincias por cultura\n");
        for (Map.Entry<String, Integer> e : count(c.provinces, "culture").entrySet()) {
            String name = c.cultureById.containsKey(e.getKey()) ? Json.str(c.cultureById.get(e.getKey()), "name") : e.getKey();
            sb.append(String.format(Locale.ROOT, "  %-24s %3d%n", name, e.getValue()));
        }
        sb.append("\nTerrenos\n");
        for (Map.Entry<String, Integer> e : count(c.provinces, "terrain").entrySet()) {
            String name = c.terrains.containsKey(e.getKey()) ? Json.str(c.terrains.get(e.getKey()), "name") : e.getKey();
            sb.append(String.format(Locale.ROOT, "  %-24s %3d%n", name, e.getValue()));
        }
        sb.append("\nCiudades-estado iniciales\n");
        for (Map<String, Object> cs : c.cityStates) {
            int capital = Json.integer(cs, "capital", -1);
            Map<String, Object> p = c.province(capital);
            double pop = p == null ? 0 : Json.number(p, "pop", 0);
            sb.append(String.format(Locale.ROOT, "  %-12s %-12s %-11s %-12s %5.1fk %s%n",
                    Json.str(cs, "name"), Json.str(cs, "culture"), Json.str(cs, "gov"),
                    p == null ? "?" : Json.str(p, "name"), pop,
                    Json.bool(cs, "playable", false) ? "[jugable]" : ""));
        }
        sb.append("\nUnidades\n");
        for (Map<String, Object> u : c.units) {
            sb.append(String.format(Locale.ROOT, "  %-10s era %d  %4d hombres  oro %3.0f  ATQ %2.0f  DEF %2.0f  mant. %.1f%n",
                    Json.str(u, "name"), Json.integer(u, "era", 0), Json.integer(u, "men", 0),
                    Json.number(u, "gold", 0), Json.number(u, "atk", 0), Json.number(u, "defense", 0),
                    Json.number(u, "upkeep", 0)));
        }
        sb.append("\nEdificios\n");
        for (Map<String, Object> b : c.buildings) {
            Map<String, Object> cost = Json.map(b, "cost");
            sb.append(String.format(Locale.ROOT, "  %-18s era %d  %3.0f oro / %3.0f madera / %3.0f piedra%n",
                    Json.str(b, "name"), Json.integer(b, "era", 0),
                    Json.number(cost, "gold", 0), Json.number(cost, "wood", 0), Json.number(cost, "stone", 0)));
        }
        sb.append("\nEras\n");
        for (Map<String, Object> e : c.eras) {
            sb.append(String.format(Locale.ROOT, "  %d. %-20s tec %3.0f  prod %.2f  desde %d%n",
                    Json.integer(e, "id", 0), Json.str(e, "name"), Json.number(e, "tech", 0),
                    Json.number(e, "prod", 1), Json.integer(e, "start", c.startYear())));
        }
        return sb.toString();
    }

    public static String balanceReport(Content c, Validator.Result vr) {
        StringBuilder sb = new StringBuilder();
        sb.append("# Solaris History - informe de balance\n\n");
        sb.append("Generado por `java/tools` (motor de contenido Java).\n\n");
        sb.append("## Resumen\n\n");
        sb.append("| Metrica | Valor |\n|---|---:|\n");
        sb.append("| Provincias | ").append(c.provinces.size()).append(" |\n");
        sb.append("| Ciudades-estado | ").append(c.cityStates.size()).append(" |\n");
        sb.append("| Naciones fundables | ").append(c.nations.size()).append(" |\n");
        sb.append("| Culturas | ").append(c.cultures.size()).append(" |\n");
        sb.append("| Unidades | ").append(c.units.size()).append(" |\n");
        sb.append("| Edificios | ").append(c.buildings.size()).append(" |\n");
        sb.append("| Errores de validacion | ").append(vr.errors.size()).append(" |\n");
        sb.append("| Avisos de validacion | ").append(vr.warnings.size()).append(" |\n\n");

        double minInc = Double.MAX_VALUE, maxInc = 0, sumInc = 0;
        double minPop = Double.MAX_VALUE, maxPop = 0, sumPop = 0;
        Map<String, Double> byCulture = new TreeMap<>();
        for (Map<String, Object> p : c.provinces) {
            double inc = Validator.estimateProvinceIncome(c, p);
            double pop = Json.number(p, "pop", 0);
            minInc = Math.min(minInc, inc);
            maxInc = Math.max(maxInc, inc);
            sumInc += inc;
            minPop = Math.min(minPop, pop);
            maxPop = Math.max(maxPop, pop);
            sumPop += pop;
            byCulture.merge(Json.str(p, "culture"), inc, Double::sum);
        }
        int n = Math.max(1, c.provinces.size());
        sb.append("## Economia provincial (estimacion)\n\n");
        sb.append(String.format(Locale.ROOT,
                "Formula: `(poblacion*0.10 + desarrollo*0.05) * alimento_terreno * (0.8 + valor_recurso*0.35) * comercio_cultura`\n\n"));
        sb.append("| Metrica | Valor |\n|---|---:|\n");
        sb.append(String.format(Locale.ROOT, "| Ingreso mensual medio | %.2f oro |%n", sumInc / n));
        sb.append(String.format(Locale.ROOT, "| Ingreso mensual minimo | %.2f oro |%n", minInc));
        sb.append(String.format(Locale.ROOT, "| Ingreso mensual maximo | %.2f oro |%n", maxInc));
        sb.append(String.format(Locale.ROOT, "| Poblacion media | %.1fk |%n", sumPop / n));
        sb.append(String.format(Locale.ROOT, "| Poblacion minima / maxima | %.1fk / %.1fk |%n%n", minPop, maxPop));

        sb.append("### Ingreso estimado por cultura\n\n| Cultura | Oro/mes |\n|---|---:|\n");
        byCulture.entrySet().stream()
                .sorted(Map.Entry.<String, Double>comparingByValue().reversed())
                .forEach(e -> sb.append(String.format(Locale.ROOT, "| %s | %.2f |%n", e.getKey(), e.getValue())));
        sb.append('\n');

        sb.append("## Unidades\n\n| Unidad | Era | Hombres | Oro | Alimento | ATQ | DEF | (ATQ+DEF)/100 oro | Mant. |\n");
        sb.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|\n");
        for (Map<String, Object> u : c.units) {
            double gold = Json.number(u, "gold", 0);
            double power = Json.number(u, "atk", 0) + Json.number(u, "defense", 0);
            sb.append(String.format(Locale.ROOT, "| %s | %d | %d | %.0f | %.0f | %.0f | %.0f | %.1f | %.1f |%n",
                    Json.str(u, "name"), Json.integer(u, "era", 0), Json.integer(u, "men", 800), gold,
                    Json.number(u, "food", 0), Json.number(u, "atk", 0), Json.number(u, "defense", 0),
                    gold > 0 ? power / gold * 100 : 0, Json.number(u, "upkeep", 0)));
        }
        sb.append('\n');

        sb.append("## Edificios\n\n| Edificio | Era | Oro | Madera | Piedra | Efectos |\n");
        sb.append("|---|---:|---:|---:|---:|---|\n");
        for (Map<String, Object> b : c.buildings) {
            Map<String, Object> cost = Json.map(b, "cost");
            sb.append(String.format(Locale.ROOT, "| %s | %d | %.0f | %.0f | %.0f | %s |%n",
                    Json.str(b, "name"), Json.integer(b, "era", 0),
                    Json.number(cost, "gold", 0), Json.number(cost, "wood", 0), Json.number(cost, "stone", 0),
                    effects(Json.map(b, "mods"))));
        }
        sb.append('\n');

        sb.append("## Arranque de las ciudades jugables\n\n");
        sb.append("| Ciudad | Cultura | Provincia | Pop. | Ingreso capital | Oro inicial | Unidades asequibles |\n");
        sb.append("|---|---|---|---:|---:|---:|---:|\n");
        for (Map<String, Object> cs : c.cityStates) {
            if (!Json.bool(cs, "playable", false)) continue;
            Map<String, Object> p = c.province(Json.integer(cs, "capital", -1));
            double inc = p == null ? 0 : Validator.estimateProvinceIncome(c, p);
            int affordable = 0;
            for (Map<String, Object> u : c.units) {
                if (Json.number(u, "gold", 0) <= 140 && Json.number(u, "food", 0) <= 60) affordable++;
            }
            sb.append(String.format(Locale.ROOT, "| %s | %s | %s | %.1fk | %.2f | 140 | %d/%d |%n",
                    Json.str(cs, "name"), Json.str(cs, "culture"),
                    p == null ? "?" : Json.str(p, "name"),
                    p == null ? 0 : Json.number(p, "pop", 0), inc,
                    affordable, c.units.size()));
        }
        sb.append('\n');

        sb.append("## Naciones fundables por cultura\n\n| Cultura | Naciones |\n|---|---:|\n");
        Map<String, Integer> natByCulture = new TreeMap<>();
        for (Map<String, Object> nt : c.nations) natByCulture.merge(Json.str(nt, "culture"), 1, Integer::sum);
        natByCulture.forEach((k, v) -> sb.append(String.format(Locale.ROOT, "| %s | %d |%n", k, v)));
        sb.append('\n');

        sb.append("## Avisos de balance\n\n");
        int warns = 0;
        for (Map<String, Object> u : c.units) {
            double gold = Json.number(u, "gold", 0);
            if (gold > 0 && Json.integer(u, "men", 0) > 0) {
                double per100 = gold / Json.integer(u, "men", 1) * 100.0;
                if (per100 > 12) {
                    sb.append(String.format(Locale.ROOT, "- `%s` cuesta %.1f oro por cada 100 hombres.%n", Json.str(u, "id"), per100));
                    warns++;
                }
            }
        }
        for (Map<String, Object> b : c.buildings) {
            double total = Json.number(Json.map(b, "cost"), "gold", 0)
                    + Json.number(Json.map(b, "cost"), "wood", 0) * 0.5
                    + Json.number(Json.map(b, "cost"), "stone", 0) * 0.5;
            if (total > 120) {
                sb.append(String.format(Locale.ROOT, "- `%s` cuesta %.0f recursos equivalentes; revisa su retorno.%n",
                        Json.str(b, "id"), total));
                warns++;
            }
        }
        for (String w : vr.warnings) {
            sb.append("- ").append(w).append('\n');
            warns++;
        }
        if (warns == 0) sb.append("Sin avisos: el contenido esta dentro de los rangos esperados.\n");
        sb.append('\n');
        sb.append("## Validacion\n\n");
        if (vr.ok()) {
            sb.append("Sin errores. El motor puede cargar este contenido.\n");
        } else {
            sb.append("**Hay errores que impiden una carga fiable:**\n\n");
            for (String e : vr.errors) sb.append("- ERROR: ").append(e).append('\n');
        }
        return sb.toString();
    }

    private static String effects(Map<String, Object> mods) {
        if (mods.isEmpty()) return "-";
        StringBuilder sb = new StringBuilder();
        for (Map.Entry<String, Object> e : mods.entrySet()) {
            if (sb.length() > 0) sb.append(", ");
            sb.append(e.getKey()).append(' ');
            double v = ((Number) e.getValue()).doubleValue();
            sb.append(v >= 0 ? "+" : "").append(String.format(Locale.ROOT, "%.2f", v));
        }
        return sb.toString();
    }

    private static Map<String, Integer> count(List<Map<String, Object>> list, String key) {
        Map<String, Integer> out = new TreeMap<>();
        for (Map<String, Object> m : list) out.merge(Json.str(m, key, "?"), 1, Integer::sum);
        return out;
    }

    public static void exportCsv(Content c, Path outDir) throws IOException {
        Files.createDirectories(outDir);
        List<String> provinces = new ArrayList<>();
        provinces.add("id,name,region,terrain,resource,culture,lat,lon,pop,dev,coastal,adj_count,sea_adj_count,biome,continent,explore_sea_adj_count");
        for (Map<String, Object> p : c.provinces) {
            provinces.add(String.join(",", csv(Json.integer(p, "id", 0)), csv(Json.str(p, "name")),
                    csv(Json.str(p, "region")), csv(Json.str(p, "terrain")), csv(Json.str(p, "resource")),
                    csv(Json.str(p, "culture")), String.format(Locale.ROOT, "%.4f", Json.number(p, "lat", 0)),
                    String.format(Locale.ROOT, "%.4f", Json.number(p, "lon", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(p, "pop", 0)),
                    String.valueOf(Json.integer(p, "dev", 1)), String.valueOf(Json.bool(p, "coastal", false)),
                    String.valueOf(Json.list(p, "adj").size()), String.valueOf(Json.list(p, "sea_adj").size()),
                    csv(Json.str(p,"biome")),csv(Json.str(p,"continent")),String.valueOf(Json.list(p,"explore_sea_adj").size())));
        }
        writeLines(outDir.resolve("provinces.csv"), provinces);

        List<String> cities = new ArrayList<>();
        cities.add("id,name,culture,gov,capital,capital_name,playable,color");
        for (Map<String, Object> cs : c.cityStates) {
            Map<String, Object> p = c.province(Json.integer(cs, "capital", -1));
            cities.add(String.join(",", String.valueOf(Json.integer(cs, "id", 0)), csv(Json.str(cs, "name")),
                    csv(Json.str(cs, "culture")), csv(Json.str(cs, "gov")),
                    String.valueOf(Json.integer(cs, "capital", -1)), csv(p == null ? "" : Json.str(p, "name")),
                    String.valueOf(Json.bool(cs, "playable", false)), c.colorText(cs)));
        }
        writeLines(outDir.resolve("city_states.csv"), cities);

        List<String> nations = new ArrayList<>();
        nations.add("id,name,culture,gov,capital,era,color");
        for (Map<String, Object> n : c.nations) {
            nations.add(String.join(",", csv(Json.str(n, "id")), csv(Json.str(n, "name")),
                    csv(Json.str(n, "culture")), csv(Json.str(n, "gov")), csv(Json.str(n, "capital")),
                    String.valueOf(Json.integer(n, "era", 0)), c.colorText(n)));
        }
        writeLines(outDir.resolve("nations.csv"), nations);

        List<String> units = new ArrayList<>();
        units.add("id,name,era,gold,food,wood,stone,men,atk,defense,upkeep,speed,naval");
        for (Map<String, Object> u : c.units) {
            units.add(String.join(",", csv(Json.str(u, "id")), csv(Json.str(u, "name")),
                    String.valueOf(Json.integer(u, "era", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "gold", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "food", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "wood", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "stone", 0)),
                    String.valueOf(Json.integer(u, "men", 0)),
                    String.format(Locale.ROOT, "%.1f", Json.number(u, "atk", 0)),
                    String.format(Locale.ROOT, "%.1f", Json.number(u, "defense", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "upkeep", 0)),
                    String.format(Locale.ROOT, "%.2f", Json.number(u, "speed", 0)),
                    String.valueOf(Json.bool(u, "naval", false))));
        }
        writeLines(outDir.resolve("units.csv"), units);

        List<String> buildings = new ArrayList<>();
        buildings.add("id,name,era,gold,wood,stone");
        for (Map<String, Object> b : c.buildings) {
            Map<String, Object> cost = Json.map(b, "cost");
            buildings.add(String.join(",", csv(Json.str(b, "id")), csv(Json.str(b, "name")),
                    String.valueOf(Json.integer(b, "era", 0)),
                    String.valueOf(Json.number(cost, "gold", 0)),
                    String.valueOf(Json.number(cost, "wood", 0)),
                    String.valueOf(Json.number(cost, "stone", 0))));
        }
        writeLines(outDir.resolve("buildings.csv"), buildings);

        List<String> cultures = new ArrayList<>();
        cultures.add("id,name,short,color,farm,growth,tech,trade,mil,stab,naval,expansion");
        for (Map<String, Object> cu : c.cultures) {
            Map<String, Object> m = Json.map(cu, "mods");
            cultures.add(String.join(",", csv(Json.str(cu, "id")), csv(Json.str(cu, "name")),
                    csv(Json.str(cu, "short")), c.colorText(cu),
                    String.valueOf(Json.number(m, "farm", 1)), String.valueOf(Json.number(m, "growth", 1)),
                    String.valueOf(Json.number(m, "tech", 1)), String.valueOf(Json.number(m, "trade", 1)),
                    String.valueOf(Json.number(m, "mil", 1)), String.valueOf(Json.number(m, "stab", 1)),
                    String.valueOf(Json.number(m, "naval", 1)), String.valueOf(Json.number(m, "expansion", 1))));
        }
        writeLines(outDir.resolve("cultures.csv"), cultures);
    }

    private static String csv(int value) {
        return Integer.toString(value);
    }

    private static String csv(String value) {
        if (value.indexOf(',') >= 0 || value.indexOf('"') >= 0 || value.indexOf('\n') >= 0) {
            return '"' + value.replace("\"", "\"\"") + '"';
        }
        return value;
    }

    private static void writeLines(Path path, List<String> lines) throws IOException {
        Files.write(path, lines, StandardCharsets.UTF_8);
    }

    /** Manifiesto SHA-256 de los archivos de contenido. */
    public static Path manifest(Content c) throws Exception {
        List<Path> files = new ArrayList<>();
        try (var stream = Files.walk(c.dir)) {
            stream.filter(Files::isRegularFile)
                    .filter(p -> {
                        String name = p.getFileName().toString();
                        return !name.equals("manifest.sha256") && !p.startsWith(c.dir.resolve("derived"));
                    })
                    .sorted(Comparator.comparing(Path::toString))
                    .forEach(files::add);
        }
        List<String> lines = new ArrayList<>();
        for (Path p : files) {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] digest = md.digest(Files.readAllBytes(p));
            StringBuilder hex = new StringBuilder();
            for (byte b : digest) hex.append(String.format("%02x", b));
            String rel = c.dir.relativize(p).toString().replace('\\', '/');
            lines.add(hex + "  " + rel);
        }
        Path out = c.dir.resolve("manifest.sha256");
        Files.write(out, lines, StandardCharsets.UTF_8);
        return out;
    }

    public static void writeReports(Content c, Path outDir) throws IOException {
        Files.createDirectories(outDir);
        Validator.Result vr = Validator.validate(c);
        Files.writeString(outDir.resolve("stats.txt"), stats(c), StandardCharsets.UTF_8);
        Files.writeString(outDir.resolve("balance.md"), balanceReport(c, vr), StandardCharsets.UTF_8);
        Files.writeString(outDir.resolve("validation.txt"), validationText(vr), StandardCharsets.UTF_8);
    }

    public static String validationText(Validator.Result vr) {
        StringBuilder sb = new StringBuilder();
        sb.append("Solaris History - validacion de contenido\n");
        sb.append("=========================================\n\n");
        sb.append("Errores: ").append(vr.errors.size()).append('\n');
        for (String e : vr.errors) sb.append("  [ERROR] ").append(e).append('\n');
        sb.append("Avisos: ").append(vr.warnings.size()).append('\n');
        for (String w : vr.warnings) sb.append("  [AVISO] ").append(w).append('\n');
        sb.append('\n');
        if (vr.ok()) sb.append("Resultado: OK, el contenido es coherente.\n");
        else sb.append("Resultado: FALLIDO. Corrige los errores antes de publicar.\n");
        return sb.toString();
    }

    public static Map<String, Object> summaryJson(Content c) {
        Map<String, Object> out = new LinkedHashMap<>();
        out.put("start_year", c.startYear());
        out.put("provinces", c.provinces.size());
        out.put("city_states", c.cityStates.size());
        out.put("nations", c.nations.size());
        out.put("cultures", c.cultures.size());
        out.put("units", c.units.size());
        out.put("buildings", c.buildings.size());
        out.put("eras", c.eras.size());
        return out;
    }
}

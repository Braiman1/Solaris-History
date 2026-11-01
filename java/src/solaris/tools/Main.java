package solaris.tools;

import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Herramienta de contenido de Solaris History. */
public final class Main {
    private static final String VERSION = "0.1.0";

    public static void main(String[] args) {
        System.setOut(new PrintStream(System.out, true, StandardCharsets.UTF_8));
        System.setErr(new PrintStream(System.err, true, StandardCharsets.UTF_8));

        if (args.length == 0 || args[0].equals("-h") || args[0].equals("--help") || args[0].equals("help")) {
            usage();
            return;
        }
        String cmd = args[0].toLowerCase(Locale.ROOT);
        try {
            switch (cmd) {
                case "validate" -> {
                    Content c = load(args, 1);
                    Validator.Result r = Validator.validate(c);
                    System.out.print(Analyzer.validationText(r));
                    if (!r.ok()) System.exit(1);
                }
                case "stats" -> {
                    Content c = load(args, 1);
                    System.out.print(Analyzer.stats(c));
                }
                case "balance" -> {
                    Content c = load(args, 1);
                    Validator.Result r = Validator.validate(c);
                    Path out = optionalPath(args, 2, c.dir.resolve("derived").resolve("balance.md"));
                    Files.createDirectories(out.toAbsolutePath().getParent());
                    Files.writeString(out, Analyzer.balanceReport(c, r), StandardCharsets.UTF_8);
                    System.out.print(Analyzer.balanceReport(c, r));
                    System.out.println("\nInforme escrito en " + out.toAbsolutePath());
                }
                case "export" -> {
                    Content c = load(args, 1);
                    Path out = optionalPath(args, 2, c.dir.resolve("derived").resolve("csv"));
                    Analyzer.exportCsv(c, out);
                    System.out.println("CSV exportados en " + out.toAbsolutePath());
                }
                case "report" -> {
                    Content c = load(args, 1);
                    Path out = optionalPath(args, 2, c.dir.resolve("derived"));
                    Analyzer.writeReports(c, out);
                    System.out.println("Informes escritos en " + out.toAbsolutePath());
                    System.out.println("  stats.txt, balance.md, validation.txt");
                }
                case "manifest" -> {
                    Content c = load(args, 1);
                    Path out = Analyzer.manifest(c);
                    System.out.println("Manifiesto SHA-256 escrito en " + out.toAbsolutePath());
                }
                case "version", "--version" -> System.out.println("Solaris History Content Tools " + VERSION);
                default -> {
                    System.err.println("Comando desconocido: " + args[0]);
                    usage();
                    System.exit(2);
                }
            }
        } catch (Exception e) {
            System.err.println("ERROR: " + e.getMessage());
            System.exit(2);
        }
    }

    private static Content load(String[] args, int index) throws Exception {
        Path dir = args.length > index ? Path.of(args[index]) : defaultContentDir();
        return Content.load(dir);
    }

    private static Path optionalPath(String[] args, int index, Path def) {
        return args.length > index ? Path.of(args[index]) : def;
    }

    /** Busca content/ en el directorio actual o en sus padres. */
    private static Path defaultContentDir() {
        Path dir = Path.of("").toAbsolutePath();
        for (int i = 0; i < 5 && dir != null; i++) {
            Path candidate = dir.resolve("content");
            if (Files.isRegularFile(candidate.resolve("world.json"))) return candidate;
            dir = dir.getParent();
        }
        return Path.of("content");
    }

    private static void usage() {
        System.out.println("""
                Solaris History - herramientas de contenido (Java)
                Uso: java -jar SolarisTools.jar <comando> [carpeta_contenido] [salida]

                Comandos:
                  validate    Comprueba ids, referencias, adyacencias, costes y datos historicos.
                  stats       Resumen del mundo, culturas, unidades, edificios y eras.
                  balance     Auditoria economica y de balance; escribe content/derived/balance.md.
                  export      Exporta provincias, ciudades, naciones, unidades y edificios a CSV.
                  report      Genera stats.txt, balance.md y validation.txt en content/derived.
                  manifest    Calcula un manifiesto SHA-256 de todos los archivos de contenido.
                  version     Muestra la version de la herramienta.

                Sin carpeta de contenido se busca 'content' en el directorio actual o en sus padres.
                """);
    }

    private Main() {
    }
}

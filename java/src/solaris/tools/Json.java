package solaris.tools;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Parser JSON minimo sin dependencias externas.
 * Los objetos se representan como Map, las listas como List, los numeros como Double.
 */
public final class Json {
    private final String src;
    private int pos;

    private Json(String src) {
        this.src = src;
    }

    public static Object parse(String text) {
        Json p = new Json(text);
        p.skipWs();
        Object value = p.readValue();
        p.skipWs();
        if (p.pos != p.src.length()) {
            throw p.error("contenido extra despues del valor principal");
        }
        return value;
    }

    private IllegalArgumentException error(String msg) {
        int line = 1, col = 1;
        for (int i = 0; i < pos && i < src.length(); i++) {
            if (src.charAt(i) == '\n') { line++; col = 1; } else { col++; }
        }
        return new IllegalArgumentException("JSON invalido en linea " + line + ", columna " + col + ": " + msg);
    }

    private void skipWs() {
        while (pos < src.length()) {
            char c = src.charAt(pos);
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') pos++;
            else break;
        }
    }

    private Object readValue() {
        if (pos >= src.length()) throw error("se esperaba un valor");
        char c = src.charAt(pos);
        switch (c) {
            case '{': return readObject();
            case '[': return readArray();
            case '"': return readString();
            case 't':
                expect("true");
                return Boolean.TRUE;
            case 'f':
                expect("false");
                return Boolean.FALSE;
            case 'n':
                expect("null");
                return null;
            default:
                if (c == '-' || (c >= '0' && c <= '9')) return readNumber();
                throw error("caracter inesperado '" + c + "'");
        }
    }

    private void expect(String token) {
        if (!src.startsWith(token, pos)) throw error("se esperaba '" + token + "'");
        pos += token.length();
    }

    private Map<String, Object> readObject() {
        Map<String, Object> out = new LinkedHashMap<>();
        pos++; // {
        skipWs();
        if (pos < src.length() && src.charAt(pos) == '}') { pos++; return out; }
        while (true) {
            skipWs();
            if (pos >= src.length() || src.charAt(pos) != '"') throw error("se esperaba una clave");
            String key = readString();
            skipWs();
            if (pos >= src.length() || src.charAt(pos) != ':') throw error("se esperaba ':'");
            pos++;
            skipWs();
            out.put(key, readValue());
            skipWs();
            if (pos >= src.length()) throw error("objeto sin cerrar");
            char c = src.charAt(pos++);
            if (c == '}') return out;
            if (c != ',') throw error("se esperaba ',' o '}'");
        }
    }

    private List<Object> readArray() {
        List<Object> out = new ArrayList<>();
        pos++; // [
        skipWs();
        if (pos < src.length() && src.charAt(pos) == ']') { pos++; return out; }
        while (true) {
            skipWs();
            out.add(readValue());
            skipWs();
            if (pos >= src.length()) throw error("lista sin cerrar");
            char c = src.charAt(pos++);
            if (c == ']') return out;
            if (c != ',') throw error("se esperaba ',' o ']'");
        }
    }

    private String readString() {
        pos++; // "
        StringBuilder sb = new StringBuilder();
        while (pos < src.length()) {
            char c = src.charAt(pos++);
            if (c == '"') return sb.toString();
            if (c == '\\') {
                if (pos >= src.length()) throw error("escape incompleto");
                char e = src.charAt(pos++);
                switch (e) {
                    case '"': sb.append('"'); break;
                    case '\\': sb.append('\\'); break;
                    case '/': sb.append('/'); break;
                    case 'b': sb.append('\b'); break;
                    case 'f': sb.append('\f'); break;
                    case 'n': sb.append('\n'); break;
                    case 'r': sb.append('\r'); break;
                    case 't': sb.append('\t'); break;
                    case 'u': {
                        if (pos + 4 > src.length()) throw error("escape unicode incompleto");
                        String hex = src.substring(pos, pos + 4);
                        try {
                            sb.append((char) Integer.parseInt(hex, 16));
                        } catch (NumberFormatException ex) {
                            throw error("escape unicode invalido");
                        }
                        pos += 4;
                        break;
                    }
                    default: throw error("escape desconocido '\\" + e + "'");
                }
            } else {
                sb.append(c);
            }
        }
        throw error("cadena sin cerrar");
    }

    private Double readNumber() {
        int start = pos;
        if (pos < src.length() && src.charAt(pos) == '-') pos++;
        while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
        if (pos < src.length() && src.charAt(pos) == '.') {
            pos++;
            while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
        }
        if (pos < src.length() && (src.charAt(pos) == 'e' || src.charAt(pos) == 'E')) {
            pos++;
            if (pos < src.length() && (src.charAt(pos) == '+' || src.charAt(pos) == '-')) pos++;
            while (pos < src.length() && Character.isDigit(src.charAt(pos))) pos++;
        }
        try {
            return Double.parseDouble(src.substring(start, pos));
        } catch (NumberFormatException ex) {
            throw error("numero invalido");
        }
    }

    // ------------------------------------------------------------------
    // accesores comodos
    // ------------------------------------------------------------------
    @SuppressWarnings("unchecked")
    public static Map<String, Object> obj(Object value) {
        if (value instanceof Map<?, ?> m) return (Map<String, Object>) m;
        return Map.of();
    }

    @SuppressWarnings("unchecked")
    public static List<Object> arr(Object value) {
        if (value instanceof List<?> l) return (List<Object>) l;
        return List.of();
    }

    public static Object get(Object value, String key) {
        return obj(value).get(key);
    }

    public static Map<String, Object> map(Object value, String key) {
        return obj(obj(value).get(key));
    }

    public static List<Object> list(Object value, String key) {
        return arr(obj(value).get(key));
    }

    public static String str(Object value, String key, String def) {
        Object v = obj(value).get(key);
        return v instanceof String s ? s : def;
    }

    public static String str(Object value, String key) {
        return str(value, key, "");
    }

    public static int integer(Object value, String key, int def) {
        Object v = obj(value).get(key);
        return v instanceof Number n ? (int) Math.round(n.doubleValue()) : def;
    }

    public static int integer(Object value, String key) {
        return integer(value, key, 0);
    }

    public static double number(Object value, String key, double def) {
        Object v = obj(value).get(key);
        return v instanceof Number n ? n.doubleValue() : def;
    }

    public static double number(Object value, String key) {
        return number(value, key, 0);
    }

    public static boolean bool(Object value, String key, boolean def) {
        Object v = obj(value).get(key);
        return v instanceof Boolean b ? b : def;
    }

    public static boolean bool(Object value, String key) {
        return bool(value, key, false);
    }
}

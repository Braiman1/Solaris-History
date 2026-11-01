// Solaris History - parser JSON minimo (sin dependencias)
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace sol {

class Json {
public:
    enum Type { Null, Bool, Number, String, Array, Object };

    Json() : type_(Null) {}
    static Json parse(const std::string& text, std::string* error = nullptr);

    Type type() const { return type_; }
    bool isNull() const { return type_ == Null; }
    bool isArray() const { return type_ == Array; }
    bool isObject() const { return type_ == Object; }
    size_t size() const { return type_ == Array ? arr_.size() : (type_ == Object ? obj_.size() : 0); }

    bool has(const std::string& k) const;
    const Json& operator[](const std::string& k) const;
    const Json& operator[](size_t i) const;

    double number(double def = 0.0) const { return type_ == Number ? num_ : def; }
    int integer(int def = 0) const { return type_ == Number ? (int)std::lround(num_) : def; }
    bool boolean(bool def = false) const { return type_ == Bool ? b_ : def; }
    std::string string(const std::string& def = "") const { return type_ == String ? str_ : def; }
    const std::vector<Json>& array() const { return arr_; }

private:
    friend struct JsonParser;
    Type type_;
    bool b_ = false;
    double num_ = 0;
    std::string str_;
    std::vector<Json> arr_;
    std::vector<std::pair<std::string, Json>> obj_;
};

struct JsonParser {
    const std::string& s;
    size_t i = 0;
    std::string err;

    explicit JsonParser(const std::string& src) : s(src) {}

    void ws() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
    }
    bool fail(const std::string& m) { if (err.empty()) err = m + " en offset " + std::to_string(i); return false; }

    bool utf8(unsigned cp, std::string& out) {
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
        else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
        return true;
    }

    bool parseString(std::string& out) {
        if (i >= s.size() || s[i] != '"') return fail("se esperaba cadena");
        ++i;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return true;
            if (c == '\\') {
                if (i >= s.size()) return fail("escape incompleto");
                char e = s[i++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (i + 4 > s.size()) return fail("\\u incompleto");
                        unsigned cp = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = s[i++]; cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                            else return fail("hex invalido");
                        }
                        utf8(cp, out);
                        break;
                    }
                    default: return fail("escape desconocido");
                }
            } else {
                out += c;
            }
        }
        return fail("cadena sin cerrar");
    }

    bool parseValue(Json& v) {
        ws();
        if (i >= s.size()) return fail("fin inesperado");
        char c = s[i];
        if (c == '{') {
            v.type_ = Json::Object;
            ++i; ws();
            if (i < s.size() && s[i] == '}') { ++i; return true; }
            while (true) {
                ws();
                std::string key;
                if (!parseString(key)) return false;
                ws();
                if (i >= s.size() || s[i] != ':') return fail("se esperaba ':'");
                ++i;
                Json child;
                if (!parseValue(child)) return false;
                v.obj_.push_back({key, std::move(child)});
                ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; return true; }
                return fail("objeto sin cerrar");
            }
        } else if (c == '[') {
            v.type_ = Json::Array;
            ++i; ws();
            if (i < s.size() && s[i] == ']') { ++i; return true; }
            while (true) {
                Json child;
                if (!parseValue(child)) return false;
                v.arr_.push_back(std::move(child));
                ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; return true; }
                return fail("array sin cerrar");
            }
        } else if (c == '"') {
            v.type_ = Json::String;
            return parseString(v.str_);
        } else if (c == 't' && s.compare(i, 4, "true") == 0) {
            v.type_ = Json::Bool; v.b_ = true; i += 4; return true;
        } else if (c == 'f' && s.compare(i, 5, "false") == 0) {
            v.type_ = Json::Bool; v.b_ = false; i += 5; return true;
        } else if (c == 'n' && s.compare(i, 4, "null") == 0) {
            v.type_ = Json::Null; i += 4; return true;
        } else if (c == '-' || (c >= '0' && c <= '9')) {
            size_t start = i;
            if (s[i] == '-') ++i;
            while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
            v.type_ = Json::Number;
            v.num_ = std::strtod(s.substr(start, i - start).c_str(), nullptr);
            return true;
        }
        return fail("valor invalido");
    }

    bool run(Json& v) {
        if (!parseValue(v)) return false;
        ws();
        if (i != s.size()) return fail("texto sobrante");
        return true;
    }
};

inline const Json& Json::operator[](const std::string& k) const {
    static Json nul;
    if (type_ != Object) return nul;
    for (auto& p : obj_) if (p.first == k) return p.second;
    return nul;
}
inline bool Json::has(const std::string& k) const {
    if (type_ != Object) return false;
    for (auto& p : obj_) if (p.first == k) return true;
    return false;
}
inline const Json& Json::operator[](size_t idx) const {
    static Json nul;
    if (type_ != Array || idx >= arr_.size()) return nul;
    return arr_[idx];
}
inline Json Json::parse(const std::string& text, std::string* error) {
    JsonParser p(text);
    Json out;
    if (!p.run(out)) {
        if (error) *error = p.err;
        return Json();
    }
    return out;
}

} // namespace sol

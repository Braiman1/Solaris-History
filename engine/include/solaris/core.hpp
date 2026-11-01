// Solaris History - tipos basicos, matematicas, utilidades
#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <algorithm>

namespace sol {

constexpr float PI = 3.14159265358979f;
constexpr float DEG = PI / 180.0f;

inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }

struct Vec2 {
    float x = 0, y = 0;
    Vec2() {}
    Vec2(float a, float b) : x(a), y(b) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() {}
    Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(const Vec3& v) { float l = length(v); return l > 1e-6f ? v * (1.0f / l) : Vec3(); }
inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

inline Vec3 latLonToDir(float lat, float lon) {
    float p = lat * DEG, l = lon * DEG;
    return {std::cos(p) * std::sin(l), std::sin(p), std::cos(p) * std::cos(l)};
}
inline void dirToLatLon(const Vec3& d, float& lat, float& lon) {
    lat = std::asin(clampf(d.y, -1, 1)) / DEG;
    lon = std::atan2(d.x, d.z) / DEG;
}

struct Color {
    unsigned char r = 255, g = 255, b = 255, a = 255;
    Color() {}
    Color(int rr, int gg, int bb, int aa = 255)
        : r((unsigned char)std::clamp(rr, 0, 255)), g((unsigned char)std::clamp(gg, 0, 255)),
          b((unsigned char)std::clamp(bb, 0, 255)), a((unsigned char)std::clamp(aa, 0, 255)) {}
    Color withAlpha(int aa) const { return Color(r, g, b, aa); }
    Color lighten(float f) const { return Color((int)(r + (255 - r) * f), (int)(g + (255 - g) * f), (int)(b + (255 - b) * f), a); }
    Color darken(float f) const { return Color((int)(r * (1 - f)), (int)(g * (1 - f)), (int)(b * (1 - f)), a); }
};
inline Color mixColor(const Color& a, const Color& b, float t) {
    return Color((int)lerpf(a.r, b.r, t), (int)lerpf(a.g, b.g, t), (int)lerpf(a.b, b.b, t), (int)lerpf(a.a, b.a, t));
}

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    Rect() {}
    Rect(float xx, float yy, float ww, float hh) : x(xx), y(yy), w(ww), h(hh) {}
    bool contains(float px, float py) const { return px >= x && py >= y && px <= x + w && py <= y + h; }
};

struct Rng {
    uint64_t s = 88172645463325252ull;
    Rng() {}
    explicit Rng(uint64_t seed) : s(seed ? seed : 1) {}
    uint32_t next() {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return (uint32_t)(s >> 32);
    }
    float f() { return (next() & 0xFFFFFF) / 16777216.0f; }
    float range(float a, float b) { return a + (b - a) * f(); }
    int irange(int a, int b) { return b <= a ? a : a + (int)(next() % (uint32_t)(b - a + 1)); }
    template <typename T> const T& pick(const std::vector<T>& v) { return v[irange(0, (int)v.size() - 1)]; }
};

inline std::string readTextFile(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return std::string();
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string out;
    if (n > 0) {
        out.resize((size_t)n);
        size_t got = std::fread(&out[0], 1, (size_t)n, f);
        out.resize(got);
    }
    std::fclose(f);
    return out;
}

inline std::string fmt(const char* f, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, f);
    vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return std::string(buf);
}

inline std::string numStr(double v, int decimals = 1) {
    char b[64];
    if (decimals <= 0) snprintf(b, sizeof(b), "%.0f", v);
    else snprintf(b, sizeof(b), "%.*f", decimals, v);
    return b;
}

inline std::string intStr(long long v) {
    std::string s = std::to_string(v);
    // separador de miles
    for (int i = (int)s.size() - 3; i > 0; i -= 3)
        if (s[i - 1] != '-') s.insert((size_t)i, ".");
    return s;
}

// UTF-8 basico
inline std::vector<unsigned> utf8Codepoints(const std::string& s) {
    std::vector<unsigned> out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i++];
        if (c < 0x80) out.push_back(c);
        else if ((c >> 5) == 0x6 && i < s.size()) {
            unsigned cp = (c & 0x1F) << 6; cp |= ((unsigned char)s[i++] & 0x3F); out.push_back(cp);
        } else if ((c >> 4) == 0xE && i + 1 < s.size()) {
            unsigned cp = (c & 0x0F) << 12; cp |= ((unsigned char)s[i++] & 0x3F) << 6; cp |= ((unsigned char)s[i++] & 0x3F); out.push_back(cp);
        } else if ((c >> 3) == 0x1E && i + 2 < s.size()) {
            unsigned cp = (c & 0x07) << 18; cp |= ((unsigned char)s[i++] & 0x3F) << 12;
            cp |= ((unsigned char)s[i++] & 0x3F) << 6; cp |= ((unsigned char)s[i++] & 0x3F); out.push_back(cp);
        } else out.push_back('?');
    }
    return out;
}

inline std::string upperAscii(std::string s) {
    for (auto& c : s) if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    return s;
}

inline std::string monthName(int m) {
    static const char* n[] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
                              "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
    return n[std::clamp(m, 0, 11)];
}

inline std::string yearLabel(int year) {
    if (year < 0) return std::to_string(-year) + " a.C.";
    return std::to_string(year) + " d.C.";
}

inline std::string dateLabel(int year, int month) { return monthName(month) + " de " + yearLabel(year); }

inline int absMonth(int year, int month) { return (year + 3300) * 12 + month; }
inline void fromAbs(int abs, int& year, int& month) { year = abs / 12 - 3300; month = abs % 12; }

inline std::string shortNum(double v) {
    if (v >= 1000000) return numStr(v / 1000000.0, 1) + "M";
    if (v >= 1000) return numStr(v / 1000.0, 1) + "k";
    return numStr(v, v < 10 ? 1 : 0);
}

} // namespace sol

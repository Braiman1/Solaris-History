#include "solaris/gfx.hpp"
#include <GL/glu.h>
#include <windows.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace sol {

static inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// ---------------------------------------------------------------------------
// texturas
// ---------------------------------------------------------------------------
bool Texture::loadRaw(const std::string& path, int width, int height, bool repeat) {
    std::string data = readTextFile(path);
    if (data.size() != (size_t)width * height * 4) return false;
    w = width; h = height;
    if (!id) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    return true;
}

bool Texture::createEmpty(int width, int height, bool repeat) {
    w = width; h = height;
    if (!id) glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP);
    std::vector<unsigned char> zero((size_t)width * height * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, zero.data());
    return true;
}

// ---------------------------------------------------------------------------
// fuente (atlas generado con GDI)
// ---------------------------------------------------------------------------
bool Font::init() {
    const int W = cellW * cols, H = cellH * cols;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bmp || !bits) return false;
    HBITMAP oldBmp = (HBITMAP)SelectObject(dc, bmp);
    memset(bits, 0, (size_t)W * H * 4);

    HFONT fnt = CreateFontW(-40, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(dc, fnt);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    SetTextAlign(dc, TA_TOP | TA_LEFT);

    for (int i = 0; i < 256; ++i) {
        int cx = (i % cols) * cellW;
        int cy = (i / cols) * cellH;
        wchar_t ch[2] = {(wchar_t)i, 0};
        if (i >= 32) {
            TextOutW(dc, cx + 2, cy + 5, ch, 1);
            SIZE sz = {0, 0};
            GetTextExtentPoint32W(dc, ch, 1, &sz);
            glyphs[i].advance = (float)sz.cx;
            glyphs[i].valid = sz.cx > 0;
        } else {
            glyphs[i].advance = (i == ' ') ? 11.0f : 0.0f;
            glyphs[i].valid = false;
        }
        // la celda completa se usa como quad; el UV corresponde a la celda
        glyphs[i].u0 = (float)cx / W;
        glyphs[i].v0 = (float)cy / H;
        glyphs[i].u1 = (float)(cx + cellW) / W;
        glyphs[i].v1 = (float)(cy + cellH) / H;
    }
    SelectObject(dc, oldFont);
    DeleteObject(fnt);
    GdiFlush();

    unsigned char* px = (unsigned char*)bits;
    for (int i = 0; i < W * H; ++i) {
        unsigned char b = px[i * 4 + 0], g = px[i * 4 + 1], r = px[i * 4 + 2];
        unsigned char a = std::max(r, std::max(g, b));
        px[i * 4 + 0] = 255;
        px[i * 4 + 1] = 255;
        px[i * 4 + 2] = 255;
        px[i * 4 + 3] = a;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

    SelectObject(dc, oldBmp);
    DeleteObject(bmp);
    DeleteDC(dc);
    return true;
}

float Font::width(const std::string& text, float scale) const {
    float w = 0;
    for (unsigned cp : utf8Codepoints(text)) {
        if (cp > 255) cp = '?';
        w += (glyphs[cp].advance + 1.0f) * scale;
    }
    return w;
}

void Font::draw(const std::string& text, float x, float y, float scale, Color color, bool shadow) const {
    if (!tex) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    int passes = shadow ? 2 : 1;
    for (int pass = 0; pass < passes; ++pass) {
        bool sh = shadow && pass == 0;
        float ox = sh ? 1.6f : 0.0f;
        float oy = sh ? 1.8f : 0.0f;
        glColor4ub(sh ? 8 : color.r, sh ? 10 : color.g, sh ? 14 : color.b,
                   sh ? (unsigned char)(color.a * 0.75f) : color.a);
        float px = x + ox, py = y + oy;
        float cw = cellW * scale, ch = cellH * scale;
        glBegin(GL_QUADS);
        for (unsigned cp : utf8Codepoints(text)) {
            if (cp > 255) cp = '?';
            const Glyph& g = glyphs[cp];
            if (g.advance <= 0) { px += cw * 0.28f; continue; }
            glTexCoord2f(g.u0, g.v0); glVertex2f(px, py);
            glTexCoord2f(g.u1, g.v0); glVertex2f(px + cw, py);
            glTexCoord2f(g.u1, g.v1); glVertex2f(px + cw, py + ch);
            glTexCoord2f(g.u0, g.v1); glVertex2f(px, py + ch);
            px += (g.advance + 1.2f) * scale;
        }
        glEnd();
    }
}

// ---------------------------------------------------------------------------
// mapa politico
// ---------------------------------------------------------------------------
static bool isOceanPixel(const unsigned char* p) {
    int r = p[0], g = p[1], b = p[2];
    return b > r + 22 && b > g + 22;
}

bool PoliticalMap::build(const Content& content, const std::string& contentDir) {
    W = 2048; H = 1024;
    std::string raw = readTextFile(contentDir + "/textures/earth.rgba");
    if (raw.size() != (size_t)W * H * 4) return false;
    const unsigned char* earth = (const unsigned char*)raw.data();

    provAt.assign((size_t)W * H, -1);
    land.assign((size_t)W * H, 0);
    pixels.assign((size_t)W * H * 4, 0);
    for (int i = 0; i < W * H; ++i) land[(size_t)i] = isOceanPixel(earth + (size_t)i * 4) ? 0 : 1;

    // rejilla de candidatos 2x2 grados
    const int CW = 180, CH = 90;
    std::vector<std::array<int, 8>> cand((size_t)CW * CH);
    for (int cy = 0; cy < CH; ++cy) {
        float lat = 90.0f - (cy + 0.5f) * 2.0f;
        for (int cx = 0; cx < CW; ++cx) {
            float lon = -180.0f + (cx + 0.5f) * 2.0f;
            std::vector<std::pair<float, int>> dists;
            dists.reserve(content.provinces.size());
            for (auto& p : content.provinces) {
                float dlat = p.lat - lat;
                float dlon = std::fabs(p.lon - lon);
                if (dlon > 180) dlon = 360 - dlon;
                float dlonScaled = dlon * std::cos(lat * DEG);
                float d2 = dlat * dlat + dlonScaled * dlonScaled;
                dists.push_back({d2, p.id});
            }
            std::partial_sort(dists.begin(), dists.begin() + std::min<size_t>(8, dists.size()), dists.end());
            for (int k = 0; k < 8; ++k) cand[(size_t)cy * CW + cx][(size_t)k] = k < (int)dists.size() ? dists[(size_t)k].second : -1;
        }
    }

    for (int y = 0; y < H; ++y) {
        float lat = 90.0f - (y + 0.5f) * (180.0f / H);
        int cy = std::clamp((int)((90.0f - lat) / 2.0f), 0, CH - 1);
        float coslat = std::cos(lat * DEG);
        for (int x = 0; x < W; ++x) {
            size_t idx = (size_t)y * W + x;
            if (!land[idx]) continue;
            float lon = -180.0f + (x + 0.5f) * (360.0f / W);
            int cx = std::clamp((int)((lon + 180.0f) / 2.0f), 0, CW - 1);
            float best = 1e9f;
            int bestId = -1;
            for (int k = 0; k < 8; ++k) {
                int pid = cand[(size_t)cy * CW + cx][(size_t)k];
                if (pid < 0) continue;
                const ProvinceDef& p = content.provinces[(size_t)pid];
                float dlat = p.lat - lat;
                float dlon = std::fabs(p.lon - lon);
                if (dlon > 180) dlon = 360 - dlon;
                float d2 = dlat * dlat + (dlon * coslat) * (dlon * coslat);
                if (d2 < best) { best = d2; bestId = pid; }
            }
            if (bestId >= 0 && best < 105.0f) provAt[idx] = (int16_t)bestId;
        }
    }

    // bordes y costas
    borderVerts.clear();
    coastVerts.clear();
    auto toPoint = [&](float lat, float lon, float r = 1.0035f) {
        Vec3 d = latLonToDir(lat, lon);
        return d * r;
    };
    for (int y = 0; y < H; ++y) {
        float lat0 = 90.0f - y * (180.0f / H);
        float lat1 = 90.0f - (y + 1) * (180.0f / H);
        for (int x = 0; x < W; ++x) {
            size_t i = (size_t)y * W + x;
            int a = provAt[i];
            // derecha
            int xr = (x + 1) % W;
            int b = provAt[(size_t)y * W + xr];
            if (a != b && (a >= 0 || b >= 0)) {
                float lon = -180.0f + (x + 1) * (360.0f / W);
                Vec3 p1 = toPoint(lat0, lon), p2 = toPoint(lat1, lon);
                bool coast = (a >= 0 && !land[(size_t)y * W + xr]) || (b >= 0 && !land[i]);
                auto& list = coast ? coastVerts : borderVerts;
                list.push_back(p1); list.push_back(p2);
            }
            // abajo
            if (y + 1 < H) {
                int c = provAt[(size_t)(y + 1) * W + x];
                if (a != c && (a >= 0 || c >= 0)) {
                    float lon0 = -180.0f + x * (360.0f / W);
                    float lon1 = -180.0f + (x + 1) * (360.0f / W);
                    Vec3 p1 = toPoint(lat1, lon0), p2 = toPoint(lat1, lon1);
                    bool coast = (a >= 0 && !land[(size_t)(y + 1) * W + x]) || (c >= 0 && !land[i]);
                    auto& list = coast ? coastVerts : borderVerts;
                    list.push_back(p1); list.push_back(p2);
                }
            }
        }
    }

    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return true;
}

int PoliticalMap::pixelAt(float lat, float lon) const {
    int x = (int)((lon + 180.0f) / 360.0f * W);
    int y = (int)((90.0f - lat) / 180.0f * H);
    x = ((x % W) + W) % W;
    y = std::clamp(y, 0, H - 1);
    return (int)y * W + x;
}

int PoliticalMap::pick(float lat, float lon) const {
    int i = pixelAt(lat, lon);
    if (i < 0 || i >= (int)provAt.size()) return -1;
    return provAt[(size_t)i];
}

void PoliticalMap::paint(const Sim& sim, int selected) {
    if (pixels.size() != (size_t)W * H * 4) return;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            size_t i = (size_t)y * W + x;
            size_t o = i * 4;
            int pid = provAt[i];
            if (pid < 0 || !land[i]) {
                pixels[o + 3] = 0;
                continue;
            }
            Color c = sim.provinceMapColor(pid);
            const Province& p = sim.prov[(size_t)pid];
            float light = 0.92f + (p.def % 5) * 0.015f;
            c = Color((int)(c.r * light), (int)(c.g * light), (int)(c.b * light));
            int alpha = p.owner > 0 ? 108 : 46;
            if (p.owner > 0 && p.owner == sim.player) alpha = 125;
            if (pid == selected) {
                c = c.lighten(0.42f);
                alpha = 190;
            }
            pixels[o + 0] = c.r;
            pixels[o + 1] = c.g;
            pixels[o + 2] = c.b;
            pixels[o + 3] = (unsigned char)alpha;
        }
    }
}

void PoliticalMap::upload() {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

void PoliticalMap::buildDisplayLists() {
    if (!borderList) borderList = glGenLists(1);
    glNewList(borderList, GL_COMPILE);
    glBegin(GL_LINES);
    for (auto& v : borderVerts) glVertex3f(v.x, v.y, v.z);
    glEnd();
    glEndList();
    if (!coastList) coastList = glGenLists(1);
    glNewList(coastList, GL_COMPILE);
    glBegin(GL_LINES);
    for (auto& v : coastVerts) glVertex3f(v.x, v.y, v.z);
    glEnd();
    glEndList();
}

void PoliticalMap::sync(const Sim& sim, int selected) {
    uint64_t h = 1469598103934665603ull ^ (uint64_t)(selected + 1);
    for (auto& p : sim.prov) {
        h ^= (uint64_t)(p.owner + 3);
        h *= 1099511628211ull;
    }
    for (auto& n : sim.nations) {
        Color tone=sim.nationColor(n.id);
        h ^= (uint64_t)tone.r | ((uint64_t)tone.g << 8) | ((uint64_t)tone.b << 16) | ((uint64_t)n.alive << 24);
        h *= 1099511628211ull;
    }
    if (h == checksum) return;
    checksum = h;
    paint(sim, selected);
    upload();
}

// ---------------------------------------------------------------------------
// globo
// ---------------------------------------------------------------------------
static void addSphereVertex(float lat, float lon) {
    Vec3 d = latLonToDir(lat, lon);
    glNormal3f(d.x, d.y, d.z);
    glTexCoord2f((lon + 180.0f) / 360.0f, (90.0f - lat) / 180.0f);
    glVertex3f(d.x, d.y, d.z);
}

void Gfx::makeGlow() {
    const int N = 256;
    std::vector<unsigned char> px((size_t)N * N * 4, 0);
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            float dx = (x + 0.5f) / N * 2 - 1, dy = (y + 0.5f) / N * 2 - 1;
            float d = std::sqrt(dx * dx + dy * dy);
            float a = clamp01((1.0f - d) * 1.6f);
            a *= a * a;
            size_t o = ((size_t)y * N + x) * 4;
            px[o + 0] = 88; px[o + 1] = 142; px[o + 2] = 205;
            px[o + 3] = (unsigned char)(a * 150);
        }
    }
    glow.createEmpty(N, N, false);
    glBindTexture(GL_TEXTURE_2D, glow.id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}

bool Gfx::init(const Content& content, const std::string& contentDir) {
    Json credits = Json::parse(readTextFile(contentDir + "/media/attributions.json"));
    for (const auto& a : credits.array()) {
        MediaCredit credit;
        credit.title=a["file"].string(); credit.author=a["author"].string();
        credit.license=a["license"].string(); credit.source=a["source"].string();
        credit.note=a["note"].string(); credit.noteEs=a["note_es"].string(credit.note);
        mediaCredits[a["id"].string()]=credit;
    }
    Json assets = Json::parse(readTextFile(contentDir + "/media/index.json"));
    for (const auto& a : assets.array()) {
        std::string id = a["id"].string();
        // IDs come from the bundled catalogue, never arbitrary paths.
        if (id.empty() || id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos) continue;
        media[id].loadRaw(contentDir + "/media/" + id + ".rgba", 128, 128);
    }
    moon.loadRaw(contentDir+"/media/moon_surface.rgba",512,256,true);
    if (!earth.loadRaw(contentDir + "/textures/earth.rgba", 2048, 1024, true)) return false;
    if (!font.init()) return false;
    if (!map.build(content, contentDir)) return false;
    map.buildDisplayLists();
    makeGlow();

    // esfera
    sphereList = glGenLists(1);
    glNewList(sphereList, GL_COMPILE);
    for (int lat = -90; lat < 90; lat += 2) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int lon = -180; lon <= 180; lon += 2) {
            addSphereVertex((float)lat, (float)lon);
            addSphereVertex((float)(lat + 2), (float)lon);
        }
        glEnd();
    }
    glEndList();

    // circulo unitario para anillos
    circleList = glGenLists(1);
    glNewList(circleList, GL_COMPILE);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        float a = i / 48.0f * 2 * PI;
        glVertex3f(std::cos(a), std::sin(a), 0);
    }
    glEnd();
    glEndList();

    // estrellas
    Rng rng(77123);
    for (int i = 0; i < 900; ++i) {
        float z = rng.range(-1, 1);
        float a = rng.range(0, 2 * PI);
        float r = std::sqrt(std::max(0.0f, 1 - z * z));
        stars.push_back(Vec3(r * std::cos(a), z, r * std::sin(a)) * 40.0f);
        int t = rng.irange(0, 9);
        if (t < 6) starColors.push_back(Color(190, 200, 220, rng.irange(100, 210)));
        else if (t < 8) starColors.push_back(Color(230, 220, 190, rng.irange(120, 230)));
        else starColors.push_back(Color(160, 190, 255, rng.irange(140, 240)));
    }

    // iconos de unidad
    unitTex.resize(content.units.size());
    unitIcons.resize(content.units.size(), 0);
    for (size_t i = 0; i < content.units.size(); ++i) {
        std::string p = contentDir + "/textures/" + content.units[i].icon + ".raw";
        if (unitTex[i].loadRaw(p, 256, 256, false)) unitIcons[i] = unitTex[i].id;
    }
    return true;
}

void Gfx::setupCamera(int width, int height) const {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(44.0, (double)width / std::max(1, height), 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vec3 p = cam.pos();
    gluLookAt(p.x, p.y, p.z, 0, 0, 0, 0, 1, 0);
}

Vec3 Gfx::rayDirection(float mx, float my, int width, int height) const {
    float aspect = (float)width / std::max(1, height);
    float tanHalf = std::tan(44.0f * 0.5f * DEG);
    float ndcX = (mx / width) * 2.0f - 1.0f;
    float ndcY = 1.0f - (my / height) * 2.0f;
    Vec3 dir = cam.forward() + cam.right() * (ndcX * tanHalf * aspect) + cam.up() * (ndcY * tanHalf);
    return normalize(dir);
}

bool Gfx::project(const Vec3& p, int width, int height, float& sx, float& sy, bool& front) const {
    Vec3 o = cam.pos();
    Vec3 rel = p - o;
    Vec3 f = cam.forward();
    float z = dot(rel, f);
    front = z > 0.05f && dot(p, o) > 0.0f;
    if (z <= 0.05f) return false;
    float aspect = (float)width / std::max(1, height);
    float tanHalf = std::tan(44.0f * 0.5f * DEG);
    float nx = dot(rel, cam.right()) / (z * tanHalf * aspect);
    float ny = dot(rel, cam.up()) / (z * tanHalf);
    sx = (nx * 0.5f + 0.5f) * width;
    sy = (0.5f - ny * 0.5f) * height;
    return true;
}

GLuint Gfx::iconFor(int unitIdx) const {
    if (unitIdx < 0 || unitIdx >= (int)unitIcons.size()) return 0;
    return unitIcons[(size_t)unitIdx];
}

void Gfx::drawSelection(const Sim& sim, int selected, float time) {
    if (selected < 0 || selected >= (int)sim.prov.size()) return;
    const ProvinceDef& pd = sim.pdef(selected);
    Vec3 dir = latLonToDir(pd.lat, pd.lon);
    Vec3 north(0, 1, 0);
    Vec3 t1 = normalize(cross(dir, north));
    if (length(t1) < 0.01f) t1 = Vec3(1, 0, 0);
    Vec3 t2 = cross(dir, t1);
    float pulse = 0.024f + 0.005f * std::sin(time * 4.0f);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    for (int pass = 0; pass < 2; ++pass) {
        float r = 1.012f + pass * 0.004f;
        glColor4ub(232, 197, 122, pass == 0 ? 235 : 90);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 40; ++i) {
            float a = i / 40.0f * 2 * PI;
            Vec3 p = dir * r + t1 * (std::cos(a) * pulse * (pass + 1)) + t2 * (std::sin(a) * pulse * (pass + 1));
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
    glLineWidth(1.0f);
}

void Gfx::drawGlobe(const Sim& sim, int selected, float time, int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.020f, 0.028f, 0.045f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera(width, height);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, glow.id);
    glColor4ub(255, 255, 255, 255);
    {
        Vec3 r = cam.right() * 3.05f, u = cam.up() * 3.05f;
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-r.x - u.x, -r.y - u.y, -r.z - u.z);
        glTexCoord2f(1, 0); glVertex3f(r.x - u.x, r.y - u.y, r.z - u.z);
        glTexCoord2f(1, 1); glVertex3f(r.x + u.x, r.y + u.y, r.z + u.z);
        glTexCoord2f(0, 1); glVertex3f(-r.x + u.x, -r.y + u.y, -r.z + u.z);
        glEnd();
    }
    // estrellas
    glDisable(GL_TEXTURE_2D);
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < stars.size(); ++i) {
        const Color& c = starColors[i];
        glColor4ub(c.r, c.g, c.b, c.a);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();
    glPointSize(1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    // esfera base iluminada
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat amb[4] = {0.58f, 0.58f, 0.58f, 1.0f};
    GLfloat dif[4] = {0.62f, 0.62f, 0.60f, 1.0f};
    GLfloat lp[4] = {0.35f, 0.55f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glColor4ub(255, 255, 255, 255);
    if(showMoon) {
        glEnable(GL_NORMALIZE);
        glDisable(GL_TEXTURE_2D);
        // Keep the decorative Moon visible above the map, outside the side panels.
        float depth=1.5f,tanHalf=std::tan(22.f*DEG);
        Vec3 moonPos=cam.pos()+cam.forward()*depth+cam.right()*(.44f*depth*tanHalf*width/std::max(1,height))+cam.up()*(.72f*depth*tanHalf);
        glPushMatrix();glTranslatef(moonPos.x,moonPos.y,moonPos.z);glScalef(.055f,.055f,.055f);
        if(moon.id){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,moon.id);}
        glColor4ub(145,145,145,255);glCallList(sphereList);glPopMatrix();
        glEnable(GL_TEXTURE_2D);glColor4ub(255,255,255,255);
    }
    glBindTexture(GL_TEXTURE_2D, earth.id);
    glCallList(sphereList);

    // capa politica
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, map.tex);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glPushMatrix();
    glScalef(1.0008f, 1.0008f, 1.0008f);
    glCallList(sphereList);
    glPopMatrix();
    glDisable(GL_POLYGON_OFFSET_FILL);

    // bordes
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);
    glColor4ub(24, 20, 16, 150);
    glCallList(map.borderList);
    glColor4ub(210, 220, 225, 70);
    glLineWidth(1.3f);
    glCallList(map.coastList);
    glLineWidth(1.0f);

    drawSelection(sim, selected, time);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

} // namespace sol

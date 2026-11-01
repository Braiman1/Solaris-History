// Solaris History - graficos: globo terraqueo, texturas, fuente GDI, mapa politico
#pragma once
#include "content.hpp"
#include "sim.hpp"
#include <GL/gl.h>
#include <string>
#include <vector>

namespace sol {

struct Camera {
    float yaw = 0.38f;
    float pitch = 0.30f;
    float dist = 2.85f;

    Vec3 pos() const {
        float cp = std::cos(pitch);
        return {cp * std::sin(yaw) * dist, std::sin(pitch) * dist, cp * std::cos(yaw) * dist};
    }
    Vec3 forward() const { return normalize(-pos()); }
    Vec3 right() const { return normalize(cross(forward(), Vec3(0, 1, 0))); }
    Vec3 up() const { return cross(right(), forward()); }
    void orbit(float dx, float dy) {
        yaw -= dx * 0.0042f;
        pitch = clampf(pitch + dy * 0.0042f, -1.40f, 1.40f);
    }
    void zoom(float d) { dist = clampf(dist - d * 0.12f, 1.55f, 4.6f); }
};

struct Texture {
    GLuint id = 0;
    int w = 0, h = 0;
    bool loadRaw(const std::string& path, int width, int height, bool repeat = false);
    bool createEmpty(int width, int height, bool repeat = false);
    void bind() const { glBindTexture(GL_TEXTURE_2D, id); }
};

struct Glyph {
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    float advance = 0;
    bool valid = false;
};

struct Font {
    GLuint tex = 0;
    int cellW = 48, cellH = 56, cols = 16;
    Glyph glyphs[256];
    bool init();
    float width(const std::string& text, float scale) const;
    void draw(const std::string& text, float x, float y, float scale, Color color, bool shadow = false) const;
    float lineHeight(float scale) const { return cellH * scale * 0.82f; }
};

struct PoliticalMap {
    int W = 2048, H = 1024;
    std::vector<int16_t> provAt;
    std::vector<unsigned char> land;
    std::vector<unsigned char> pixels;
    std::vector<Vec3> borderVerts;
    std::vector<Vec3> coastVerts;
    GLuint tex = 0;
    GLuint borderList = 0, coastList = 0;
    uint64_t checksum = 0;

    bool build(const Content& content, const std::string& contentDir);
    void paint(const Sim& sim, int selected);
    void upload();
    void buildDisplayLists();
    void sync(const Sim& sim, int selected);
    int pick(float lat, float lon) const;
    int pixelAt(float lat, float lon) const;
};

struct Gfx {
    Camera cam;
    Texture earth;
    Texture glow;
    Font font;
    PoliticalMap map;
    GLuint sphereList = 0;
    GLuint circleList = 0;
    std::vector<GLuint> unitIcons;
    std::vector<Texture> unitTex;
    std::vector<Vec3> stars;
    std::vector<Color> starColors;

    bool init(const Content& content, const std::string& contentDir);
    void drawGlobe(const Sim& sim, int selected, float time, int width, int height);
    void drawSelection(const Sim& sim, int selected, float time);
    void setupCamera(int width, int height) const;
    Vec3 rayDirection(float mx, float my, int width, int height) const;
    bool project(const Vec3& p, int width, int height, float& sx, float& sy, bool& front) const;
    GLuint iconFor(int unitIdx) const;
    void makeGlow();
};

} // namespace sol

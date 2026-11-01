// Solaris History - interfaz inmediata (paneles, botones, texto, retratos)
#pragma once
#include "gfx.hpp"
#include "locale.hpp"
#include "platform.hpp"
#include <string>
#include <vector>

namespace sol {

namespace pal {
const Color bg(13, 16, 21, 255);
const Color panel(24, 27, 33, 246);
const Color panelTop(34, 38, 45, 250);
const Color panelSoft(30, 34, 40, 240);
const Color border(176, 144, 78, 170);
const Color borderSoft(120, 104, 76, 90);
const Color gold(231, 196, 116, 255);
const Color goldDim(186, 158, 96, 255);
const Color text(235, 229, 214, 255);
const Color dim(164, 158, 146, 255);
const Color faint(118, 114, 106, 255);
const Color good(126, 196, 122, 255);
const Color bad(216, 106, 88, 255);
const Color war(224, 148, 92, 255);
const Color blue(118, 170, 214, 255);
const Color dark(12, 14, 18, 230);
} // namespace pal

struct UI {
    Platform* P = nullptr;
    Gfx* G = nullptr;
    int w = 1600, h = 900;
    float mx = 0, my = 0;
    bool mouseDown = false;
    bool clicked = false;
    bool rightClicked = false;
    float wheel = 0;
    std::vector<Rect> hits;
    std::string tooltip;
    float tooltipX = 0, tooltipY = 0;

    void begin(Platform& p, Gfx& g);
    void end();
    void addHit(Rect r) { hits.push_back(r); }
    bool overUI(float x, float y) const;
    bool hover(Rect r) const { return r.contains(mx, my); }

    void panel(Rect r, bool strong = true, bool block = true);
    void rect(Rect r, Color c);
    void gradient(Rect r, Color top, Color bottom);
    void outline(Rect r, Color c, float thickness = 1.0f);
    void cornerAccents(Rect r, Color c, float len = 12.0f);
    void divider(float x, float y, float w, Color c = pal::borderSoft);
    void text(const std::string& s, float x, float y, float scale, Color c, bool shadow = false);
    void textC(const std::string& s, float cx, float y, float scale, Color c, bool shadow = false);
    void textR(const std::string& s, float rx, float y, float scale, Color c, bool shadow = false);
    float textW(const std::string& s, float scale) const { return G->font.width(Locale::text(s), scale); }
    void textBlock(const std::string& s, float x, float y, float maxW, float scale, Color c,
                   float lineGap = 2.0f, int maxLines = 8);
    std::vector<std::string> wrap(const std::string& s, float maxW, float scale) const;

    bool button(const std::string& label, Rect r, bool enabled = true, Color accent = pal::gold,
                float scale = 0.38f);
    bool smallButton(const std::string& label, Rect r, bool enabled = true, Color accent = pal::goldDim);
    bool tab(const std::string& label, Rect r, bool active, float scale = 0.36f);
    void bar(Rect r, float t, Color fill, const std::string& label = "", float scale = 0.30f);
    void icon(GLuint tex, Rect r, Color tint = Color(255, 255, 255, 255), bool shadow = true);
    void portrait(Rect r, const Character& c, Color nationColor, bool ruler);
    void chip(const std::string& s, float x, float y, Color c, float scale = 0.30f);
    void tooltipAtMouse();
    void dimScreen();
};

} // namespace sol

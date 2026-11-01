#include "solaris/ui.hpp"
#include <GL/gl.h>
#include <algorithm>

namespace sol {

void UI::begin(Platform& p, Gfx& g) {
    P = &p; G = &g;
    w = p.w; h = p.h;
    mx = p.mx; my = p.my;
    mouseDown = p.mouseDown[0];
    clicked = p.mousePressed[0];
    rightClicked = p.mousePressed[1];
    wheel = p.wheel;
    hits.clear();
    tooltip.clear();

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
}

void UI::end() {
    if (!tooltip.empty()) tooltipAtMouse();
}

void UI::tooltipAtMouse() {
    if (tooltip.empty()) return;
    float scale = 0.33f;
    float tw = textW(tooltip, scale) + 18;
    float th = 26;
    float x = mx + 16, y = my + 18;
    if (x + tw > w - 8) x = w - tw - 8;
    if (y + th > h - 8) y = my - th - 12;
    rect({x + 2, y + 3, tw, th}, Color(0, 0, 0, 110));
    rect({x, y, tw, th}, Color(26, 25, 22, 248));
    outline({x, y, tw, th}, pal::border, 1);
    text(tooltip, x + 9, y + 5, scale, pal::text);
}

bool UI::overUI(float x, float y) const {
    for (auto& r : hits) if (r.contains(x, y)) return true;
    return false;
}

void UI::rect(Rect r, Color c) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(r.x, r.y);
    glVertex2f(r.x + r.w, r.y);
    glVertex2f(r.x + r.w, r.y + r.h);
    glVertex2f(r.x, r.y + r.h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void UI::gradient(Rect r, Color top, Color bottom) {
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4ub(top.r, top.g, top.b, top.a);
    glVertex2f(r.x, r.y);
    glVertex2f(r.x + r.w, r.y);
    glColor4ub(bottom.r, bottom.g, bottom.b, bottom.a);
    glVertex2f(r.x + r.w, r.y + r.h);
    glVertex2f(r.x, r.y + r.h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void UI::outline(Rect r, Color c, float t) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(r.x, r.y); glVertex2f(r.x + r.w, r.y); glVertex2f(r.x + r.w, r.y + t); glVertex2f(r.x, r.y + t);
    glVertex2f(r.x, r.y + r.h - t); glVertex2f(r.x + r.w, r.y + r.h - t); glVertex2f(r.x + r.w, r.y + r.h); glVertex2f(r.x, r.y + r.h);
    glVertex2f(r.x, r.y); glVertex2f(r.x + t, r.y); glVertex2f(r.x + t, r.y + r.h); glVertex2f(r.x, r.y + r.h);
    glVertex2f(r.x + r.w - t, r.y); glVertex2f(r.x + r.w, r.y); glVertex2f(r.x + r.w, r.y + r.h); glVertex2f(r.x + r.w - t, r.y + r.h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void UI::cornerAccents(Rect r, Color c, float len) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    float t = 1.6f;
    // esquinas
    glVertex2f(r.x, r.y); glVertex2f(r.x + len, r.y); glVertex2f(r.x + len, r.y + t); glVertex2f(r.x, r.y + t);
    glVertex2f(r.x, r.y); glVertex2f(r.x + t, r.y); glVertex2f(r.x + t, r.y + len); glVertex2f(r.x, r.y + len);
    glVertex2f(r.x + r.w - len, r.y); glVertex2f(r.x + r.w, r.y); glVertex2f(r.x + r.w, r.y + t); glVertex2f(r.x + r.w - len, r.y + t);
    glVertex2f(r.x + r.w - t, r.y); glVertex2f(r.x + r.w, r.y); glVertex2f(r.x + r.w, r.y + len); glVertex2f(r.x + r.w - t, r.y + len);
    glVertex2f(r.x, r.y + r.h - t); glVertex2f(r.x + len, r.y + r.h - t); glVertex2f(r.x + len, r.y + r.h); glVertex2f(r.x, r.y + r.h);
    glVertex2f(r.x, r.y + r.h - len); glVertex2f(r.x + t, r.y + r.h - len); glVertex2f(r.x + t, r.y + r.h); glVertex2f(r.x, r.y + r.h);
    glVertex2f(r.x + r.w - len, r.y + r.h - t); glVertex2f(r.x + r.w, r.y + r.h - t); glVertex2f(r.x + r.w, r.y + r.h); glVertex2f(r.x + r.w - len, r.y + r.h);
    glVertex2f(r.x + r.w - t, r.y + r.h - len); glVertex2f(r.x + r.w, r.y + r.h - len); glVertex2f(r.x + r.w, r.y + r.h); glVertex2f(r.x + r.w - t, r.y + r.h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void UI::panel(Rect r, bool strong, bool block) {
    rect({r.x + 4, r.y + 5, r.w, r.h}, Color(0, 0, 0, 95));
    gradient(r, strong ? pal::panelTop : pal::panelSoft, pal::panel);
    outline(r, strong ? pal::border : pal::borderSoft, 1.0f);
    cornerAccents(r, strong ? pal::gold : pal::goldDim, 11);
    if (block) addHit(r);
}

void UI::divider(float x, float y, float wdt, Color c) {
    rect({x, y, wdt, 1}, c);
}

void UI::text(const std::string& s, float x, float y, float scale, Color c, bool shadow) {
    G->font.draw(s, x, y, scale, c, shadow);
}

void UI::textC(const std::string& s, float cx, float y, float scale, Color c, bool shadow) {
    G->font.draw(s, cx - textW(s, scale) * 0.5f, y, scale, c, shadow);
}

void UI::textR(const std::string& s, float rx, float y, float scale, Color c, bool shadow) {
    G->font.draw(s, rx - textW(s, scale), y, scale, c, shadow);
}

std::vector<std::string> UI::wrap(const std::string& s, float maxW, float scale) const {
    std::vector<std::string> lines;
    std::string line;
    std::vector<std::string> words;
    std::string cur;
    for (char c : s) {
        if (c == ' ') { if (!cur.empty()) words.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) words.push_back(cur);
    for (auto& wd : words) {
        std::string test = line.empty() ? wd : line + " " + wd;
        if (textW(test, scale) > maxW && !line.empty()) {
            lines.push_back(line);
            line = wd;
        } else line = test;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void UI::textBlock(const std::string& s, float x, float y, float maxW, float scale, Color c,
                   float lineGap, int maxLines) {
    auto lines = wrap(s, maxW, scale);
    float lh = G->font.lineHeight(scale) + lineGap;
    int n = std::min((int)lines.size(), maxLines);
    for (int i = 0; i < n; ++i) text(lines[(size_t)i], x, y + i * lh, scale, c);
}

bool UI::button(const std::string& label, Rect r, bool enabled, Color accent, float scale) {
    bool hov = enabled && hover(r);
    if (hov) P->setCursorHand(true);
    if (enabled && hov && P->mousePressed[0]) {
        gradient(r, Color(64, 54, 36, 255), Color(44, 38, 28, 255));
        outline(r, Color(120, 96, 52, 255), 1);
        return true;
    }
    if (enabled) {
        gradient(r, hov ? Color(54, 48, 36, 250) : Color(40, 37, 31, 245),
                 hov ? Color(38, 34, 27, 250) : Color(27, 26, 23, 245));
        outline(r, hov ? accent : Color(112, 98, 70, 170), 1);
        cornerAccents(r, hov ? accent.withAlpha(200) : Color(110, 95, 66, 100), 7);
    } else {
        gradient(r, Color(32, 32, 32, 220), Color(24, 24, 24, 220));
        outline(r, Color(64, 62, 58, 140), 1);
    }
    Color tc = enabled ? (hov ? Color(255, 240, 200) : pal::text) : pal::faint;
    textC(label, r.x + r.w * 0.5f, r.y + (r.h - G->font.cellH * scale) * 0.5f - 2, scale, tc);
    addHit(r);
    return false;
}

bool UI::smallButton(const std::string& label, Rect r, bool enabled, Color accent) {
    return button(label, r, enabled, accent, 0.30f);
}

bool UI::tab(const std::string& label, Rect r, bool active, float scale) {
    bool hov = hover(r);
    if (hov) P->setCursorHand(true);
    if (active) {
        rect(r, Color(52, 44, 32, 250));
        rect({r.x, r.y + r.h - 2, r.w, 2}, pal::gold);
    } else if (hov) {
        rect(r, Color(40, 38, 34, 220));
    }
    textC(label, r.x + r.w * 0.5f, r.y + (r.h - G->font.cellH * scale) * 0.5f - 1, scale,
          active ? pal::gold : (hov ? pal::text : pal::dim));
    addHit(r);
    return hov && P->mousePressed[0];
}

void UI::bar(Rect r, float t, Color fill, const std::string& label, float scale) {
    t = clampf(t, 0, 1);
    rect(r, Color(12, 13, 15, 220));
    if (t > 0) {
        Color top = fill.lighten(0.12f);
        gradient({r.x + 1, r.y + 1, (r.w - 2) * t, r.h - 2}, top, fill.darken(0.12f));
    }
    outline(r, Color(80, 74, 62, 150), 1);
    if (!label.empty()) {
        text(label, r.x + 7, r.y + (r.h - G->font.cellH * scale) * 0.5f - 2, scale, pal::text, true);
    }
}

void UI::icon(GLuint tex, Rect r, Color tint, bool shadow) {
    if (!tex) return;
    if (shadow) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glColor4ub(0, 0, 0, 80);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(r.x + 3, r.y + 4);
        glTexCoord2f(1, 0); glVertex2f(r.x + r.w + 3, r.y + 4);
        glTexCoord2f(1, 1); glVertex2f(r.x + r.w + 3, r.y + r.h + 4);
        glTexCoord2f(0, 1); glVertex2f(r.x + 3, r.y + r.h + 4);
        glEnd();
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4ub(tint.r, tint.g, tint.b, tint.a);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(r.x, r.y);
    glTexCoord2f(1, 0); glVertex2f(r.x + r.w, r.y);
    glTexCoord2f(1, 1); glVertex2f(r.x + r.w, r.y + r.h);
    glTexCoord2f(0, 1); glVertex2f(r.x, r.y + r.h);
    glEnd();
    glColor4ub(255, 255, 255, 255);
}

void UI::portrait(Rect r, const Character& c, Color nationColor, bool ruler) {
    float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f;
    float rad = std::min(r.w, r.h) * 0.5f;
    glDisable(GL_TEXTURE_2D);
    // fondo
    glColor4ub(nationColor.darken(0.55f).r, nationColor.darken(0.55f).g, nationColor.darken(0.55f).b, 255);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 40; ++i) {
        float a = i / 40.0f * 2 * PI;
        glVertex2f(cx + std::cos(a) * rad, cy + std::sin(a) * rad);
    }
    glEnd();
    // hombros
    glColor4ub(nationColor.r, nationColor.g, nationColor.b, 255);
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 24; ++i) {
        float a = PI + i / 24.0f * PI;
        glVertex2f(cx + std::cos(a) * rad * 1.02f, cy + 0.30f * rad + std::sin(a) * rad * 0.68f);
    }
    glEnd();
    // cabeza
    Color skin(206, 168, 128);
    for (int pass = 0; pass < 2; ++pass) {
        float hr = rad * (pass == 0 ? 0.40f : 0.33f);
        glColor4ub(pass == 0 ? 60 : skin.r, pass == 0 ? 44 : skin.g, pass == 0 ? 34 : skin.b, 255);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy - rad * 0.16f);
        for (int i = 0; i <= 24; ++i) {
            float a = i / 24.0f * 2 * PI;
            glVertex2f(cx + std::cos(a) * hr, cy - rad * 0.16f + std::sin(a) * hr);
        }
        glEnd();
    }
    // pelo
    glColor4ub(52, 40, 30, 255);
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 18; ++i) {
        float a = PI + i / 18.0f * PI;
        glVertex2f(cx + std::cos(a) * rad * 0.42f, cy - rad * 0.14f + std::sin(a) * rad * 0.42f);
    }
    glEnd();
    if (c.female) {
        glBegin(GL_QUADS);
        glVertex2f(cx - rad * 0.42f, cy - rad * 0.12f);
        glVertex2f(cx + rad * 0.42f, cy - rad * 0.12f);
        glVertex2f(cx + rad * 0.46f, cy + rad * 0.30f);
        glVertex2f(cx - rad * 0.46f, cy + rad * 0.30f);
        glEnd();
    }
    // aro
    glColor4ub(ruler ? 226 : 120, ruler ? 190 : 108, ruler ? 108 : 80, 255);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        float a = i / 48.0f * 2 * PI;
        glVertex2f(cx + std::cos(a) * (rad - 1), cy + std::sin(a) * (rad - 1));
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void UI::chip(const std::string& s, float x, float y, Color c, float scale) {
    float wd = textW(s, scale) + 14;
    rect({x, y, wd, 20}, c.withAlpha(36));
    outline({x, y, wd, 20}, c.withAlpha(130), 1);
    text(s, x + 7, y + 1, scale, c.withAlpha(255));
}

void UI::dimScreen() {
    rect({0, 0, (float)w, (float)h}, Color(4, 6, 9, 195));
    addHit({0, 0, (float)w, (float)h});
}

} // namespace sol

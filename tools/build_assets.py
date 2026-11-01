#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Solaris History - Procesador de recursos graficos.
- earth_src.png -> textures/earth.rgba (2048x1024, RGBA8888)
- Iconos de unidad dibujados de forma original (CC0) -> textures/<id>.rgba (256x256)
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import math

ROOT = Path(__file__).resolve().parent.parent
TEX = ROOT / "content" / "textures"
TEX.mkdir(parents=True, exist_ok=True)

S = 256
GRID = 100.0
K = S / GRID

BRONZE = (201, 154, 75)
BRONZE_L = (232, 197, 122)
BRONZE_D = (128, 88, 38)
DARK = (44, 32, 22)
LEATHER = (122, 78, 46)
RED = (142, 59, 47)
STEEL = (176, 184, 190)
STEEL_D = (96, 106, 116)


def sc(*pts):
    return [(x * K, y * K) for x, y in pts]


def poly(d, pts, fill=None, outline=None, width=2):
    d.polygon(sc(*pts), fill=fill, outline=outline, width=int(width))


def line(d, pts, fill, width=2):
    d.line(sc(*pts), fill=fill, width=int(width * K), joint="curve")


def ellipse(d, box, fill=None, outline=None, width=2):
    x0, y0, x1, y1 = box
    d.ellipse([x0 * K, y0 * K, x1 * K, y1 * K], fill=fill, outline=outline, width=int(width))


def base_canvas():
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def finish(draw, im, name):
    # sombra suave bajo la figura
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    sd.ellipse([22 * K, 90 * K, 78 * K, 99 * K], fill=(0, 0, 0, 90))
    shadow = shadow.filter(ImageFilter.GaussianBlur(4 * K / 2))
    im = Image.alpha_composite(shadow, im)
    im.save(TEX / f"{name}.png")
    (TEX / f"{name}.raw").write_bytes(im.tobytes())  # RGBA plano, listo para OpenGL
    return im


def spear(d, x0, y0, x1, y1, color=BRONZE_L):
    line(d, [(x0, y0), (x1, y1)], DARK, 2.6)
    line(d, [(x0, y0), (x1, y1)], color, 1.4)
    dx, dy = x1 - x0, y1 - y0
    n = math.hypot(dx, dy) or 1
    dx, dy = dx / n, dy / n
    tipx, tipy = x1 + dx * 4, y1 + dy * 4
    poly(d, [(x1 + dy * 2.1, y1 - dx * 2.1), (tipx, tipy),
             (x1 - dy * 2.1, y1 + dx * 2.1)], fill=STEEL, outline=DARK, width=0.9)


def make_soldier():
    im, d = base_canvas()
    spear(d, 80, 8, 46, 46)          # lanza en diagonal
    # piernas
    poly(d, [(42, 72), (49, 72), (47, 97), (41, 97)], fill=LEATHER, outline=DARK, width=1.4)
    poly(d, [(53, 72), (60, 72), (63, 97), (57, 97)], fill=LEATHER, outline=DARK, width=1.4)
    # torso / tunica
    poly(d, [(36, 44), (62, 44), (66, 76), (34, 76)], fill=BRONZE, outline=DARK, width=1.8)
    poly(d, [(40, 48), (58, 48), (60, 58), (38, 58)], fill=BRONZE_L)
    # cinturon
    line(d, [(35, 68), (65, 68)], DARK, 3.2)
    line(d, [(35, 68), (65, 68)], BRONZE_D, 1.6)
    # brazo con lanza
    line(d, [(58, 48), (72, 52), (76, 42)], LEATHER, 5.5)
    # escudo redondo
    ellipse(d, (18, 48, 48, 80), fill=BRONZE_D, outline=DARK, width=2.2)
    ellipse(d, (21, 51, 45, 77), fill=RED, outline=BRONZE_L, width=1.8)
    ellipse(d, (29, 60, 37, 68), fill=BRONZE_L, outline=DARK, width=1.2)
    # cabeza y casco
    ellipse(d, (40, 24, 60, 44), fill=(196, 150, 108), outline=DARK, width=1.6)
    poly(d, [(39, 32), (40, 22), (50, 16), (61, 22), (61, 33), (54, 29), (46, 29)], fill=BRONZE_D, outline=DARK, width=1.6)
    poly(d, [(48, 16), (52, 16), (56, 8), (48, 10), (43, 9), (46, 13)], fill=RED, outline=DARK, width=1.2)  # cresta
    line(d, [(50, 32), (50, 37)], DARK, 1.2)
    # sandalias
    line(d, [(40, 96), (48, 96)], DARK, 3.2)
    line(d, [(56, 96), (64, 96)], DARK, 3.2)
    return finish(d, im, "soldado")


def make_lancero():
    im, d = base_canvas()
    # lanza larga hacia arriba-derecha
    spear(d, 26, 96, 88, 6)
    # piernas
    poly(d, [(44, 70), (51, 70), (49, 97), (42, 97)], fill=LEATHER, outline=DARK, width=1.4)
    poly(d, [(55, 70), (62, 70), (66, 97), (59, 97)], fill=LEATHER, outline=DARK, width=1.4)
    # torso con coraza
    poly(d, [(38, 42), (64, 42), (68, 74), (34, 74)], fill=STEEL_D, outline=DARK, width=1.8)
    poly(d, [(42, 46), (60, 46), (62, 56), (40, 56)], fill=STEEL)
    line(d, [(35, 64), (67, 64)], BRONZE, 2.2)
    # escudo grande
    poly(d, [(16, 44), (38, 40), (44, 62), (38, 84), (18, 80), (12, 62)], fill=BRONZE_D, outline=DARK, width=2.2)
    poly(d, [(19, 48), (35, 45), (39, 62), (35, 79), (20, 76), (16, 62)], fill=RED, outline=BRONZE_L, width=1.6)
    poly(d, [(24, 58), (32, 58), (28, 70)], fill=BRONZE_L, outline=DARK, width=1.0)
    # brazo
    line(d, [(60, 48), (74, 55)], LEATHER, 5.5)
    # cabeza con casco corintio
    ellipse(d, (43, 25, 60, 43), fill=(196, 150, 108), outline=DARK, width=1.4)
    poly(d, [(41, 34), (42, 24), (50, 18), (60, 23), (62, 36), (57, 41), (54, 33), (48, 33), (45, 41)], fill=BRONZE_D, outline=DARK, width=1.8)
    line(d, [(51, 33), (51, 38)], DARK, 1.4)
    poly(d, [(50, 18), (55, 18), (58, 10), (49, 12), (45, 9), (47, 14)], fill=RED, outline=DARK, width=1.1)
    return finish(d, im, "lancero")


def make_jinete():
    im, d = base_canvas()
    # caballo: cuerpo
    ellipse(d, (18, 46, 78, 72), fill=BRONZE_D, outline=DARK, width=2.0)
    # cuello y cabeza
    poly(d, [(62, 52), (78, 32), (84, 34), (82, 42), (74, 54)], fill=BRONZE_D, outline=DARK, width=2.0)
    poly(d, [(78, 30), (88, 26), (92, 32), (84, 38), (79, 36)], fill=BRONZE_D, outline=DARK, width=1.8)
    line(d, [(88, 27), (92, 24)], DARK, 2.0)  # orejas
    # patas
    for x0, x1 in ((24, 20), (32, 34), (58, 54), (66, 72)):
        line(d, [(x0, 66), (x1, 94)], BRONZE_D, 5.0)
        line(d, [(x1, 94), (x1 + 4, 94)], BRONZE_D, 4.0)
    # cola
    line(d, [(18, 54), (10, 62), (8, 74)], BRONZE_D, 3.0)
    # manta
    poly(d, [(34, 46), (56, 46), (58, 62), (32, 62)], fill=RED, outline=DARK, width=1.4)
    line(d, [(33, 54), (58, 54)], BRONZE_L, 1.6)
    # jinete
    poly(d, [(38, 30), (50, 30), (53, 50), (39, 50)], fill=BRONZE, outline=DARK, width=1.6)
    ellipse(d, (39, 18, 51, 32), fill=(196, 150, 108), outline=DARK, width=1.4)
    poly(d, [(38, 24), (39, 17), (45, 13), (52, 18), (52, 25), (47, 22), (42, 22)], fill=BRONZE_D, outline=DARK, width=1.3)
    # lanza del jinete
    spear(d, 36, 52, 68, 6)
    # arnes
    line(d, [(46, 50), (62, 46)], LEATHER, 2.0)
    return finish(d, im, "jinete")


def make_barco():
    im, d = base_canvas()
    # olas
    for y in (88, 93):
        d.arc([6 * K, (y - 3) * K, 40 * K, (y + 3) * K], 190, 350, fill=(80, 130, 150, 220), width=int(1.6 * K))
        d.arc([38 * K, (y - 3) * K, 72 * K, (y + 3) * K], 190, 350, fill=(80, 130, 150, 220), width=int(1.6 * K))
        d.arc([70 * K, (y - 3) * K, 98 * K, (y + 3) * K], 190, 350, fill=(80, 130, 150, 220), width=int(1.6 * K))
    # casco
    poly(d, [(10, 62), (90, 62), (82, 80), (22, 80), (14, 74)], fill=BRONZE_D, outline=DARK, width=2.2)
    poly(d, [(14, 65), (88, 65), (81, 75), (24, 75), (18, 71)], fill=LEATHER)
    # proa y popa
    poly(d, [(88, 62), (94, 50), (91, 40), (95, 42), (98, 54), (94, 66)], fill=BRONZE, outline=DARK, width=1.8)
    line(d, [(12, 62), (8, 50), (12, 44)], BRONZE, 2.4)
    # remos
    for x in range(24, 80, 8):
        line(d, [(x, 74), (x - 4, 88)], BRONZE_L, 1.4)
    # mastil y vela
    line(d, [(50, 64), (50, 12)], DARK, 3.0)
    poly(d, [(26, 18), (50, 14), (50, 56), (28, 52)], fill=(226, 214, 186), outline=DARK, width=1.6)
    poly(d, [(50, 14), (72, 20), (70, 54), (50, 56)], fill=(208, 194, 164), outline=DARK, width=1.6)
    for y in (28, 38, 48):
        line(d, [(29, y), (49, y + 1)], RED, 1.2)
        line(d, [(51, y + 1), (69, y)], RED, 1.2)
    # escudo en la proa
    ellipse(d, (78, 52, 90, 64), fill=RED, outline=BRONZE_L, width=1.4)
    return finish(d, im, "barco")


def main():
    src = TEX / "earth_src.png"
    if not src.exists():
        raise SystemExit("Falta content/textures/earth_src.png")
    im = Image.open(src).convert("RGBA").resize((2048, 1024), Image.LANCZOS)
    (TEX / "earth.rgba").write_bytes(im.tobytes())
    im.resize((1024, 512)).save(TEX / "earth_preview.png")
    print("earth.rgba 2048x1024", (TEX / 'earth.rgba').stat().st_size)

    for fn in (make_soldier, make_lancero, make_jinete, make_barco):
        ic = fn()
        print("icono", fn.__name__, ic.size)
    print("OK")


if __name__ == "__main__":
    main()

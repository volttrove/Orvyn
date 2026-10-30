#!/usr/bin/env python3
"""
fontgen.py - Velora font generator (ESP32 / FastLED)

Jalankan:  python fontgen.py

Menghasilkan:
    font5x7.h        font5x7.cpp
    fontJP16x16.h    fontJP16x16.cpp

Butuh: Pillow  (pip install pillow)
       NotoSansJP-Regular.ttf di folder yang sama dengan script ini.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TTF_PATH = os.path.join(HERE, "NotoSansJP-Regular.ttf")

# Ambang binarisasi (0-255) untuk glyph Jepang. Makin kecil = garis makin tebal.
JP_THRESHOLD = 100


# ======================================================================
# DATA FONT ASCII 5x7  (format kolom: 1 byte = 1 kolom, kiri -> kanan)
# ASCII 32 (spasi) sampai 90 ('Z'), indeks = kode ASCII - 32
# Di drawChar: bit0 digambar di baris paling bawah, bit6 di paling atas.
# ======================================================================

FONT5X7 = [
    (" ", [0x00, 0x00, 0x00, 0x00, 0x00]),
    ("!", [0x00, 0x00, 0x5F, 0x00, 0x00]),
    ('"', [0x00, 0x07, 0x00, 0x07, 0x00]),
    ("#", [0x14, 0x7F, 0x14, 0x7F, 0x14]),
    ("$", [0x24, 0x2A, 0x7F, 0x2A, 0x12]),
    ("%", [0x23, 0x13, 0x08, 0x64, 0x62]),
    ("&", [0x36, 0x49, 0x55, 0x22, 0x50]),
    ("'", [0x00, 0x05, 0x03, 0x00, 0x00]),
    ("(", [0x00, 0x1C, 0x22, 0x41, 0x00]),
    (")", [0x00, 0x41, 0x22, 0x1C, 0x00]),
    ("*", [0x14, 0x08, 0x3E, 0x08, 0x14]),
    ("+", [0x08, 0x08, 0x3E, 0x08, 0x08]),
    (",", [0x00, 0x50, 0x30, 0x00, 0x00]),
    ("-", [0x08, 0x08, 0x08, 0x08, 0x08]),
    (".", [0x00, 0x60, 0x60, 0x00, 0x00]),
    ("/", [0x20, 0x10, 0x08, 0x04, 0x02]),
    ("0", [0x3E, 0x51, 0x49, 0x45, 0x3E]),
    ("1", [0x00, 0x42, 0x7F, 0x40, 0x00]),
    ("2", [0x42, 0x61, 0x51, 0x49, 0x46]),
    ("3", [0x21, 0x41, 0x45, 0x4B, 0x31]),
    ("4", [0x18, 0x14, 0x12, 0x7F, 0x10]),
    ("5", [0x27, 0x45, 0x45, 0x45, 0x39]),
    ("6", [0x3C, 0x4A, 0x49, 0x49, 0x30]),
    ("7", [0x01, 0x71, 0x09, 0x05, 0x03]),
    ("8", [0x36, 0x49, 0x49, 0x49, 0x36]),
    ("9", [0x06, 0x49, 0x49, 0x29, 0x1E]),
    (":", [0x00, 0x36, 0x36, 0x00, 0x00]),
    (";", [0x00, 0x56, 0x36, 0x00, 0x00]),
    ("<", [0x08, 0x14, 0x22, 0x41, 0x00]),
    ("=", [0x14, 0x14, 0x14, 0x14, 0x14]),
    (">", [0x00, 0x41, 0x22, 0x14, 0x08]),
    ("?", [0x02, 0x01, 0x51, 0x09, 0x06]),
    ("@", [0x32, 0x49, 0x79, 0x41, 0x3E]),
    ("A", [0x7E, 0x11, 0x11, 0x11, 0x7E]),
    ("B", [0x7F, 0x49, 0x49, 0x49, 0x36]),
    ("C", [0x3E, 0x41, 0x41, 0x41, 0x22]),
    ("D", [0x7F, 0x41, 0x41, 0x22, 0x1C]),
    ("E", [0x7F, 0x49, 0x49, 0x49, 0x41]),
    ("F", [0x7F, 0x09, 0x09, 0x09, 0x01]),
    ("G", [0x3E, 0x41, 0x49, 0x49, 0x7A]),
    ("H", [0x7F, 0x08, 0x08, 0x08, 0x7F]),
    ("I", [0x00, 0x41, 0x7F, 0x41, 0x00]),
    ("J", [0x20, 0x40, 0x41, 0x3F, 0x01]),
    ("K", [0x7F, 0x08, 0x14, 0x22, 0x41]),
    ("L", [0x7F, 0x40, 0x40, 0x40, 0x40]),
    ("M", [0x7F, 0x02, 0x0C, 0x02, 0x7F]),
    ("N", [0x7F, 0x04, 0x08, 0x10, 0x7F]),
    ("O", [0x3E, 0x41, 0x41, 0x41, 0x3E]),
    ("P", [0x7F, 0x09, 0x09, 0x09, 0x06]),
    ("Q", [0x3E, 0x41, 0x51, 0x21, 0x5E]),
    ("R", [0x7F, 0x09, 0x19, 0x29, 0x46]),
    ("S", [0x46, 0x49, 0x49, 0x49, 0x31]),
    ("T", [0x01, 0x01, 0x7F, 0x01, 0x01]),
    ("U", [0x3F, 0x40, 0x40, 0x40, 0x3F]),
    ("V", [0x1F, 0x20, 0x40, 0x20, 0x1F]),
    ("W", [0x3F, 0x40, 0x38, 0x40, 0x3F]),
    ("X", [0x63, 0x14, 0x08, 0x14, 0x63]),
    ("Y", [0x07, 0x08, 0x70, 0x08, 0x07]),
    ("Z", [0x61, 0x51, 0x49, 0x45, 0x43]),
]

assert len(FONT5X7) == 59  # ASCII 32..90


# ======================================================================
# FONT JEPANG 16x16
# ======================================================================

JP_GLYPHS = [
    ("FONT_YIN",  "音"),
    ("FONT_GAKU", "楽"),
    ("FONT_DEN",  "電"),
    ("FONT_CHI",  "池"),
]


def render_glyph16(font_path, ch):
    """Render 1 karakter ke bitmap 16x16 -> list 16 int (uint16, bit15 = kiri)."""
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        sys.exit("ERROR: Pillow belum terpasang. Jalankan: pip install pillow")

    resample = getattr(Image, "Resampling", Image).LANCZOS

    big = 256
    font = ImageFont.truetype(font_path, big)
    canvas = Image.new("L", (big * 2, big * 2), 0)
    ImageDraw.Draw(canvas).text((big // 2, big // 2), ch, font=font, fill=255)

    bbox = canvas.getbbox()
    if bbox is None:
        sys.exit("ERROR: karakter '%s' tidak ada di font (kosong)." % ch)

    crop = canvas.crop(bbox)
    w, h = crop.size
    scale = min(16.0 / w, 16.0 / h)
    nw = max(1, min(16, int(round(w * scale))))
    nh = max(1, min(16, int(round(h * scale))))
    small = crop.resize((nw, nh), resample)

    out = Image.new("L", (16, 16), 0)
    out.paste(small, ((16 - nw) // 2, (16 - nh) // 2))

    rows = []
    for y in range(16):
        bits = 0
        for x in range(16):
            if out.getpixel((x, y)) >= JP_THRESHOLD:
                bits |= 1 << (15 - x)
        rows.append(bits)
    return rows


# ======================================================================
# GENERATOR FILE C++
# ======================================================================

COMMON_MATRIX = """\
#ifndef MATRIX_WIDTH
#define MATRIX_WIDTH 16
#endif

#ifndef MATRIX_HEIGHT
#define MATRIX_HEIGHT 16
#endif
"""


def write_file(name, text):
    path = os.path.join(HERE, name)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("  dibuat: " + name)


FONT5X7_H = """\
#pragma once

#include <FastLED.h>

extern const uint8_t font5x7[][5];

void drawChar(
    char c,
    int x,
    int y,
    CRGB color
);

void drawString(
    const char *text,
    int x,
    int y,
    CRGB color
);
"""

FONT5X7_CPP_HEAD = """\
#include "font5x7.h"
#include <Arduino.h>

#define WIDTH 16
#define HEIGHT 16

uint16_t XYText(uint8_t x, uint8_t y)
{
    if (y & 1)
        return y * WIDTH + (WIDTH - 1 - x);
    else
        return y * WIDTH + x;
}

// dari main.cpp
extern uint16_t XY(uint8_t x, uint8_t y);
extern CRGB leds[];

// ASCII mulai dari spasi (32)
const uint8_t font5x7[][5] = {

"""

FONT5X7_CPP_TAIL = """\

};

void drawChar(char c, int x, int y, CRGB color)
{
    if (c < 32 || c > 90)
        c = ' ';

    const uint8_t *glyph = font5x7[c - 32];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = glyph[col];

        for (int row = 0; row < 7; row++)
        {
            if (line & (1 << row))
            {
                int xx = x + col;
                int yy = y + (6 - row);

                if (xx >= 0 && xx < WIDTH &&
                    yy >= 0 && yy < HEIGHT)
                {
                    leds[XYText(xx, yy)] = color;
                }
            }
        }
    }
}

void drawString(const char *text, int x, int y, CRGB color)
{
    while (*text)
    {
        drawChar(*text, x, y, color);

        x += 6;      // 5 pixel font + 1 spasi

        text++;
    }
}
"""


def gen_font5x7():
    write_file("font5x7.h", FONT5X7_H)

    entries = []
    for ch, cols in FONT5X7:
        # spasi sampai '/' (ASCII 32-47) diberi tanda kutip di komentar
        label = "'%s'" % ch if ord(ch) <= 47 else ch
        data = ",".join("0x%02X" % b for b in cols)
        entries.append("// %s\n{%s}" % (label, data))

    body = ",\n\n".join(entries)
    write_file("font5x7.cpp", FONT5X7_CPP_HEAD + body + FONT5X7_CPP_TAIL)


def gen_fontjp16x16():
    if not os.path.isfile(TTF_PATH):
        sys.exit("ERROR: 'NotoSansJP-Regular.ttf' tidak ditemukan di folder:\n  " + HERE)

    rendered = []
    for name, ch in JP_GLYPHS:
        rows = render_glyph16(TTF_PATH, ch)
        rendered.append((name, ch, rows))

        # preview di terminal
        print("\n  %s (%s)" % (name, ch))
        for r in rows:
            print("   " + "".join("#" if r & (1 << (15 - x)) else "." for x in range(16)))
    print()

    # ---- header ----
    h = []
    h.append("#ifndef FONTJP16X16_H\n#define FONTJP16X16_H\n\n")
    h.append("#include <Arduino.h>\n#include <FastLED.h>\n\n")
    h.append(COMMON_MATRIX + "\n")
    h.append("struct Glyph16\n{\n    uint16_t row[16];\n};\n\n")
    for name, _ in JP_GLYPHS:
        h.append("extern const Glyph16 %s PROGMEM;\n" % name)
    h.append("\nvoid drawGlyph16(\n    const Glyph16 &glyph,\n"
             "    int x,\n    int y,\n    CRGB color);\n\n")
    h.append("#endif // FONTJP16X16_H\n")
    write_file("fontJP16x16.h", "".join(h))

    # ---- cpp ----
    c = []
    c.append('#include "fontJP16x16.h"\n\n')
    c.append("extern uint16_t XY(uint8_t x, uint8_t y);\n")
    c.append("extern CRGB leds[];\n\n")
    for name, ch, rows in rendered:
        c.append("// %s\n" % ch)
        c.append("const Glyph16 %s PROGMEM =\n{\n    {\n" % name)
        for i, r in enumerate(rows):
            sep = "," if i < 15 else ""
            c.append("        0x%04X%s\n" % (r, sep))
        c.append("    }\n};\n\n")

    c.append("""\
void drawGlyph16(
    const Glyph16 &glyph,
    int x,
    int y,
    CRGB color)
{
    for (int row = 0; row < 16; row++)
    {
        uint16_t bits = pgm_read_word(&glyph.row[row]);

        for (int col = 0; col < 16; col++)
        {
            if (bits & (1 << (15 - col)))
            {
                int xx = x + col;
                int yy = y + row;

                if (xx >= 0 && xx < MATRIX_WIDTH &&
                    yy >= 0 && yy < MATRIX_HEIGHT)
                {
                    leds[XY(xx, yy)] = color;
                }
            }
        }
    }
}
""")
    write_file("fontJP16x16.cpp", "".join(c))


def main():
    print("Velora fontgen")
    gen_font5x7()
    gen_fontjp16x16()
    print("Selesai. 4 file dibuat di: " + HERE)


if __name__ == "__main__":
    main()
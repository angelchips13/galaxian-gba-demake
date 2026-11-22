#!/usr/bin/env python3
import os
import random
import struct
import sys

from PIL import Image, ImageDraw

ROM_PATH = "Galaxian.nes"
OUT_DIR = "graphics"
DEBUG_SHEET = "chr_sheet_debug.png"

# Índices de tile estimados de la CHR-ROM (0-511, ambas tablas de patrones).
# Cada sprite es una lista de fotogramas; cada fotograma es una lista de filas;
# cada fila es una lista de tiles. Un tile puede ser un entero o una tupla
# (indice, volteo_horizontal, volteo_vertical).
# Ajusta estos valores consultando chr_sheet_debug.png.
TILE_MAP = {
    "player_ship": {
        "palette": "player",
        "frames": [
            [[0x00, 0x01],
             [0x02, 0x03]],
        ],
    },
    "player_laser": {
        "palette": "laser",
        "frames": [
            [[0x04]],
        ],
    },
    "enemy_bee": {
        "palette": "bee",
        "frames": [
            [[0x10, 0x11],
             [0x12, 0x13]],
            [[0x14, 0x15],
             [0x16, 0x17]],
        ],
    },
    "enemy_butterfly": {
        "palette": "butterfly",
        "frames": [
            [[0x20, 0x21],
             [0x22, 0x23]],
            [[0x24, 0x25],
             [0x26, 0x27]],
        ],
    },
    "enemy_flagship": {
        "palette": "flagship",
        "frames": [
            [[0x30, 0x31],
             [0x32, 0x33]],
        ],
    },
    "explosion": {
        "palette": "explosion",
        "frames": [
            [[0x40, 0x41],
             [0x42, 0x43]],
            [[0x44, 0x45],
             [0x46, 0x47]],
            [[0x48, 0x49],
             [0x4A, 0x4B]],
            [[0x4C, 0x4D],
             [0x4E, 0x4F]],
        ],
    },
}

# Paletas de 4 colores (indice 0 = negro transparente #000000).
PALETTES = {
    "player":    [(0, 0, 0), (216, 40, 0), (252, 252, 252), (60, 120, 252)],
    "laser":     [(0, 0, 0), (252, 252, 252), (252, 224, 96), (216, 40, 0)],
    "bee":       [(0, 0, 0), (60, 120, 252), (252, 224, 96), (252, 252, 252)],
    "butterfly": [(0, 0, 0), (168, 0, 32), (252, 252, 252), (60, 120, 252)],
    "flagship":  [(0, 0, 0), (252, 224, 96), (216, 40, 0), (60, 120, 252)],
    "explosion": [(0, 0, 0), (216, 40, 0), (252, 160, 68), (252, 252, 252)],
}

DEBUG_PALETTE = [(0, 0, 0), (110, 110, 110), (190, 190, 190), (255, 255, 255)]

STAR_PALETTE = [
    (0, 0, 0),
    (72, 72, 96),
    (136, 136, 168),
    (252, 252, 252),
    (96, 160, 252),
    (252, 96, 96),
    (252, 224, 96),
]


def parse_ines(data):
    if len(data) < 16 or data[0:4] != b"NES\x1a":
        raise SystemExit("Error: cabecera iNES invalida (se esperaba 'NES\\x1A').")
    prg_banks = data[4]
    chr_banks = data[5]
    flags6 = data[6]
    flags7 = data[7]
    mapper = (flags7 & 0xF0) | (flags6 >> 4)
    trainer = 512 if (flags6 & 0x04) else 0
    chr_offset = 16 + trainer + prg_banks * 16384
    chr_size = chr_banks * 8192
    if chr_size == 0:
        raise SystemExit("Error: la ROM usa CHR-RAM, no hay CHR-ROM que extraer.")
    if chr_offset + chr_size > len(data):
        raise SystemExit("Error: la ROM esta truncada.")
    print("iNES: PRG=%d x16KB, CHR=%d x8KB, mapper=%d, trainer=%s"
          % (prg_banks, chr_banks, mapper, "si" if trainer else "no"))
    return data[chr_offset:chr_offset + chr_size]


def decode_tile(chr_data, index):
    offset = index * 16
    if offset + 16 > len(chr_data):
        raise SystemExit("Error: tile %d fuera de rango de la CHR-ROM." % index)
    plane0 = chr_data[offset:offset + 8]
    plane1 = chr_data[offset + 8:offset + 16]
    rows = []
    for y in range(8):
        row = []
        for x in range(8):
            bit = 7 - x
            row.append((((plane1[y] >> bit) & 1) << 1) | ((plane0[y] >> bit) & 1))
        rows.append(row)
    return rows


def build_debug_sheet(chr_data, path):
    tiles = len(chr_data) // 16
    cols = 16
    scale = 3
    cell_w = 8 * scale + 4
    cell_h = 8 * scale + 14
    rows = (tiles + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w, rows * cell_h), (24, 24, 40))
    draw = ImageDraw.Draw(sheet)
    for index in range(tiles):
        pixels = decode_tile(chr_data, index)
        tile_img = Image.new("RGB", (8, 8))
        tile_img.putdata([DEBUG_PALETTE[v] for row in pixels for v in row])
        tile_img = tile_img.resize((8 * scale, 8 * scale), Image.NEAREST)
        cx = (index % cols) * cell_w
        cy = (index // cols) * cell_h
        sheet.paste(tile_img, (cx + 2, cy + 2))
        draw.rectangle((cx + 1, cy + 1, cx + 2 + 8 * scale, cy + 2 + 8 * scale),
                       outline=(70, 70, 110))
        draw.text((cx + 2, cy + 8 * scale + 4), "%03X" % index, fill=(255, 255, 0))
    sheet.save(path)
    print("Hoja de depuracion guardada: %s (%d tiles)" % (path, tiles))


def make_palette_image(size, colors):
    img = Image.new("P", size, 0)
    flat = []
    for color in colors:
        flat.extend(color)
    while len(flat) < 48:
        flat.extend((0, 0, 0))
    img.putpalette(flat[:48])
    return img


def blit_tile(img, pixels, ox, oy, hflip, vflip):
    for y in range(8):
        for x in range(8):
            sx = 7 - x if hflip else x
            sy = 7 - y if vflip else y
            img.putpixel((ox + x, oy + y), pixels[sy][sx])


def build_sprite(chr_data, spec):
    frames = spec["frames"]
    rows = len(frames[0])
    cols = len(frames[0][0])
    frame_w = cols * 8
    frame_h = rows * 8
    img = make_palette_image((frame_w, frame_h * len(frames)), PALETTES[spec["palette"]])
    for f, frame in enumerate(frames):
        for r, row in enumerate(frame):
            for c, ref in enumerate(row):
                if isinstance(ref, tuple):
                    index, hflip, vflip = ref
                else:
                    index, hflip, vflip = ref, False, False
                blit_tile(img, decode_tile(chr_data, index),
                          c * 8, f * frame_h + r * 8, hflip, vflip)
    return img


def save_bmp_4bpp(img, path):
    if img.mode != "P":
        raise ValueError("La imagen debe estar en modo 'P'.")
    width, height = img.size
    raw = img.tobytes()
    if max(raw) > 15:
        raise ValueError("%s usa indices mayores que 15." % path)
    palette = list(img.getpalette() or [])[:48]
    while len(palette) < 48:
        palette.extend((0, 0, 0))
    row_bytes = ((width * 4 + 31) // 32) * 4
    pixel_data = bytearray()
    for y in range(height - 1, -1, -1):
        row = bytearray(row_bytes)
        base = y * width
        for x in range(width):
            value = raw[base + x]
            if x & 1:
                row[x >> 1] |= value
            else:
                row[x >> 1] |= value << 4
        pixel_data.extend(row)
    palette_bytes = bytearray()
    for i in range(16):
        r, g, b = palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]
        palette_bytes.extend((b, g, r, 0))
    offset = 14 + 40 + len(palette_bytes)
    file_size = offset + len(pixel_data)
    with open(path, "wb") as f:
        f.write(struct.pack("<2sIHHI", b"BM", file_size, 0, 0, offset))
        f.write(struct.pack("<IiiHHIIiiII", 40, width, height, 1, 4, 0,
                            len(pixel_data), 2835, 2835, 16, 16))
        f.write(palette_bytes)
        f.write(pixel_data)
    print("Exportado: %s (%dx%d)" % (path, width, height))


def build_starfield(path):
    rng = random.Random(0x6A1A)
    size = 256
    img = make_palette_image((size, size), STAR_PALETTE)
    placed = []

    def far_enough(x, y, dist):
        for px, py in placed:
            dx = min(abs(px - x), size - abs(px - x))
            dy = min(abs(py - y), size - abs(py - y))
            if max(dx, dy) < dist:
                return False
        return True

    def put(x, y, color):
        img.putpixel((x % size, y % size), color)

    total = 120
    count = 0
    attempts = 0
    while count < total and attempts < 5000:
        attempts += 1
        x = rng.randrange(size)
        y = rng.randrange(size)
        if not far_enough(x, y, 8):
            continue
        roll = rng.random()
        if roll < 0.55:
            put(x, y, 1)
        elif roll < 0.80:
            put(x, y, 2)
        elif roll < 0.92:
            put(x, y, 3)
            put(x + 1, y, 2)
        elif roll < 0.97:
            put(x, y, rng.choice((4, 5, 6)))
        else:
            put(x, y, 3)
            put(x - 1, y, 2)
            put(x + 1, y, 2)
            put(x, y - 1, 2)
            put(x, y + 1, 2)
        placed.append((x, y))
        count += 1
    save_bmp_4bpp(img, path)


def main():
    rom_path = sys.argv[1] if len(sys.argv) > 1 else ROM_PATH
    if not os.path.isfile(rom_path):
        raise SystemExit("Error: no se encuentra la ROM '%s'." % rom_path)
    with open(rom_path, "rb") as f:
        data = f.read()
    chr_data = parse_ines(data)
    os.makedirs(OUT_DIR, exist_ok=True)
    build_debug_sheet(chr_data, DEBUG_SHEET)
    for name, spec in TILE_MAP.items():
        img = build_sprite(chr_data, spec)
        save_bmp_4bpp(img, os.path.join(OUT_DIR, name + ".bmp"))
    build_starfield(os.path.join(OUT_DIR, "starfield_bg.bmp"))
    print("Extraccion completada.")


if __name__ == "__main__":
    main()
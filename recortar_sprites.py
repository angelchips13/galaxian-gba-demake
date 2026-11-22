import os
from PIL import Image

img = Image.open("hoja.png").convert("RGB")
os.makedirs("sprites_recortados", exist_ok=True)
os.makedirs("graphics", exist_ok=True)

def exportar_gba(col, fila, nombre_salida):
    # Calibracion exacta para imagen de 144 x 281 (compensa los 7 pixeles superiores)
    x = col * 16
    y = (fila * 16) - 7
    recorte = img.crop((x, y, x + 16, y + 16))
    
    salida = Image.new("P", (16, 16), 0)
    paleta = []
    mapa_colores = {(0, 0, 0): 0}
    
    for py in range(16):
        for px in range(16):
            r, g, b = recorte.getpixel((px, py))
            # Convierte el negro y el gris oscuro de fondo en negro puro transparente (0)
            es_gris = abs(r - g) < 12 and abs(g - b) < 12 and 40 <= r <= 85
            if (r < 30 and g < 30 and b < 30) or es_gris:
                rgb = (0, 0, 0)
            else:
                rgb = (r, g, b)
                
            if rgb not in mapa_colores and len(mapa_colores) < 16:
                mapa_colores[rgb] = len(mapa_colores)
                paleta.extend(rgb)
                
            salida.putpixel((px, py), mapa_colores.get(rgb, 1))
            
    while len(paleta) < 768:
        paleta.append(0)
        
    salida.putpalette(paleta)
    salida.save(f"sprites_recortados/{nombre_salida}.bmp")
    salida.save(f"graphics/{nombre_salida}.bmp")
    print(f"Guardado perfecto: {nombre_salida}.bmp")

# Exporta los 5 personajes calibrados al pixel
exportar_gba(0, 14, "player_ship")      # Nave blanca y roja del jugador
exportar_gba(0, 2,  "enemy_flagship")   # Nave insignia amarilla
exportar_gba(0, 6,  "enemy_butterfly")  # Escolta morada
exportar_gba(0, 10, "enemy_bee")        # Alienigena verde
exportar_gba(3, 13, "explosion")        # Explosion naranja

print("Listo! Revisa la carpeta 'sprites_recortados' y compila con make.")

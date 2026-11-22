#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_string_view.h"

namespace gx
{
    // ---------------------------------------------------------------- Capacidades fijas
    inline constexpr int MAX_ENEMIES = 24;
    inline constexpr int MAX_LASERS = 3;
    inline constexpr int MAX_ENEMY_BULLETS = 4;
    inline constexpr int MAX_EXPLOSIONS = 6;

    // ---------------------------------------------------------------- Pantalla y formación
    inline constexpr int PANTALLA_ANCHO = 240;
    inline constexpr int PANTALLA_ALTO = 160;
    inline constexpr int FILAS = 4;
    inline constexpr int COLUMNAS = 6;
    inline constexpr int FORMACION_X0 = -50;
    inline constexpr int FORMACION_Y0 = -44;
    inline constexpr int SEPARACION_X = 20;
    inline constexpr int SEPARACION_Y = 16;
    inline constexpr int SWAY_MAX = 36;
    inline constexpr bn::fixed SWAY_PASO = bn::fixed(1) / 2;
    inline constexpr bn::fixed VELOCIDAD_RETORNO = 2;
    inline constexpr int ENTRADA_Y = -96;

    // ---------------------------------------------------------------- Jugador
    inline constexpr int JUGADOR_Y = 56;
    inline constexpr int JUGADOR_X_MAX = 108;
    inline constexpr bn::fixed VELOCIDAD_JUGADOR = 2;
    inline constexpr bn::fixed VELOCIDAD_LASER = 4;
    inline constexpr int COOLDOWN_DISPARO = 10;
    inline constexpr int VIDAS_INICIALES = 3;
    inline constexpr int TICKS_INVULNERABLE = 120;
    inline constexpr int TICKS_RESPAWN = 90;
    inline constexpr int TICKS_FIN_JUEGO = 60;

    // ---------------------------------------------------------------- Enemigos
    inline constexpr int TICKS_ANIMACION = 20;
    inline constexpr int TICKS_EXPLOSION = 4;
    inline constexpr int FRAMES_EXPLOSION = 4;
    inline constexpr bn::fixed VELOCIDAD_BALA = bn::fixed(5) / 2;
    inline constexpr bn::fixed DERIVA_BALA = bn::fixed(1) / 2;
    inline constexpr bn::fixed T_DISPARO = bn::fixed(2) / 5;
    inline constexpr int PICADA_BASE_TICKS = 150;
    inline constexpr int PICADA_MIN_TICKS = 70;
    inline constexpr int PAUSA_OLEADA = 60;
    inline constexpr int NIVEL_MAX = 99;
    inline constexpr int PUNTOS_MAX = 999999;

    // ---------------------------------------------------------------- Puntuación
    inline constexpr int PUNTOS_INSIGNIA = 60;
    inline constexpr int PUNTOS_ESCOLTA = 50;
    inline constexpr int PUNTOS_ALIENIGENA = 30;

    // ---------------------------------------------------------------- Tipos
    enum TipoEnemigo
    {
        TIPO_NINGUNO = 0,
        TIPO_INSIGNIA,
        TIPO_ESCOLTA,
        TIPO_ALIENIGENA
    };

    enum EstadoEnemigo
    {
        EST_FORMACION = 0,
        EST_PICADA,
        EST_RETORNO
    };

    enum Estado
    {
        TITULO,
        JUGANDO,
        GAME_OVER
    };

    inline constexpr int FORMACION[FILAS][COLUMNAS] =
    {
        { TIPO_NINGUNO,    TIPO_INSIGNIA,    TIPO_INSIGNIA,    TIPO_INSIGNIA,    TIPO_INSIGNIA,    TIPO_NINGUNO    },
        { TIPO_ESCOLTA,    TIPO_ESCOLTA,     TIPO_ESCOLTA,     TIPO_ESCOLTA,     TIPO_ESCOLTA,     TIPO_ESCOLTA    },
        { TIPO_ALIENIGENA, TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA },
        { TIPO_ALIENIGENA, TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA,  TIPO_ALIENIGENA }
    };

    constexpr int puntos_por_tipo(int tipo)
    {
        return tipo == TIPO_INSIGNIA ? PUNTOS_INSIGNIA :
               tipo == TIPO_ESCOLTA ? PUNTOS_ESCOLTA : PUNTOS_ALIENIGENA;
    }

    // ---------------------------------------------------------------- Estructuras
    struct Enemigo
    {
        bn::optional<bn::sprite_ptr> sprite;
        bn::fixed_point pos;
        bn::fixed_point p0;
        bn::fixed_point p1;
        bn::fixed_point p2;
        bn::fixed t = 0;
        bn::fixed t_paso = 0;
        int home_x = 0;
        int home_y = 0;
        int tipo = TIPO_NINGUNO;
        int estado = EST_FORMACION;
        bool disparo = false;
        bool activo = false;
    };

    struct Laser
    {
        bn::optional<bn::sprite_ptr> sprite;
        bn::fixed x = 0;
        bn::fixed y = 0;
        bool activo = false;
    };

    struct BalaEnemiga
    {
        bn::optional<bn::sprite_ptr> sprite;
        bn::fixed_point pos;
        bn::fixed_point vel;
        bool activo = false;
    };

    struct Explosion
    {
        bn::optional<bn::sprite_ptr> sprite;
        int frame = 0;
        int tick = 0;
        bool activo = false;
    };

    // ---------------------------------------------------------------- Textos bilingües
    enum Idioma
    {
        ESPANOL,
        INGLES
    };

    enum TextoId
    {
        TXT_PULSA_START = 0,
        TXT_IDIOMA,
        TXT_INSIGNIA,
        TXT_ESCOLTA,
        TXT_ALIENIGENA,
        TXT_PUNTOS,
        TXT_RECORD,
        TXT_NIVEL,
        TXT_FIN_JUEGO,
        TXT_REINICIAR,
        TXT_TOTAL
    };

    inline constexpr bn::string_view TITULO_JUEGO = "GALAXIAN";

    inline constexpr bn::string_view TEXTOS_ES[TXT_TOTAL] =
    {
        "PULSA START",
        "IDIOMA: ESPANOL (SELECT)",
        "NAVE INSIGNIA 60 PTS",
        "ESCOLTA 50 PTS",
        "ALIENIGENA 30 PTS",
        "PUNTOS",
        "RECORD",
        "NIVEL",
        "FIN DEL JUEGO",
        "PULSA START PARA REINICIAR"
    };

    inline constexpr bn::string_view TEXTOS_EN[TXT_TOTAL] =
    {
        "PRESS START",
        "LANGUAGE: ENGLISH (SELECT)",
        "FLAGSHIP 60 PTS",
        "ESCORT 50 PTS",
        "ALIEN 30 PTS",
        "SCORE",
        "HIGH SCORE",
        "STAGE",
        "GAME OVER",
        "PRESS START TO RESTART"
    };

    constexpr bn::string_view texto(Idioma idioma, TextoId id)
    {
        return idioma == ESPANOL ? TEXTOS_ES[id] : TEXTOS_EN[id];
    }
}

#endif
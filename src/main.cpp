#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_starfield_bg.h"
#include "bn_sprite_items_player_ship.h"
#include "bn_sprite_items_enemy_flagship.h"
#include "bn_sprite_items_enemy_butterfly.h"
#include "bn_sprite_items_enemy_bee.h"
#include "common_variable_8x8_sprite_font.h"

#include "game_config.h"
#include "game_systems.h"

namespace
{
    struct PantallaTitulo
    {
        bn::vector<bn::sprite_ptr, 12> titulo;
        bn::vector<bn::sprite_ptr, 56> leyenda;
        bn::vector<bn::sprite_ptr, 16> inicio;
        bn::vector<bn::sprite_ptr, 32> idioma;
        bn::vector<bn::sprite_ptr, 3> iconos;

        void limpiar_textos()
        {
            titulo.clear();
            leyenda.clear();
            inicio.clear();
            idioma.clear();
        }

        void limpiar()
        {
            limpiar_textos();
            iconos.clear();
        }
    };

    struct PantallaFin
    {
        bn::vector<bn::sprite_ptr, 16> titulo;
        bn::vector<bn::sprite_ptr, 32> reinicio;

        void limpiar()
        {
            titulo.clear();
            reinicio.clear();
        }
    };

    struct Hud
    {
        bn::vector<bn::sprite_ptr, 24> etiquetas;
        bn::vector<bn::sprite_ptr, 8> puntos;
        bn::vector<bn::sprite_ptr, 8> record;
        bn::vector<bn::sprite_ptr, 4> nivel;
        bn::vector<bn::sprite_ptr, gx::VIDAS_INICIALES> vidas;
        int ult_puntos = -1;
        int ult_record = -1;
        int ult_nivel = -1;
        int ult_vidas = -1;

        void limpiar()
        {
            etiquetas.clear();
            puntos.clear();
            record.clear();
            nivel.clear();
            vidas.clear();
            ult_puntos = -1;
            ult_record = -1;
            ult_nivel = -1;
            ult_vidas = -1;
        }
    };

    void generar_numero(bn::sprite_text_generator& gen, int x, int y, int valor,
                        bn::ivector<bn::sprite_ptr>& destino)
    {
        const bn::string<8> cadena = bn::to_string<8>(valor);
        gen.generate(x, y, bn::string_view(cadena.data(), cadena.size()), destino);
    }

    void dibujar_titulo(gx::Idioma idioma, bn::sprite_text_generator& gen, PantallaTitulo& t)
    {
        t.limpiar_textos();

        gen.set_center_alignment();
        gen.generate(0, -66, gx::TITULO_JUEGO, t.titulo);

        gen.set_left_alignment();
        gen.generate(-56, -24, gx::texto(idioma, gx::TXT_INSIGNIA), t.leyenda);
        gen.generate(-56, -8, gx::texto(idioma, gx::TXT_ESCOLTA), t.leyenda);
        gen.generate(-56, 8, gx::texto(idioma, gx::TXT_ALIENIGENA), t.leyenda);

        gen.set_center_alignment();
        gen.generate(0, 40, gx::texto(idioma, gx::TXT_PULSA_START), t.inicio);
        gen.generate(0, 62, gx::texto(idioma, gx::TXT_IDIOMA), t.idioma);
    }

    void crear_iconos_titulo(PantallaTitulo& t)
    {
        t.iconos.push_back(bn::sprite_items::enemy_flagship.create_sprite(-72, -24));
        t.iconos.push_back(bn::sprite_items::enemy_butterfly.create_sprite(-72, -8, 0));
        t.iconos.push_back(bn::sprite_items::enemy_bee.create_sprite(-72, 8, 0));
    }

    void dibujar_etiquetas_hud(gx::Idioma idioma, bn::sprite_text_generator& gen, Hud& hud)
    {
        hud.limpiar();

        gen.set_left_alignment();
        gen.generate(-116, -76, gx::texto(idioma, gx::TXT_PUNTOS), hud.etiquetas);

        gen.set_center_alignment();
        gen.generate(0, -76, gx::texto(idioma, gx::TXT_RECORD), hud.etiquetas);

        gen.set_right_alignment();
        gen.generate(116, -76, gx::texto(idioma, gx::TXT_NIVEL), hud.etiquetas);
    }

    void actualizar_hud(bn::sprite_text_generator& gen, Hud& hud, int puntos, int record,
                        int nivel, int vidas)
    {
        if(puntos != hud.ult_puntos)
        {
            hud.ult_puntos = puntos;
            hud.puntos.clear();
            gen.set_left_alignment();
            generar_numero(gen, -116, -67, puntos, hud.puntos);
        }

        if(record != hud.ult_record)
        {
            hud.ult_record = record;
            hud.record.clear();
            gen.set_center_alignment();
            generar_numero(gen, 0, -67, record, hud.record);
        }

        if(nivel != hud.ult_nivel)
        {
            hud.ult_nivel = nivel;
            hud.nivel.clear();
            gen.set_right_alignment();
            generar_numero(gen, 116, -67, nivel, hud.nivel);
        }

        if(vidas != hud.ult_vidas)
        {
            hud.ult_vidas = vidas;
            hud.vidas.clear();

            for(int i = 0; i < vidas && i < gx::VIDAS_INICIALES; ++i)
            {
                hud.vidas.push_back(bn::sprite_items::player_ship.create_sprite(-108 + i * 18, 72));
            }
        }
    }

    void dibujar_fin(gx::Idioma idioma, bn::sprite_text_generator& gen, PantallaFin& fin)
    {
        fin.limpiar();
        gen.set_center_alignment();
        gen.generate(0, -8, gx::texto(idioma, gx::TXT_FIN_JUEGO), fin.titulo);
        gen.generate(0, 8, gx::texto(idioma, gx::TXT_REINICIAR), fin.reinicio);
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr fondo = bn::regular_bg_items::starfield_bg.create_bg(0, 0);
    bn::fixed scroll_y = 0;

    bn::sprite_text_generator gen(common::variable_8x8_sprite_font);

    gx::Juego juego;
    PantallaTitulo titulo;
    PantallaFin fin;
    Hud hud;

    gx::Estado estado = gx::TITULO;
    gx::Idioma idioma = gx::ESPANOL;
    int record = 0;
    int tick_titulo = 0;
    int espera_fin = 0;

    crear_iconos_titulo(titulo);
    dibujar_titulo(idioma, gen, titulo);

    while(true)
    {
        scroll_y += 1;

        if(scroll_y >= 256)
        {
            scroll_y -= 256;
        }

        fondo.set_y(scroll_y);

        if(estado == gx::TITULO)
        {
            if(bn::keypad::select_pressed())
            {
                idioma = (idioma == gx::ESPANOL) ? gx::INGLES : gx::ESPANOL;
                dibujar_titulo(idioma, gen, titulo);
                tick_titulo = 0;
            }

            if((tick_titulo & 31) == 0)
            {
                const bool visible = ((tick_titulo >> 5) & 1) == 0;

                for(bn::sprite_ptr& s : titulo.inicio)
                {
                    s.set_visible(visible);
                }
            }

            ++tick_titulo;

            if(bn::keypad::start_pressed())
            {
                titulo.limpiar();
                dibujar_etiquetas_hud(idioma, gen, hud);
                juego.iniciar_partida();
                estado = gx::JUGANDO;
            }
        }
        else if(estado == gx::JUGANDO)
        {
            juego.actualizar();

            if(juego.puntos() > record)
            {
                record = juego.puntos();
            }

            actualizar_hud(gen, hud, juego.puntos(), record, juego.nivel(), juego.vidas());

            if(juego.terminado())
            {
                juego.apagar();
                dibujar_fin(idioma, gen, fin);
                espera_fin = 45;
                estado = gx::GAME_OVER;
            }
        }
        else
        {
            if(espera_fin > 0)
            {
                --espera_fin;
            }
            else if(bn::keypad::start_pressed())
            {
                fin.limpiar();
                dibujar_etiquetas_hud(idioma, gen, hud);
                juego.iniciar_partida();
                estado = gx::JUGANDO;
            }
        }

        bn::core::update();
    }
}
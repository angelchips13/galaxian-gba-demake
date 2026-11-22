#ifndef GAME_SYSTEMS_H
#define GAME_SYSTEMS_H

#include "bn_array.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_rect.h"
#include "bn_random.h"
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_player_ship.h"
#include "bn_sprite_items_player_laser.h"
#include "bn_sprite_items_enemy_bee.h"
#include "bn_sprite_items_enemy_butterfly.h"
#include "bn_sprite_items_enemy_flagship.h"
#include "bn_sprite_items_explosion.h"

#include "game_config.h"

namespace gx
{
    class Juego
    {
    public:
        Juego()
        {
            for(int i = 0; i < 2; ++i)
            {
                _tiles_alienigena.push_back(bn::sprite_items::enemy_bee.tiles_item().create_tiles(i));
                _tiles_escolta.push_back(bn::sprite_items::enemy_butterfly.tiles_item().create_tiles(i));
            }

            for(int i = 0; i < FRAMES_EXPLOSION; ++i)
            {
                _tiles_explosion.push_back(bn::sprite_items::explosion.tiles_item().create_tiles(i));
            }
        }

        void iniciar_partida()
        {
            apagar();
            _puntos = 0;
            _nivel = 1;
            _vidas = VIDAS_INICIALES;
            _terminado = false;
            _anim_tick = 0;
            _anim_frame = 0;
            _sway = 0;
            _sway_dir = 1;
            _timer_picada = 120;
            _cooldown = 0;
            _respawn = 0;
            _fin = 0;
            _pausa = 0;
            _crear_nave();
            _crear_oleada();
        }

        void apagar()
        {
            _nave.reset();

            for(Enemigo& e : _enemigos)
            {
                e.sprite.reset();
                e.activo = false;
            }

            for(Laser& l : _lasers)
            {
                l.sprite.reset();
                l.activo = false;
            }

            for(BalaEnemiga& b : _balas)
            {
                b.sprite.reset();
                b.activo = false;
            }

            for(Explosion& x : _explosiones)
            {
                x.sprite.reset();
                x.activo = false;
            }

            _vivos = 0;
        }

        void actualizar()
        {
            if(_terminado)
            {
                return;
            }

            _actualizar_animacion();
            _actualizar_jugador();
            _actualizar_lasers();
            _actualizar_enemigos();
            _actualizar_balas();
            _colisiones();
            _actualizar_explosiones();
            _gestionar_picadas();
            _gestionar_oleadas();
        }

        [[nodiscard]] int puntos() const
        {
            return _puntos;
        }

        [[nodiscard]] int nivel() const
        {
            return _nivel;
        }

        [[nodiscard]] int vidas() const
        {
            return _vidas;
        }

        [[nodiscard]] bool terminado() const
        {
            return _terminado;
        }

    private:
        bn::vector<bn::sprite_tiles_ptr, 2> _tiles_alienigena;
        bn::vector<bn::sprite_tiles_ptr, 2> _tiles_escolta;
        bn::vector<bn::sprite_tiles_ptr, FRAMES_EXPLOSION> _tiles_explosion;

        bn::optional<bn::sprite_ptr> _nave;
        bn::fixed _nave_x = 0;

        bn::array<Enemigo, MAX_ENEMIES> _enemigos;
        bn::array<Laser, MAX_LASERS> _lasers;
        bn::array<BalaEnemiga, MAX_ENEMY_BULLETS> _balas;
        bn::array<Explosion, MAX_EXPLOSIONS> _explosiones;

        bn::random _rng;
        bn::fixed _sway = 0;
        int _sway_dir = 1;

        int _puntos = 0;
        int _nivel = 1;
        int _vidas = VIDAS_INICIALES;
        int _vivos = 0;
        int _anim_tick = 0;
        int _anim_frame = 0;
        int _timer_picada = 120;
        int _cooldown = 0;
        int _invulnerable = 0;
        int _respawn = 0;
        int _fin = 0;
        int _pausa = 0;
        bool _terminado = false;

        // ------------------------------------------------------------ Creación
        void _crear_nave()
        {
            _nave_x = 0;
            _nave = bn::sprite_items::player_ship.create_sprite(_nave_x, JUGADOR_Y);
            _invulnerable = TICKS_INVULNERABLE;
        }

        bn::sprite_ptr _crear_sprite_enemigo(int tipo, bn::fixed x, bn::fixed y)
        {
            if(tipo == TIPO_INSIGNIA)
            {
                return bn::sprite_items::enemy_flagship.create_sprite(x, y);
            }

            if(tipo == TIPO_ESCOLTA)
            {
                return bn::sprite_items::enemy_butterfly.create_sprite(x, y, _anim_frame);
            }

            return bn::sprite_items::enemy_bee.create_sprite(x, y, _anim_frame);
        }

        void _crear_oleada()
        {
            int indice = 0;

            for(int fila = 0; fila < FILAS; ++fila)
            {
                for(int col = 0; col < COLUMNAS; ++col)
                {
                    const int tipo = FORMACION[fila][col];

                    if(tipo == TIPO_NINGUNO || indice >= MAX_ENEMIES)
                    {
                        continue;
                    }

                    Enemigo& e = _enemigos[indice];
                    e.home_x = FORMACION_X0 + col * SEPARACION_X;
                    e.home_y = FORMACION_Y0 + fila * SEPARACION_Y;
                    e.tipo = tipo;
                    e.estado = EST_RETORNO;
                    e.disparo = false;
                    e.activo = true;
                    e.t = 0;
                    e.pos = bn::fixed_point(e.home_x, ENTRADA_Y - indice * 6);
                    e.sprite = _crear_sprite_enemigo(tipo, e.pos.x(), e.pos.y());
                    ++indice;
                }
            }

            _vivos = indice;
            _timer_picada = (_nivel < 8 ? 150 - _nivel * 10 : 70);
        }

        // ------------------------------------------------------------ Animación
        void _actualizar_animacion()
        {
            ++_anim_tick;

            if(_anim_tick < TICKS_ANIMACION)
            {
                return;
            }

            _anim_tick = 0;
            _anim_frame ^= 1;

            for(Enemigo& e : _enemigos)
            {
                if(! e.activo || ! e.sprite)
                {
                    continue;
                }

                if(e.tipo == TIPO_ESCOLTA)
                {
                    e.sprite->set_tiles(_tiles_escolta[_anim_frame]);
                }
                else if(e.tipo == TIPO_ALIENIGENA)
                {
                    e.sprite->set_tiles(_tiles_alienigena[_anim_frame]);
                }
            }
        }

        // ------------------------------------------------------------ Jugador
        void _actualizar_jugador()
        {
            if(_nave)
            {
                if(bn::keypad::left_held())
                {
                    _nave_x -= VELOCIDAD_JUGADOR;
                }

                if(bn::keypad::right_held())
                {
                    _nave_x += VELOCIDAD_JUGADOR;
                }

                if(_nave_x < -JUGADOR_X_MAX)
                {
                    _nave_x = -JUGADOR_X_MAX;
                }
                else if(_nave_x > JUGADOR_X_MAX)
                {
                    _nave_x = JUGADOR_X_MAX;
                }

                _nave->set_x(_nave_x);

                if(_invulnerable > 0)
                {
                    --_invulnerable;
                    _nave->set_visible(((_invulnerable >> 2) & 1) == 0);
                }

                if(_cooldown > 0)
                {
                    --_cooldown;
                }

                if(bn::keypad::a_pressed() && _cooldown == 0)
                {
                    _disparar();
                }
            }
            else if(_vidas > 0)
            {
                if(--_respawn <= 0)
                {
                    _crear_nave();
                }
            }
            else
            {
                if(--_fin <= 0)
                {
                    _terminado = true;
                }
            }
        }

        void _disparar()
        {
            for(Laser& l : _lasers)
            {
                if(! l.activo)
                {
                    l.activo = true;
                    l.x = _nave_x;
                    l.y = JUGADOR_Y - 10;
                    l.sprite = bn::sprite_items::player_laser.create_sprite(l.x, l.y);
                    bn::sound_items::sfx_shoot.play();
                    _cooldown = COOLDOWN_DISPARO;
                    break;
                }
            }
        }

        void _actualizar_lasers()
        {
            for(Laser& l : _lasers)
            {
                if(! l.activo)
                {
                    continue;
                }

                l.y -= VELOCIDAD_LASER;

                if(l.y < -88)
                {
                    l.sprite.reset();
                    l.activo = false;
                }
                else
                {
                    l.sprite->set_y(l.y);
                }
            }
        }

        // ------------------------------------------------------------ Enemigos
        void _actualizar_enemigos()
        {
            _sway += SWAY_PASO * _sway_dir;

            if(_sway >= SWAY_MAX)
            {
                _sway = SWAY_MAX;
                _sway_dir = -1;
            }
            else if(_sway <= -SWAY_MAX)
            {
                _sway = -SWAY_MAX;
                _sway_dir = 1;
            }

            for(Enemigo& e : _enemigos)
            {
                if(! e.activo)
                {
                    continue;
                }

                if(e.estado == EST_FORMACION)
                {
                    e.pos.set_x(_sway + e.home_x);
                    e.pos.set_y(e.home_y);
                }
                else if(e.estado == EST_RETORNO)
                {
                    e.pos.set_x(_sway + e.home_x);
                    e.pos.set_y(e.pos.y() + VELOCIDAD_RETORNO);

                    if(e.pos.y() >= e.home_y)
                    {
                        e.pos.set_y(e.home_y);
                        e.estado = EST_FORMACION;
                    }
                }
                else
                {
                    e.t += e.t_paso;

                    if(e.t >= 1)
                    {
                        e.estado = EST_RETORNO;
                        e.pos = bn::fixed_point(_sway + e.home_x, ENTRADA_Y);
                    }
                    else
                    {
                        const bn::fixed inv = 1 - e.t;
                        const bn::fixed a = inv * inv;
                        const bn::fixed ab = inv * e.t;
                        const bn::fixed b = ab + ab;
                        const bn::fixed c = e.t * e.t;
                        e.pos = bn::fixed_point(
                            a * e.p0.x() + b * e.p1.x() + c * e.p2.x(),
                            a * e.p0.y() + b * e.p1.y() + c * e.p2.y());

                        if(! e.disparo && e.t >= T_DISPARO && _nave)
                        {
                            e.disparo = _disparo_enemigo(e.pos);
                        }
                    }
                }

                e.sprite->set_position(e.pos);
            }
        }

        void _iniciar_picada(Enemigo& e)
        {
            const int lado = e.pos.x() < 0 ? -1 : 1;
            bn::fixed p1x = e.pos.x() + lado * 50;

            if(p1x < -110)
            {
                p1x = -110;
            }
            else if(p1x > 110)
            {
                p1x = 110;
            }

            const int jitter = _rng.get_int(41) - 20;
            const int duracion = (_nivel < 9) ? PICADA_BASE_TICKS - _nivel * 10 : PICADA_MIN_TICKS;

            e.p0 = e.pos;
            e.p1 = bn::fixed_point(p1x, e.pos.y() - 24);
            e.p2 = bn::fixed_point(_nave_x + jitter, 100);
            e.t = 0;
            e.t_paso = bn::fixed(1) / (duracion < PICADA_MIN_TICKS ? PICADA_MIN_TICKS : duracion);
            e.estado = EST_PICADA;
            e.disparo = false;
        }

        void _gestionar_picadas()
        {
            if(! _nave || _vivos == 0)
            {
                return;
            }

            if(_timer_picada > 0)
            {
                --_timer_picada;
                return;
            }

            int buceadores = 0;

            for(const Enemigo& e : _enemigos)
            {
                if(e.activo && e.estado == EST_PICADA)
                {
                    ++buceadores;
                }
            }

            int maximo = 1 + _nivel / 3;

            if(maximo > 3)
            {
                maximo = 3;
            }

            if(buceadores >= maximo)
            {
                _timer_picada = 20;
                return;
            }

            const int inicio = _rng.get_int(MAX_ENEMIES);

            for(int i = 0; i < MAX_ENEMIES; ++i)
            {
                Enemigo& e = _enemigos[(inicio + i) % MAX_ENEMIES];

                if(e.activo && e.estado == EST_FORMACION)
                {
                    _iniciar_picada(e);
                    break;
                }
            }

            const int base = (_nivel < 8) ? 110 - _nivel * 8 : 46;
            _timer_picada = base + _rng.get_int(30);
        }

        bool _disparo_enemigo(const bn::fixed_point& origen)
        {
            for(BalaEnemiga& b : _balas)
            {
                if(! b.activo)
                {
                    b.activo = true;
                    b.pos = origen;
                    b.vel = bn::fixed_point(_nave_x > origen.x() ? DERIVA_BALA : -DERIVA_BALA, VELOCIDAD_BALA);
                    b.sprite = bn::sprite_items::player_laser.create_sprite(b.pos.x(), b.pos.y());
                    b.sprite->set_vertical_flip(true);
                    bn::sound_items::sfx_dive.play();
                    return true;
                }
            }

            return false;
        }

        void _actualizar_balas()
        {
            for(BalaEnemiga& b : _balas)
            {
                if(! b.activo)
                {
                    continue;
                }

                b.pos += b.vel;

                if(b.pos.y() > 88 || b.pos.x() < -124 || b.pos.x() > 124)
                {
                    b.sprite.reset();
                    b.activo = false;
                }
                else
                {
                    b.sprite->set_position(b.pos);
                }
            }
        }

        // ------------------------------------------------------------ Colisiones
        void _colisiones()
        {
            for(Laser& l : _lasers)
            {
                if(! l.activo)
                {
                    continue;
                }

                const bn::rect rl(l.x.integer(), l.y.integer(), 4, 8);

                for(Enemigo& e : _enemigos)
                {
                    if(! e.activo || e.pos.y() < -84)
                    {
                        continue;
                    }

                    const bn::rect re(e.pos.x().integer(), e.pos.y().integer(), 12, 12);

                    if(rl.intersects(re))
                    {
                        _matar_enemigo(e);
                        l.sprite.reset();
                        l.activo = false;
                        break;
                    }
                }
            }

            if(! _nave || _invulnerable > 0)
            {
                return;
            }

            const bn::rect rp(_nave_x.integer(), JUGADOR_Y, 10, 10);
            bool golpe = false;

            for(Enemigo& e : _enemigos)
            {
                if(e.activo && e.estado == EST_PICADA)
                {
                    const bn::rect re(e.pos.x().integer(), e.pos.y().integer(), 12, 12);

                    if(rp.intersects(re))
                    {
                        _matar_enemigo(e);
                        golpe = true;
                        break;
                    }
                }
            }

            if(! golpe)
            {
                for(BalaEnemiga& b : _balas)
                {
                    if(b.activo)
                    {
                        const bn::rect rb(b.pos.x().integer(), b.pos.y().integer(), 4, 6);

                        if(rp.intersects(rb))
                        {
                            b.sprite.reset();
                            b.activo = false;
                            golpe = true;
                            break;
                        }
                    }
                }
            }

            if(golpe)
            {
                _golpear_jugador();
            }
        }

        void _matar_enemigo(Enemigo& e)
        {
            _sumar_puntos(puntos_por_tipo(e.tipo));
            _crear_explosion(e.pos.x(), e.pos.y());
            bn::sound_items::sfx_explosion.play();
            e.sprite.reset();
            e.activo = false;
            --_vivos;
        }

        void _golpear_jugador()
        {
            _crear_explosion(_nave_x, JUGADOR_Y);
            bn::sound_items::sfx_explosion.play();
            _nave.reset();
            --_vidas;
            _respawn = TICKS_RESPAWN;
            _fin = TICKS_FIN_JUEGO;
        }

        void _sumar_puntos(int cantidad)
        {
            _puntos += cantidad;

            if(_puntos > PUNTOS_MAX)
            {
                _puntos = PUNTOS_MAX;
            }
        }

        // ------------------------------------------------------------ Explosiones
        void _crear_explosion(bn::fixed x, bn::fixed y)
        {
            for(Explosion& ex : _explosiones)
            {
                if(! ex.activo)
                {
                    ex.activo = true;
                    ex.frame = 0;
                    ex.tick = 0;
                    ex.sprite = bn::sprite_items::explosion.create_sprite(x, y, 0);
                    break;
                }
            }
        }

        void _actualizar_explosiones()
        {
            for(Explosion& ex : _explosiones)
            {
                if(! ex.activo)
                {
                    continue;
                }

                if(++ex.tick >= TICKS_EXPLOSION)
                {
                    ex.tick = 0;

                    if(++ex.frame >= FRAMES_EXPLOSION)
                    {
                        ex.sprite.reset();
                        ex.activo = false;
                    }
                    else
                    {
                        ex.sprite->set_tiles(_tiles_explosion[ex.frame]);
                    }
                }
            }
        }

        // ------------------------------------------------------------ Oleadas
        void _gestionar_oleadas()
        {
            if(_vivos > 0)
            {
                return;
            }

            if(_pausa == 0)
            {
                _pausa = PAUSA_OLEADA;
            }
            else if(--_pausa == 0)
            {
                if(_nivel < NIVEL_MAX)
                {
                    ++_nivel;
                }

                _crear_oleada();
            }
        }
    };
}

#endif
// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material ship::_material(0.5f, 1.0f, 5.0f);

//------------------------------------------------------------------------------
ship::ship()
    : object(object_type::ship)
    , _usercmd{}
    , _shield(nullptr)
    , _dead_time(time_value::max)
    , _is_destroyed(false)
    , _shape(ship_model.vertices().data(), ship_model.vertices().size())
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.f);

    _model = &ship_model;
}

//------------------------------------------------------------------------------
ship::~ship()
{
    get_world()->remove_body(&_rigid_body);

    for (auto& subsystem : _subsystems) {
        get_world()->remove(subsystem);
    }
    _subsystems.clear();
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);

    _reactor = get_world()->spawn<subsystem>(this, subsystem_info{subsystem_type::reactor, 8});
    _subsystems.push_back(_reactor.get());

    _engines = get_world()->spawn<subsystem>(this, subsystem_info{subsystem_type::engines, 2});
    _subsystems.push_back(_engines.get());

    _shield = get_world()->spawn<shield>(&_shape, this);
    _subsystems.push_back(_shield.get());

    for (int ii = 0; ii < 2; ++ii) {
        weapon_info info{};

        int c = rand() % 4;
        if (c == 0) {
            info.type = weapon_type::blaster;
            info.projectile_speed = 768.f;
            info.projectile_delay = time_delta::from_seconds(.2f);
            info.projectile_count = 3;
            info.projectile_damage = .2f;
            info.projectile_inertia = true;
        } else if (c == 1) {
            info.type = weapon_type::cannon;
            info.projectile_speed = 1280.f;
            info.projectile_delay = time_delta::from_seconds(.3f);
            info.projectile_count = 2;
            info.projectile_damage = .3f;
            info.projectile_inertia = true;
        } else if (c == 2) {
            info.type = weapon_type::missile;
            info.projectile_speed = 256.f;
            info.projectile_delay = time_delta::from_seconds(.2f);
            info.projectile_count = 3;
            info.projectile_damage = .2f;
            info.projectile_inertia = true;
        } else {
            info.type = weapon_type::laser;
            info.beam_duration = time_delta::from_seconds(1.f);
            info.beam_sweep = 1.f;
            info.beam_damage = .6f;
        }
        info.reload_time = time_delta::from_seconds(4.f);

        _weapons.push_back(get_world()->spawn<weapon>(this, info, vec2(11.f, ii ? 6.f : -6.f)));
        _subsystems.push_back(_weapons.back().get());
    }
}

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    if (!_is_destroyed) {
        renderer->draw_model(_model, get_transform(time), _color);

        constexpr color4 subsystem_colors[2][2] = {
            { color4(.4f, 1.f, .2f, .225f), color4(1.f, .2f, 0.f, .333f) },
            { color4(.4f, 1.f, .2f, 1.00f), color4(1.f, .2f, 0.f, 1.00f) },
        };

        float alpha = 1.f;
        if (time > _dead_time && time - _dead_time < destruction_time) {
            alpha = 1.f - (time - _dead_time) / destruction_time;
        }

        //
        // draw reactor ui
        //

        if (_reactor) {
            constexpr float mint = (3.f / 4.f) * math::pi<float>;
            constexpr float maxt = (5.f / 4.f) * math::pi<float>;
            constexpr float radius = 40.f;
            constexpr float edge_width = 1.f;
            constexpr float edge = edge_width / radius;

            vec2 pos = get_position(time);

            for (int ii = 0; ii < _reactor->maximum_power(); ++ii) {
                bool damaged = ii >= _reactor->maximum_power() - std::ceil(_reactor->damage() - .2f);
                bool powered = ii < _reactor->current_power();

                color4 c = subsystem_colors[powered][damaged]; c.a *= alpha;

                float t0 = maxt - (maxt - mint) * (float(ii + 1) / float(_reactor->maximum_power()));
                float t1 = maxt - (maxt - mint) * (float(ii + 0) / float(_reactor->maximum_power()));
                renderer->draw_arc(pos, radius, 3.f, t0 + .5f * edge, t1 - .5f * edge, c);
            }
        }

        //
        // draw shield ui
        //

        if (_shield) {
            constexpr float mint = (-1.f / 4.f) * math::pi<float>;
            constexpr float maxt = (1.f / 4.f) * math::pi<float>;
            constexpr float radius = 40.f;
            constexpr float edge_width = 1.f;
            constexpr float edge = edge_width / radius;

            float midt =  mint + (maxt - mint) * (_shield->strength() / _shield->info().maximum_power);

            vec2 pos = get_position(time);

            color4 c0 = _shield->colors()[1]; c0.a *= 1.5f * alpha;
            color4 c1 = _shield->colors()[1]; c1.a = alpha;
            renderer->draw_arc(pos, radius, 3.f, mint, maxt, c0);
            renderer->draw_arc(pos, radius, 3.f, mint, midt, c1);
        }

        //
        // draw subsystems ui
        //

        vec2 position = get_position(time) - vec2(vec2i(8 * static_cast<int>(_subsystems.size() - 2) / 2, 40));

        for (auto const* subsystem : _subsystems) {
            // reactor subsystem ui is drawn explicitly
            if (subsystem->info().type == subsystem_type::reactor) {
                continue;
            }

            // draw power bar for each subsystem power level
            for (int ii = 0; ii < subsystem->maximum_power(); ++ii) {
                bool damaged = ii >= subsystem->maximum_power() - std::ceil(subsystem->damage() - .2f);
                bool powered = ii < subsystem->current_power();

                color4 c = subsystem_colors[powered][damaged]; c.a *= alpha;

                renderer->draw_box(vec2(7,3), position + vec2(vec2i(0,10 + 4 * ii)), c);
            }
            position += vec2(8,0);
        }
    }
}

//------------------------------------------------------------------------------
bool ship::touch(object* /*other*/, physics::collision const* /*collision*/)
{
    return !_is_destroyed;
}

//------------------------------------------------------------------------------
void ship::think()
{
    time_value time = get_world()->frametime();

    if (_dead_time > time && _reactor && _reactor->damage() == _reactor->maximum_power()) {
        _dead_time = time;
    }

    if (_engines && _engines->current_power()) {
        constexpr float radius = 128.f;
        constexpr float speed = 16.f;
        constexpr float angular_speed = (2.f * math::pi<float>) * speed / (2.f * math::pi<float> * radius);
        float t0 = -.5f * math::pi<float> + get_rotation();
        float t1 = -.5f * math::pi<float> + get_rotation() + FRAMETIME.to_seconds() * angular_speed;
        vec2 p0 = vec2(cos(t0), sin(t0)) * radius;
        vec2 p1 = vec2(cos(t1), sin(t1)) * radius;
        set_linear_velocity((p1 - p0) / FRAMETIME.to_seconds());
        set_angular_velocity(angular_speed);
    } else {
        set_linear_velocity(get_linear_velocity() * .99f);
        set_angular_velocity(get_angular_velocity() * .99f);
    }

    if (_dead_time > time) {
        for (auto& subsystem : _subsystems) {
            if (subsystem->damage()) {
                subsystem->repair(1.f / 15.f);
                break;
            }
        }
        _shield->recharge(1.f / 5.f);
    }

    for (auto& weapon : _weapons) {
        if (_dead_time > time && _random.uniform_real() < .01f) {
            // get a list of all ships in the world
            std::vector<game::ship*> ships;
            for (auto* object : get_world()->objects()) {
                if (object != this && object->_type == object_type::ship) {
                    if (!static_cast<ship*>(object)->is_destroyed()) {
                        ships.push_back(static_cast<ship*>(object));
                    }
                }
            }

            // select a random target
            if (ships.size()) {
                game::ship* target = ships[_random.uniform_int(ships.size())];
                if (weapon->info().type != weapon_type::laser) {
                    weapon->attack_projectile(target, vec2_zero);
                } else {
                    float angle = _random.uniform_real(2.f * math::pi<float>);
                    vec2 v = vec2(cosf(angle), sinf(angle));
                    weapon->attack_beam(target, v * -4.f, v * 4.f);
                }
            }
        }

        if (_dead_time > time) {
            // cancel pending attacks on targets that have been destroyed
            if (weapon->target() && weapon->target()->_type == object_type::ship) {
                if (static_cast<ship const*>(weapon->target())->is_destroyed()) {
                    weapon->cancel();
                }
            }
        }
    }

    //
    // Death sequence
    //

    if (time > _dead_time) {
        if (time - _dead_time < destruction_time) {
            float t = (time - _dead_time) / destruction_time;
            float s = powf(_random.uniform_real(), 6.f * (1.f - t));

            // random explosion at a random point on the ship
            if (s > .2f) {
                // find a random point on the ship's model
                vec2 v;
                do {
                    v = _model->mins() + (_model->maxs() - _model->mins()) * vec2(_random.uniform_real(), _random.uniform_real());
                } while (!_model->contains(v));

                get_world()->add_effect(time, effect_type::explosion, v * get_transform(), vec2_zero, .2f * s);
                if (s * s > t) {
                    sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
                    get_world()->add_sound(_sound_explosion, get_position(), .2f * s);
                }
            }
        } else if (!_is_destroyed) {
            // add final explosion effect
            get_world()->add_effect(time, effect_type::explosion, get_position(), vec2_zero);
            sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
            get_world()->add_sound(_sound_explosion, get_position());

            // remove all subsystems
            for (auto& subsystem : _subsystems) {
                get_world()->remove(subsystem);
            }
            _subsystems.clear();
            _weapons.clear();

            _is_destroyed = true;
        } else if (time - _dead_time > destruction_time + respawn_time) {
            // remove this ship
            get_world()->remove(this);

            // spawn a new ship to take this ship's place
            ship* new_ship = get_world()->spawn<ship>();
            new_ship->set_position(vec2(_random.uniform_real(-320.f,320.f), _random.uniform_real(-240.f,240.f)), true);
            new_ship->set_rotation(_random.uniform_real(2.f * math::pi<float>), true);
        }
    }
}

//------------------------------------------------------------------------------
void ship::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void ship::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
void ship::damage(object* /*inflictor*/, vec2 /*point*/, float amount)
{
    // get list of subsystems that can take additional damage
    std::vector<subsystem*> subsystems;
    for (auto* subsystem : _subsystems) {
        if (subsystem->damage() < subsystem->maximum_power()) {
            subsystems.push_back(subsystem);
        }
    }

    // apply damage to a random subsystem
    if (subsystems.size()) {
        int idx = rand() % subsystems.size();
        subsystems[idx]->damage(amount * 6.f);
    }
}

//------------------------------------------------------------------------------
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

} // namespace game

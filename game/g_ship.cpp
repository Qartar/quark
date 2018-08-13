// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_module.h"

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
    _world->remove_body(&_rigid_body);

    for (auto& module : _modules) {
        _world->remove(module);
    }
    _modules.clear();
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    _world->add_body(this, &_rigid_body);

    _reactor = _world->spawn<module>(this, module_info{module_type::reactor, 8});
    _modules.push_back(_reactor.get());

    _engines = _world->spawn<module>(this, module_info{module_type::engines, 2});
    _modules.push_back(_engines.get());

    _shield = _world->spawn<shield>(&_shape, this);
    _modules.push_back(_shield.get());

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

        _weapons.push_back(_world->spawn<weapon>(this, info, vec2(11.f, ii ? 6.f : -6.f)));
        _modules.push_back(_weapons.back().get());
    }
}

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    if (!_is_destroyed) {
        renderer->draw_model(_model, get_transform(time), _color);

        vec2 position = get_position(time) - vec2(vec2i(8 * static_cast<int>(_modules.size() - 1) / 2, 40));

        for (auto const* module : _modules) {
            int h = module->info().type == module_type::reactor ? 1 : 3;
            for (int ii = 0; ii < module->maximum_power(); ++ii) {
                bool damaged = ii >= module->maximum_power() - std::ceil(module->damage() - .2f);
                bool powered = ii < module->current_power();

                color4 c;
                if (damaged && !powered) {
                    c = color4(1,.2f,0,1);
                } else if (damaged && powered) {
                    float t = sin(2.f * math::pi<float> * time.to_seconds() - .1f * ii) * .5f + .5f;
                    c = color4(1,.2f,0,1) * t + color4(0,.8f,.4f,1) * (1.f - t);
                } else if (!damaged && !powered) {
                    c = color4(1,.8f,0,1);
                } else /*if (!damaged && powered)*/ {
                    c = color4(0,.8f,.4f,1);
                }

                renderer->draw_box(vec2(vec2i(7,h)), position + vec2(vec2i(0,8 + (h + 1) * ii + h / 2)), c);
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
    time_value time = _world->frametime();

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
        for (auto& module : _modules) {
            if (module->damage()) {
                module->repair(1.f / 15.f);
                break;
            }
        }
        _shield->recharge(1.f / 5.f);
    }

    for (auto& weapon : _weapons) {
        if (_dead_time > time && _random.uniform_real() < .01f) {
            // get a list of all ships in the world
            std::vector<game::ship*> ships;
            for (auto& object : _world->objects()) {
                if (object.get() != this && object->_type == object_type::ship) {
                    if (!static_cast<ship*>(object.get())->is_destroyed()) {
                        ships.push_back(static_cast<ship*>(object.get()));
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

    {
        if (time - _dead_time < destruction_time) {
            float t = (time - _dead_time) / destruction_time;
            float s = powf(_random.uniform_real(), 6.f * (1.f - t));

            // find a random point on the ship's model
            vec2 v;
            do {
                v = _model->mins() + (_model->maxs() - _model->mins()) * vec2(_random.uniform_real(), _random.uniform_real());
            } while (!_model->contains(v));

            // random explosion at a random point on the ship
            if (s > .2f) {
                _world->add_effect(time, effect_type::explosion, v * get_transform(), vec2_zero, .2f * s);
                if (s * s > t) {
                    sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
                    _world->add_sound(_sound_explosion, get_position(), .2f * s);
                }
            }
        } else if (!_is_destroyed) {
            // add final explosion effect
            _world->add_effect(time, effect_type::explosion, get_position(), vec2_zero);
            sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
            _world->add_sound(_sound_explosion, get_position());

            // remove all modules
            for (auto& module : _modules) {
                _world->remove(module);
            }
            _modules.clear();
            _weapons.clear();

            _is_destroyed = true;
        } else if (time - _dead_time > destruction_time + respawn_time) {
            // remove this ship
            _world->remove(this);

            // spawn a new ship to take this ship's place
            ship* new_ship = _world->spawn<ship>();
            new_ship->set_position(_world->mins() + (_world->maxs() - _world->mins()) * vec2(_random.uniform_real(), _random.uniform_real()), true);
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
    // get list of modules that can take additional damage
    std::vector<module*> modules;
    for (auto* module : _modules) {
        if (module->damage() < module->maximum_power()) {
            modules.push_back(module);
        }
    }

    // apply damage to a random module
    if (modules.size()) {
        int idx = rand() % modules.size();
        modules[idx]->damage(amount * 6.f);
    }
}

//------------------------------------------------------------------------------
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

} // namespace game

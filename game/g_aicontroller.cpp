// g_aicontroller.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_ship.h"
#include "g_subsystem.h"
#include "g_weapon.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
aicontroller::aicontroller(ship* target)
    : object(object_type::controller)
    , _ship(target)
    , _destroyed_time(time_value::max)
{}

//------------------------------------------------------------------------------
aicontroller::~aicontroller()
{}

//------------------------------------------------------------------------------
void aicontroller::spawn()
{
}

//------------------------------------------------------------------------------
void aicontroller::think()
{
    time_value time = get_world()->frametime();

    if (!_ship) {
        return;
    }

    //
    // update navigation
    //

    if (!_ship->is_destroyed()) {
        constexpr float radius = 128.f;
        constexpr float speed = 16.f;
        constexpr float angular_speed = speed / radius;

        _ship->engines()->set_move_target(mat2::rotate(_ship->get_rotation())[0]);
        _ship->engines()->set_look_target(angular_speed);
    }

    //
    // update weapons
    //

    for (auto& weapon : _ship->weapons()) {
        if (!_ship->is_destroyed() && _random.uniform_real() < .01f) {
            // get a list of all ships in the world
            std::vector<game::ship*> ships;
            for (auto* object : get_world()->objects()) {
                if (object != _ship && object->_type == object_type::ship) {
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

        if (!_ship->is_destroyed()) {
            // cancel pending attacks on targets that have been destroyed
            if (weapon->target() && weapon->target()->_type == object_type::ship) {
                if (static_cast<ship const*>(weapon->target())->is_destroyed()) {
                    weapon->cancel();
                }
            }
        }
    }

    //
    // handle respawn
    //

    if (_ship->is_destroyed()) {
        if (_destroyed_time > time) {
            _destroyed_time = time;
        } else if (time - _destroyed_time >= respawn_time) {
            _destroyed_time = time_value::max;
            get_world()->remove(_ship.get());

            // spawn a new ship to replace the destroyed ship's place
            _ship = get_world()->spawn<ship>();
            _ship->set_position(vec2(_random.uniform_real(-320.f, 320.f), _random.uniform_real(-240.f, 240.f)), true);
            _ship->set_rotation(_random.uniform_real(2.f * math::pi<float>), true);

        }
    }
}

//------------------------------------------------------------------------------
vec2 aicontroller::get_position(time_value time) const
{
    return _ship ? _ship->get_position(time) : vec2_zero;
}

//------------------------------------------------------------------------------
float aicontroller::get_rotation(time_value time) const
{
    return _ship ? _ship->get_rotation(time) : 0.f;
}

//------------------------------------------------------------------------------
mat3 aicontroller::get_transform(time_value time) const
{
    return _ship ? _ship->get_transform(time) : mat3_identity;
}

} // namespace game

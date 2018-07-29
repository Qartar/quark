// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_shield.h"
#include "g_weapon.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material ship::_material(0.5f, 1.0f, 5.0f);
physics::convex_shape ship::_shape({
    {-16.f,  -9.f},
    {-15.f, -10.f},
    { -1.f, -10.f},
    {  8.f,  -8.f},
    { 10.f,  -7.f},
    { 11.f,  -6.f},
    { 11.f,   6.f},
    { 10.f,   7.f},
    {  8.f,   8.f},
    { -1.f,  10.f},
    {-15.f,  10.f},
    {-16.f,   9.f},
});

//------------------------------------------------------------------------------
ship::ship()
    : object(object_type::ship)
    , _usercmd{}
    , _shield(nullptr)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.f);

    _model = &ship_model;
}

//------------------------------------------------------------------------------
ship::~ship()
{
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    _shield = _world->spawn<shield>(&_shape, this);

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
            info.beam_damage = 1.f;
        }
        info.reload_time = time_delta::from_seconds(4.f);

        _weapons.push_back(_world->spawn<weapon>(this, info, vec2(11.f, ii ? 6.f : -6.f)));
    }
}

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    renderer->draw_model(_model, get_transform(time), _color);
}

//------------------------------------------------------------------------------
bool ship::touch(object* /*other*/, physics::collision const* /*collision*/)
{
    return true;
}

//------------------------------------------------------------------------------
void ship::think()
{
    {
        constexpr float radius = 128.f;
        constexpr float speed = 16.f;
        constexpr float angular_speed = (2.f * math::pi<float>) * speed / (2.f * math::pi<float> * radius);
        float t0 = -.5f * math::pi<float> + get_rotation();
        float t1 = -.5f * math::pi<float> + get_rotation() + FRAMETIME.to_seconds() * angular_speed;
        vec2 p0 = vec2(cos(t0), sin(t0)) * radius;
        vec2 p1 = vec2(cos(t1), sin(t1)) * radius;
        set_linear_velocity((p1 - p0) / FRAMETIME.to_seconds());
        set_angular_velocity(angular_speed);
    }

    _shield->set_position(get_position());
    _shield->set_rotation(get_rotation());
    _shield->set_linear_velocity(get_linear_velocity());
    _shield->set_angular_velocity(get_angular_velocity());

    _shield->recharge(1.f / 15.f);

    for (auto& weapon : _weapons) {
        while (_random.uniform_real() < .01f) {
            std::vector<game::ship*> ships;
            for (auto& object : _world->objects()) {
                if (object.get() != this && object->_type == object_type::ship) {
                    ships.push_back(static_cast<ship*>(object.get()));
                }
            }
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
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

} // namespace game

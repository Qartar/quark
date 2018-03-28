// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_shield.h"

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
    if (_shield == nullptr) {
        _shield = _world->spawn<shield>(&_shape, this);
    }

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

    while (_random.uniform_real() < .05f) {
        float angle = _random.uniform_real(2.f * math::pi<float>);
        vec2 launch_direction = vec2(std::cos(angle), std::sin(angle));

        projectile* proj = _world->spawn<projectile>(nullptr, .1f, weapon_type::blaster);

        vec2 launch_position = get_position() - launch_direction * 256.f;

        proj->set_position(launch_position, true);
        proj->set_linear_velocity(launch_direction * 768.f);
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

// g_shield.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_shield.h"
#include "g_projectile.h"
#include "g_weapon.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material shield::_material(0.0f, 0.0f);

//------------------------------------------------------------------------------
shield::shield(physics::shape const* base, game::ship* owner)
    : subsystem(owner, {subsystem_type::shields, 2})
    , _base(base)
    , _strength(2)
    , _damage_time(time_value::zero)
{
    _type = object_type::shield;

    float radius = 32.f;

    for (int ii = 0; ii < kNumVertices; ++ii) {
        float a = ii * (2.f * math::pi<float> / kNumVertices);
        _vertices[ii] = vec2(std::cos(a), std::sin(a)) * radius;
        _flux[ii] = 0.f;
    }

    for (int ii = 0; ii < 4; ++ii) {
        step_vertices();
    }

    _shape = physics::convex_shape(_vertices, kNumVertices);
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.f);

    constexpr color4 schemes[12] = {
        // blue
        color4(.3f, .7f, 1.f, 1.f),
        color4(.0f, .5f, 1.f, .2f),
        color4(.0f, .3f, 1.f, .1f),
        color4(.0f, .0f, 1.f, .0f),
        // green
        color4(.7f, 1.f, .4f, 1.f),
        color4(.4f, 1.f, .2f, .15f),
        color4(.2f, .9f, .1f, .05f),
        color4(.0f, .8f, .0f, .0f),
        // orange
        color4(1.f, .6f, .1f, 1.f),
        color4(1.f, .3f, .1f, .2f),
        color4(.9f, .2f, .0f, .075f),
        color4(.8f, .1f, .0f, .0f),
    };

    static int style = 0;
    for (int ii = 0; ii < _colors.size(); ++ii) {
        _colors[ii] = schemes[(style % 3) * 4 + ii];
    }
    ++style;
}

//------------------------------------------------------------------------------
shield::~shield()
{
    get_world()->remove_body(&_rigid_body);
}

//------------------------------------------------------------------------------
void shield::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);
}

//------------------------------------------------------------------------------
void shield::draw(render::system* renderer, time_value time) const
{
    vec2 draw_vertices[kNumVertices * 2 + 1];
    color4 draw_colors[kNumVertices * 2 + 1];
    int draw_indices[kNumVertices * 9];

    vec2 pos = get_position(time);
    mat2 rot; rot.set_rotation(get_rotation(time));


    for (int ii = 0; ii < kNumVertices; ++ii) {
        float s = _flux[ii] / (.1f + _flux[ii]);

        draw_vertices[ii * 2 + 0] = _vertices[ii] * rot + pos;
        draw_vertices[ii * 2 + 1] = _vertices[ii] * rot * (.9f * s + .8f * (1.f - s)) + pos;

        draw_colors[ii * 2 + 0] = _colors[0] * s + _colors[1] * (1.f - s);
        draw_colors[ii * 2 + 1] = _colors[1] * s + _colors[2] * (1.f - s);

        draw_colors[ii * 2 + 0].a *= std::max(s, _strength + s);
        draw_colors[ii * 2 + 1].a *= std::max(0.f, _strength);

        draw_indices[ii * 9 +  0] = ii * 2 + 0;
        draw_indices[ii * 9 +  1] = ii * 2 + 1;
        draw_indices[ii * 9 +  2] = ii * 2 + 2;

        draw_indices[ii * 9 +  3] = ii * 2 + 1;
        draw_indices[ii * 9 +  4] = ii * 2 + 3;
        draw_indices[ii * 9 +  5] = ii * 2 + 2;

        draw_indices[ii * 9 +  6] = ii * 2 + 1;
        draw_indices[ii * 9 +  7] = kNumVertices * 2;
        draw_indices[ii * 9 +  8] = ii * 2 + 3;
    }

    draw_vertices[kNumVertices * 2] = pos;
    draw_colors[kNumVertices * 2] = _colors[3];

    draw_indices[kNumVertices * 9 - 7] = 0;
    draw_indices[kNumVertices * 9 - 5] = 1;
    draw_indices[kNumVertices * 9 - 4] = 0;
    draw_indices[kNumVertices * 9 - 1] = 1;

    renderer->draw_triangles(draw_vertices, draw_colors, draw_indices, kNumVertices * 9);
}

//------------------------------------------------------------------------------
bool shield::touch(object* other, physics::collision const* collision)
{
    if (other->_type == object_type::projectile) {
        return damage(collision->point, static_cast<projectile*>(other)->damage());
    } else if (other->_type == object_type::weapon) {
        weapon_info const& info = static_cast<weapon*>(other)->info();
        return damage(collision->point, info.beam_damage * FRAMETIME.to_seconds());
    }

    return true;
}

//------------------------------------------------------------------------------
bool shield::damage(vec2 position, float damage)
{
    int best = 0;
 
    vec2 pos = get_position();
    mat2 rot; rot.set_rotation(-get_rotation());
    vec2 local = (position - pos) * rot;

    // find the nearest vertex to the impact point
    for (int ii = 1; ii < kNumVertices; ++ii) {
        if ((_vertices[ii] - local).length_sqr() < (_vertices[best] - local).length_sqr()) {
            best = ii;
        }
    }

    _damage_time = get_world()->frametime();

    if (_strength == 0.f) {
        return false;
    }

    _strength = std::max(0.f, _strength - damage);

    damage *= kNumVertices;

    float limit = 1.f;
    float delta = std::min(limit, damage);
    _flux[best] += delta * .4f;
    damage -= delta;
    limit *= .9f;

    for (int ii = 1; damage > 0.0f && ii < kNumVertices / 2; ++ii) {
        int i0 = (best + kNumVertices - ii) % kNumVertices;
        int i1 = (best + kNumVertices + ii) % kNumVertices;

        delta = std::min(limit, damage);
        _flux[i0] += delta * .4f;

        delta = std::min(limit, damage);
        _flux[i1] += delta * .4f;

        damage -= 2.f * delta;
        limit *= .8f;
    }

    return true;
}

//------------------------------------------------------------------------------
void shield::step_vertices()
{
    float hull_radius[kNumVertices];
    float prev_radius[kNumVertices];
    float shield_radius[kNumVertices];

    for (int ii = 0; ii < kNumVertices; ++ii) {
        vec2 v = physics::collide::closest_point(_base, _vertices[ii]);
        hull_radius[ii] = v.length();
        shield_radius[ii] = hull_radius[ii] + 4.f;
    }

    for (int kk = 0; kk < 16; ++kk) {
        for (int ii = 0; ii < kNumVertices; ++ii) {
            prev_radius[ii] = shield_radius[ii];
        }

        for (int ii = 0; ii < kNumVertices; ++ii) {
            vec2 va = _vertices[(ii + kNumVertices - 1) % kNumVertices];
            vec2 vb = _vertices[(ii + kNumVertices + 1) % kNumVertices];
            vec2 vc = _vertices[ii];

            int ia = (ii + kNumVertices - 1) % kNumVertices;
            int ib = (ii + kNumVertices + 1) % kNumVertices;

            float r = (1.f / 3.f) * (prev_radius[ia] + prev_radius[ib] + prev_radius[ii]);
            if (r < hull_radius[ii] + 2.f) {
                r = hull_radius[ii] + 2.f;
            } else if (r > hull_radius[ii] + 6.f) {
                r = hull_radius[ii] + 6.f;
            }

            shield_radius[ii] = r;
        }
    }

    for (int ii = 0; ii < kNumVertices; ++ii) {
        _vertices[ii] = _vertices[ii].normalize() * shield_radius[ii];
    }
}

//------------------------------------------------------------------------------
void shield::step_strength()
{
    float delta[kNumVertices] = {0};

    // propagate shield flux
    for (int ii = 0; ii < kNumVertices; ++ii) {
        float f0 = _flux[(ii + kNumVertices - 1) % kNumVertices];
        float f1 = _flux[(ii + kNumVertices + 1) % kNumVertices];
        delta[ii] = (f0 + f1) * 0.5f - _flux[ii];
    }

    // add propagated flux and decay
    for (int ii = 0; ii < kNumVertices; ++ii) {
        _flux[ii] = _flux[ii] * .6f + delta[ii] * .4f;
    }
}

//------------------------------------------------------------------------------
void shield::recharge(float strength_per_second)
{
    assert(strength_per_second >= 0.f);
    time_value time = get_world()->frametime();
    if (time - _damage_time < recharge_delay) {
        return;
    }

    float delta = strength_per_second * FRAMETIME.to_seconds();
    _strength = clamp(_strength + delta, 0.f, std::max(_strength, static_cast<float>(current_power())));
}

//------------------------------------------------------------------------------
void shield::think()
{
    subsystem::think();
    step_strength();

    if (_strength > current_power()) {
        float delta = discharge_rate * FRAMETIME.to_seconds();
        _strength = std::max(_strength - delta, static_cast<float>(current_power()));
    }

    set_position(_owner->get_position());
    set_rotation(_owner->get_rotation());
    set_linear_velocity(_owner->get_linear_velocity());
    set_angular_velocity(_owner->get_angular_velocity());
}

//------------------------------------------------------------------------------
void shield::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void shield::write_snapshot(network::message& /*message*/) const
{
}

} // namespace game

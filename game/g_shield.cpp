// g_shield.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_shield.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material shield::_material(0.5f, 1.0f, 5.0f);

//------------------------------------------------------------------------------
shield::shield(physics::shape const* base, game::object* owner)
    : object(object_type::shield, owner)
    , _base(base)
    , _shape(_vertices, kNumVertices)
{
    float radius = 32.f;

    for (int ii = 0; ii < kNumVertices; ++ii) {
        float a = ii * (2.f * math::pi<float> / kNumVertices);
        _vertices[ii] = vec2(std::cos(a), std::sin(a)) * radius;
        _strength[ii] = 1.f;
        _flux[ii] = 0.f;
    }

    for (int ii = 0; ii < 4; ++ii) {
        step_vertices();
    }

    _shape = physics::convex_shape(_vertices, kNumVertices);
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.f);

    static int index = 0;
    _style = index++;
}

//------------------------------------------------------------------------------
shield::~shield()
{
}

//------------------------------------------------------------------------------
void shield::draw(render::system* renderer, time_value /*time*/) const
{
    vec2 draw_vertices[kNumVertices * 2 + 1];
    color4 draw_colors[kNumVertices * 2 + 1];
    int draw_indices[kNumVertices * 9];

    vec2 pos = get_position();
    mat2 rot; rot.set_rotation(get_rotation());

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

    color4 const* scheme = schemes;
    switch (_style % 3) {
        case 0: scheme = schemes; break;
        case 1: scheme = schemes + 4; break;
        case 2: scheme = schemes + 8; break;
    }

    for (int ii = 0; ii < kNumVertices; ++ii) {
        float s = _flux[ii] / (.1f + _flux[ii]);

        draw_vertices[ii * 2 + 0] = _vertices[ii] * rot + pos;
        draw_vertices[ii * 2 + 1] = _vertices[ii] * rot * (.9f * s + .8f * (1.f - s)) + pos;

        draw_colors[ii * 2 + 0] = scheme[0] * s + scheme[1] * (1.f - s);
        draw_colors[ii * 2 + 1] = scheme[1] * s + scheme[2] * (1.f - s);

        draw_colors[ii * 2 + 0].a *= std::max(s, _strength[ii] + s);
        draw_colors[ii * 2 + 1].a *= std::max(0.f, _strength[ii]);

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
    draw_colors[kNumVertices * 2] = scheme[3];

    draw_indices[kNumVertices * 9 - 7] = 0;
    draw_indices[kNumVertices * 9 - 5] = 1;
    draw_indices[kNumVertices * 9 - 4] = 0;
    draw_indices[kNumVertices * 9 - 1] = 1;

    renderer->draw_triangles(draw_vertices, draw_colors, draw_indices, kNumVertices * 9);
}

//------------------------------------------------------------------------------
bool shield::touch(object* other, physics::collision const* collision)
{
    int best = 0;
 
    vec2 pos = get_position();
    mat2 rot; rot.set_rotation(-get_rotation());
    vec2 local = (collision->point - pos) * rot;

    // find the nearest vertex to the impact point
    for (int ii = 1; ii < kNumVertices; ++ii) {
        if ((_vertices[ii] - local).length_sqr() < (_vertices[best] - local).length_sqr()) {
            best = ii;
        }
    }

    if (other->_type == object_type::projectile) {
        float damage = kNumVertices * static_cast<projectile*>(other)->damage();

        if (_strength[best] < 2.f * static_cast<projectile*>(other)->damage()) {
            return false;
        }

        float delta = std::min(_strength[best], damage);
        _strength[best] -= delta;
        damage -= delta;

        for (int ii = 1; damage > 0.0f && ii < kNumVertices / 2; ++ii) {
            int i0 = (best + kNumVertices - ii) % kNumVertices;
            int i1 = (best + kNumVertices + ii) % kNumVertices;

            delta = std::min(_strength[i0], damage);
            _strength[i0] -= delta;
            damage -= delta;

            delta = std::min(_strength[i1], damage);
            _strength[i1] -= delta;
            damage -= delta;
        }
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

    for (int ii = 0; ii < kNumVertices; ++ii) {
        float sa = _strength[(ii + kNumVertices - 1) % kNumVertices];
        float sb = _strength[(ii + kNumVertices + 1) % kNumVertices];
        float sc = _strength[ii];

        delta[ii] += (sa - sc) * 0.2f;
        delta[ii] += (sb - sc) * 0.2f;
        _flux[ii] += std::abs(sa - sc) * 0.4f;
        _flux[ii] += std::abs(sb - sc) * 0.4f;
    }

    for (int ii = 0; ii < kNumVertices; ++ii) {
        _strength[ii] += delta[ii];
        _flux[ii] = _flux[ii] * .5f;
    }
}

//------------------------------------------------------------------------------
void shield::recharge(float strength_per_second)
{
    float delta = strength_per_second * FRAMETIME.to_seconds();
    for (int ii = 0; ii < kNumVertices; ++ii) {
        _strength[ii] = std::min(1.f, _strength[ii] + delta);
    }
}

//------------------------------------------------------------------------------
void shield::think()
{
    // step twice to speed up propagation
    step_strength();
    step_strength();
    step_strength();
    step_strength();
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
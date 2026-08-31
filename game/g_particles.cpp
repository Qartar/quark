// g_particles.cpp
//

#include "precompiled.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
render::particle* world::add_particle (time_value time)
{
    _particles.emplace_back(render::particle{});
    _particles.back().time = time;
    return &_particles.back();
}

//------------------------------------------------------------------------------
void world::free_particle (render::particle *p) const
{
    _particles[p - _particles.data()] = _particles.back();
    _particles.pop_back();
}

//------------------------------------------------------------------------------
void world::draw_particles(render::system* renderer, time_value time) const
{
    for (std::size_t ii = 0; ii < _particles.size(); ++ii) {
        double ptime = (time - _particles[ii].time).to_seconds();
        // Particles can be emitted in the future, skip size/alpha culling in
        // case particle would have negative size or alpha when time is negative
        if (ptime < 0) {
            continue;
        }
        if (_particles[ii].color.a + _particles[ii].color_velocity.a * ptime < 0.0) {
            free_particle(&_particles[ii]);
            --ii;
        } else if (_particles[ii].size + _particles[ii].size_velocity * ptime < 0.0) {
            free_particle(&_particles[ii]);
            --ii;
        }
    }

    renderer->draw_particles(
        time,
        _particles.data(),
        _particles.size());
}

//------------------------------------------------------------------------------
void world::clear_particles()
{
    _particles.clear();
}

//------------------------------------------------------------------------------
void world::add_effect(time_value time, effect_type type, vec3 position, vec3 direction, float strength, vec3 velocity)
{
    write_effect(time, type, position, direction, strength);

    float   r, d;

    mat3 tx = globe::surface_projection(position).submatrix<3,3>();

    switch (type) {
        case effect_type::smoke: {
            int count = static_cast<int>(strength);
            if (_random.uniform_real() < (strength - count)) {
                ++count;
            }
            render::particle* p;

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(4.f);

                p->position = position + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = sqrt(_random.uniform_real()) * 32.f;

                p->velocity = vec3(vec2(cos(r),sin(r))*d) * tx;
                p->velocity += direction * d * 5.f + velocity;

                p->color = color4(1.0f,_random.uniform_real(.25f, .75f),0.0f,0.1f);
                p->color_velocity = color4(-2,-2,0,-p->color.a/(0.25f+square(_random.uniform_real())*7.5f));
                p->size = _random.uniform_real(2.f, 4.f);
                p->size_velocity = 3.0f;

                p->drag = _random.uniform_real(2.f, 4.f);
            }
            break;
        }

        case effect_type::sparks: {
            render::particle* p;

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                p->position = position + vec3(vec2(_random.uniform_real(-2.f, 2.f),_random.uniform_real(-2.f, 2.f))) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(128.f);

                p->velocity = vec3(vec2(cos(r)*d,sin(r)*d)) * tx;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1,_random.uniform_real(.5f, 1.f),0,strength*_random.uniform_real(.5f, 1.5f));
                p->color_velocity = color4(0,-1.0f,0,_random.uniform_real(-3.f, -2.f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(.5f, 1.f);
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::cannon_impact:
        case effect_type::missile_impact:
        case effect_type::explosion: {
            render::particle* p;
            float scale = std::sqrt(strength);

            // flash

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale + velocity;

            p->color = color4(1,.95f,.9f,1);
            p->color_velocity = color4(0,0,0,-3);
            p->size = 1.f;
            p->size_velocity = 144.0f * scale;

            // fire

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(8.f * scale);

                p->position = position + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = sqrt(_random.uniform_real()) * 128.f * strength;

                p->velocity = vec3(vec2(cos(r),sin(r))*d) * tx;
                p->velocity += direction * d * 0.5f + velocity;

                p->color = color4(1.0f,_random.uniform_real(.25f, .75f),0.0f,0.1f);
                p->color_velocity = color4(-2,-2,0,-p->color.a/(0.25f+square(_random.uniform_real())*7.5f));
                p->size = _random.uniform_real(4.f, 8.f) * scale;
                p->size_velocity = 6.0f * scale;

                p->drag = _random.uniform_real(2.f, 4.f) * scale;
            }

            // debris

            for (int ii = 0; ii < 32 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(2.f * scale);

                p->position = position + vec3(vec2(cos(r)*d,sin(r)*d)) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(128.f * scale);

                p->velocity = vec3(vec2(cos(r)*d,sin(r)*d)) * tx;
                p->velocity += direction * d * 0.5f + velocity;

                p->color = color4(1,_random.uniform_real(.5f, 1.f),0,1);
                p->color_velocity = color4(0,0,0,_random.uniform_real(-2.5f, -1.5f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(.5f, 1.f);
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::cannon: {
            render::particle* p;
            float scale = std::cbrt(strength);

            // flash

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 9.6f + velocity;

            p->color = color4(1,.95f,.9f,1);
            p->color_velocity = color4(0,0,0,-5);
            p->size = 1.f;
            p->size_velocity = 144.0f * scale;

            // fire

            for (int ii = 0; ii < 16 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(4.f);

                p->position = position + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = sqrt(_random.uniform_real()) * 32.f;

                p->velocity = vec3(vec2(cos(r),sin(r))*d * scale) * tx;
                p->velocity += direction * d * 5.f * scale + velocity;

                p->color = color4(1.0f,_random.uniform_real(.25f, .75f),0.0f,0.1f);
                p->color_velocity = color4(-2,-2,0,-p->color.a/(0.25f+square(_random.uniform_real())*7.5f));
                p->size = _random.uniform_real(6.f, 12.f) * scale;
                p->size_velocity = 3.0f * scale;

                p->drag = _random.uniform_real(2.f, 4.f);
            }

            // debris

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(.5f);

                p->position = position + vec3(vec2(cos(r)*d,sin(r)*d)) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(64.f);

                p->velocity = vec3(vec2(cos(r)*d,sin(r)*d)) * tx;
                p->velocity += direction * _random.uniform_real(96.f) + velocity;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(64.f, 128.f);

                p->acceleration = vec3(vec2(cos(r), sin(r))*d) * tx;

                p->color = color4(1,_random.uniform_real(.25f, .75f),0,1);
                p->color_velocity = color4(0,0,0,_random.uniform_real(-3.5f, -1.5f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(2.f, 4.f);
                p->flags = render::particle::tail;
            }

            // smoke

            int count = int(16 * scale);
            for (int ii = 0; ii < count; ++ii) {
                float dt = square(ii / float(count));
                if ( (p = add_particle(time + time_delta::from_seconds(dt))) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(4.f);

                p->position = position + velocity * dt + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = sqrt(_random.uniform_real()) * 32.f;

                p->velocity = vec3(vec2(cos(r),sin(r))*d * scale * (1.f - dt)) * tx;
                p->velocity += direction * d * 5.f * scale * (1.f - dt) + velocity;

                p->color = color4(1.0f,_random.uniform_real(.25f, .75f),0.0f,0.1f);
                p->color_velocity = color4(-2,-2,0,-p->color.a/(0.25f+square(_random.uniform_real())*7.5f));
                p->size = _random.uniform_real(4.f, 8.f) * (1.5f - dt) * scale;
                p->size_velocity = 2.0f * (1.5f - dt) * scale;

                p->drag = _random.uniform_real(2.f, 4.f);
            }

            break;
        }

        case effect_type::splash: {
            render::particle* p;
            float scale = std::sqrt(strength);

            // shock wave

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = vec3_zero;

            p->color = color4(0.8f,0.9f,1.0f,0.25f);
            p->color_velocity = color4(0,0,0,-0.05f / std::sqrt(scale));
            p->size = 12.0f * scale;
            p->size_velocity = 0.5f;

            if ( (p = add_particle(time + time_delta::from_seconds(.5f * scale))) == NULL )
                return;

            p->position = position;
            p->velocity = vec3_zero;

            p->color = color4(0.8f,0.9f,1.0f,0.25f);
            p->color_velocity = color4(0,0,0,-.025f / std::sqrt(scale));
            p->size = 12.0f * scale;
            p->size_velocity = 0.5f;
            p->flags = render::particle::invert;

            // splash

            for (int ii = 0; ii < 64 * scale; ++ii) {
                float time_offset = (.5f + _random.normal_real(.0625f, .25f)) * scale;
                if ( (p = add_particle(time + time_delta::from_seconds(time_offset))) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(4.f * scale);

                p->position = position + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real() * 32.f * strength;

                p->velocity = vec3(vec2(cos(r),sin(r))*d) * tx;
                p->velocity += direction * d * 0.5f;

                p->color = color4(0.8f,0.9f,1.0f,0.25f);
                p->color_velocity = color4(0,0,0,-p->color.a/((5.f+square(_random.uniform_real())*3.f)) * scale);
                p->size = _random.uniform_real(.5f, 6.f) * scale;
                p->size_velocity = 1.0f * strength;

                p->drag = _random.uniform_real(2.f, 4.f) * scale;
            }

            break;
        }

        case effect_type::funnel_smoke: {
            int count = static_cast<int>(strength);
            if (_random.uniform_real() < (strength - count)) {
                ++count;
            }
            render::particle* p;

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(4.f);

                p->position = position + vec3(vec2(cos(r),sin(r))*d) * tx;

                r = _random.uniform_real(2.f * math::pi);
                d = sqrt(_random.uniform_real()) * 8.f;

                p->velocity = vec3(vec2(cos(r),sin(r))*d) * tx;
                p->velocity += direction * d * 5.f + velocity;

                float albedo = _random.uniform_real(0.f, .2f);
                p->color = color4(albedo, albedo, albedo, 0.2f);
                float duration = std::exp(_random.normal_real(1.f, 1.f));
                p->color_velocity = color4(0,0,0,-p->color.a/duration);
                p->size = _random.uniform_real(2.f, 4.f);
                p->size_velocity = _random.uniform_real(5.f, 10.f) / sqrt(duration);

                p->drag = _random.uniform_real(1.f, 2.f);
            }

            break;
        }

        default:
            break;
    }
}

//------------------------------------------------------------------------------
void world::add_trail_effect(effect_type type, vec3 position, vec3 old_position, vec3 direction, float strength)
{
    float   r, d;

    vec3 lerp = position - old_position;
    mat3 tx = globe::surface_projection(position).submatrix<3,3>();

    switch (type) {
        case effect_type::missile_trail: {
            int count = static_cast<int>(strength);
            render::particle* p;

            // fire

            for (int ii = 0; ii < count; ++ii) {
                float t = static_cast<float>(ii) / static_cast<float>(count);
                if ( (p = add_particle(frametime() + FRAMETIME * t)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real();
                p->position = position + vec3(vec2(std::cos(r)*d,std::sin(r)*d)) * tx + lerp * static_cast<float>(ii) / static_cast<float>(count);
                p->velocity = direction * _random.uniform_real(.25f, .75f) + vec3(vec2(_random.uniform_real(-24.f, 24.f),_random.uniform_real(-24.f, 24.f))) * tx;

                p->size = _random.uniform_real(1.f, 2.f);
                p->size_velocity = _random.uniform_real(18.f, 36.f);

                p->color = color4(1.0f,_random.uniform_real(.5f, 1.f),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/_random.uniform_real(.15f, .3f));

                p->drag = _random.uniform_real(1.5f, 2.5f);
            }

            // debris

            for (int ii = 0; ii < count; ++ii) {
                float t = static_cast<float>(ii) / static_cast<float>(count);
                if ( (p = add_particle(frametime() + FRAMETIME * t)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real();

                p->position = position + vec3(vec2(std::cos(r),std::sin(r))*d) * tx + lerp * static_cast<float>(ii) / static_cast<float>(count);

                r = _random.uniform_real(2.f * math::pi);
                d = _random.uniform_real(64.f);

                p->velocity = direction * _random.uniform_real(.25f, 1.f) + vec3(vec2(_random.uniform_real(-48.f, 48.f),_random.uniform_real(-48.f, 48.f))) * tx;

                p->color = color4(1,_random.uniform_real(.5f, 1.f),0,1);
                p->color_velocity = color4(0,0,0,-3.0f-15.0f*(1.0f-square(_random.uniform_real())));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(1.5f, 3.f);
                p->flags = render::particle::tail;
            }

            break;
        }
        default:
            break;
    }
}

} // namespace game

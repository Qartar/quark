// g_particles.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_particles.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
int particle_effect::layer::evaluate_count(
    random& r,
    vec2 pos,
    vec2 vel,
    vec2 dir,
    float str) const
{
    std::array<float, 1024> values;
    std::array<float, 8> inputs;
    assert(expr.num_values() <= values.size());
    assert(expr.num_inputs() == inputs.size());

    inputs = { pos.x, pos.y, vel.x, vel.y, dir.x, dir.y, str, 0.f };

    expr.evaluate_one(count, r, inputs.data(), values.data());
    return static_cast<int>(values[count]);
}

//------------------------------------------------------------------------------
void particle_effect::layer::evaluate(
    render::particle& p,
    random& r,
    vec2 pos,
    vec2 vel,
    vec2 dir,
    float str,
    float idx) const
{
    std::array<float, 1024> values;
    std::array<float, 8> inputs;
    assert(expr.num_values() <= values.size());
    assert(expr.num_inputs() == inputs.size());

    inputs = { pos.x, pos.y, vel.x, vel.y, dir.x, dir.y, str, idx };

    expr.evaluate(r, inputs.data(), values.data());

    p.size = values[size];
    p.size_velocity = values[size_velocity];

    p.position = vec2(
        values[position[0]],
        values[position[1]]);

    p.velocity = vec2(
        values[velocity[0]],
        values[velocity[1]]);

    p.acceleration = vec2(
        values[acceleration[0]],
        values[acceleration[1]]);

    p.drag = values[drag];

    p.color = color4(
        values[color[0]],
        values[color[1]],
        values[color[2]],
        values[color[3]]);

    p.color_velocity = color4(
        values[color_velocity[0]],
        values[color_velocity[1]],
        values[color_velocity[2]],
        values[color_velocity[3]]);

    p.flags = flags;
}

//------------------------------------------------------------------------------
parser::result<particle_effect::layer> particle_effect::parse_layer(parser::token const*& tokens, parser::token const* end) const
{
    expression_parser parser({
        "posx", "posy", "velx", "vely", "dirx", "diry", "str", "idx",
    });

    layer out{};

#if 1
    while (tokens < end) {
        if (*tokens == "flags") {
            if (++tokens >= end) {
                return parser::error{*(tokens - 1), "expected '=' after '" + *(tokens - 1) + "'"};
            } else if (*tokens++ != '=') {
                return parser::error{*(tokens - 1), "expected '=' after '" + *(tokens - 2) + "', found '" + *(tokens - 1) + "'"};
            }
            while (tokens < end) {
                if (*tokens == "invert") {
                    out.flags = static_cast<render::particle::flag_bits>(out.flags | render::particle::invert);
                    ++tokens;
                } else if (*tokens == "tail") {
                    out.flags = static_cast<render::particle::flag_bits>(out.flags | render::particle::tail);
                    ++tokens;
                } else {
                    return parser::error{*tokens, "unrecognized flag '" + *tokens + "'"};
                }
                if (*tokens == ";") {
                    break;
                } else if (tokens >= end) {
                    return parser::error{*(tokens - 1), "expected '|' after '" + *(tokens - 1) + "'"};
                } else if (*tokens++ != '|') {
                    return parser::error{*(tokens - 1), "expected '|' after '" + *(tokens - 2) + "', found '" + *(tokens - 1) + "'"};
                }
            }
            if (tokens >= end || *tokens++ != ';') {
                return parser::error{*(tokens - 1), ""};
            }
        }
    }
#else
    parser::context context;
    while (context.has_token()) {
        if (context.check_token("flags")) {
            if (!context.expect_token("=")) {
                return {};
            }
            while (context.has_token()) {
                if (context.check_token("invert")) {
                    out.flags = static_cast<render::particle::flag_bits>(out.flags | render::particle::invert);
                } else if (context.check_token("tail")) {
                    out.flags = static_cast<render::particle::flag_bits>(out.flags | render::particle::tail);
                } else {
                    return {};
                }
                if (context.peek_token(";")) {
                    break;
                } else if (!context.expect_token("|")) {
                    return {};
                }
            }
            if (!context.expect_token(";")) {
                return {};
            }
        }
    }
#endif

    return out;
}

//------------------------------------------------------------------------------
bool particle_effect::parse(std::string str)
{
    definition = str;
    auto result = parser::tokenize(str.c_str(), str.c_str() + str.size());
    if (std::holds_alternative<parser::error>(result)) {
        return false;
    }

    parser::token const* tokens = std::get<parser::tokenized>(result).data();
    parser::token const* end = tokens + std::get<parser::tokenized>(result).size();

    while (tokens < end) {
        if (*tokens == "layer") {
            if (++tokens >= end) {
                return false;
            }
            parser::token const* layer_name = tokens;
            if (++tokens >= end || *tokens != '{') {
                return false;
            }
            auto layer = parse_layer(++tokens, end);
            if (std::holds_alternative<parser::error>(layer)) {
                return false;
            }
            if (++tokens >= end || *tokens != '}') {
                return false;
            }
            layers.push_back(std::get<particle_effect::layer>(layer));
            layers.back().name = std::string(layer_name->begin, layer_name->end);
        } else {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
void world::add_particle_effect(particle_effect const* effect, time_value time, vec2 pos, vec2 vel, vec2 dir, float str)
{
    render::particle* p = nullptr;

    for (auto const& layer : effect->layers) {
        int count = layer.evaluate_count(_random, pos, vel, dir, str);
        for (int ii = 0; ii < count; ++ii) {
            if (!(p = add_particle(time))) {
                return;
            }
            layer.evaluate(*p, _random, pos, vel, dir, str, float(ii));
        }
    }
}

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
        float ptime = (time - _particles[ii].time).to_seconds();
        if (_particles[ii].color.a + _particles[ii].color_velocity.a * ptime < 0.0f) {
            free_particle(&_particles[ii]);
            --ii;
        } else if (_particles[ii].size + _particles[ii].size_velocity * ptime < 0.0f) {
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
void world::add_effect(time_value time, effect_type type, vec2 position, vec2 direction, float strength)
{
    write_effect(time, type, position, direction, strength);

    float   r, d;

    switch (type) {
        case effect_type::smoke: {
            int count = static_cast<int>(strength);
            render::particle* p;

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real();
                p->position = position + vec2(std::cos(r)*d,std::sin(r)*d);
                p->velocity = direction * _random.uniform_real(0.25f, 1.f)
                            + vec2(_random.uniform_real(-24.f, 24.f), _random.uniform_real(-24.f, 24.f));

                p->size = _random.uniform_real(2.f, 6.f);
                p->size_velocity = _random.uniform_real(2.f, 4.f);

                p->color = color4(0.5f,0.5f,0.5f,_random.uniform_real(.1f, .2f));
                p->color_velocity = color4(0,0,0,-p->color.a / _random.uniform_real(1.f, 2.f));

                p->drag = _random.uniform_real(2.5f, 4.f);
            }
            break;
        }

        case effect_type::sparks: {
            render::particle* p;

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                p->position = position + vec2(_random.uniform_real(-2.f, 2.f),_random.uniform_real(-2.f, 2.f));

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(128.f);

                p->velocity = vec2(cos(r)*d,sin(r)*d);
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

            // shock wave

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1.0f,1.0f,0.5f,0.5f);
            p->color_velocity = -p->color * color4(0,1,3,3);
            p->size = 12.0f * scale;
            p->size_velocity = 192.0f * scale;
            p->flags = render::particle::invert;

            // fire

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(8.f * scale);

                p->position = position + vec2(cos(r),sin(r))*d;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = sqrt(_random.uniform_real()) * 128.f * strength;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,_random.uniform_real(),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.5f+square(_random.uniform_real())*2.5f));
                p->size = _random.uniform_real(8.f, 24.f) * scale;
                p->size_velocity = 1.0f * strength;

                p->drag = _random.uniform_real(2.f, 4.f) * scale;
            }

            // debris

            for (int ii = 0; ii < 32 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(2.f * scale);

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(128.f * scale);

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

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

            // flash

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 9.6f;

            p->color = color4(1,1,0,1);
            p->color_velocity = color4(0,-2.5f,0,-5.f);
            p->size = .1f;
            p->size_velocity = 96.0f;

            // fire

            for (int ii = 0; ii < 8; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(4.f);

                p->position = position + vec2(cos(r),sin(r))*d;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = sqrt(_random.uniform_real()) * 32.f;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,_random.uniform_real(.25f, .75f),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.25f+square(_random.uniform_real())));
                p->size = _random.uniform_real(2.f, 6.f);
                p->size_velocity = 0.5f;
                p->flags = render::particle::invert;

                p->drag = _random.uniform_real(3.f, 6.f);
            }

            // debris

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(.5f);

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(64.f);

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * _random.uniform_real(96.f);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(64.f, 128.f);

                p->acceleration = vec2(cos(r), sin(r))*d;

                p->color = color4(1,_random.uniform_real(.25f, .75f),0,1);
                p->color_velocity = color4(0,0,0,_random.uniform_real(-3.5f, -1.5f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(2.f, 4.f);
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::blaster: {
            render::particle* p;

            // vortex

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 9.6f;

            p->color = color4(1,0,0,0);
            p->color_velocity = color4(0,1,0,2.5f);
            p->size = 19.2f;
            p->size_velocity = -96.0f;
            p->flags = render::particle::invert;

            // fire

            for (int ii = 0; ii < 8; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(4.f);

                p->position = position + vec2(cos(r),sin(r))*d;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = sqrt(_random.uniform_real()) * 32.f;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,_random.uniform_real(.5f),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.25f+square(_random.uniform_real())));
                p->size = _random.uniform_real(2.f, 6.f);
                p->size_velocity = 0.5f;
                p->flags = render::particle::invert;

                p->drag = _random.uniform_real(3.f, 6.f);

                // vortex

                render::particle* p2;

                if ( (p2 = add_particle(time)) == NULL )
                    return;

                p2->position = p->position;
                p2->velocity = p->velocity;
                p2->drag = p->drag;

                p2->color = color4(1,0,0,0);
                p2->color_velocity = color4(0,1,0,1);
                p2->size = 4.8f;
                p2->size_velocity = -18.0f;
                p2->flags = render::particle::invert;
            }

            // debris

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(.5f);

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(64.f);

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * _random.uniform_real(96.f);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(64.f, 128.f);

                p->acceleration = vec2(cos(r), sin(r))*d;

                p->color = color4(1,_random.uniform_real(.5f),0,1);
                p->color_velocity = color4(0,0,0,_random.uniform_real(-3.5f, -1.5f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(2.f, 4.f);
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::blaster_impact: {
            render::particle* p;
            float scale = std::sqrt(strength);

            // vortex

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1,0,0,0);
            p->color_velocity = color4(0,1,0,2.5f);
            p->size = 96.0f * scale;
            p->size_velocity = -480.0f * scale;
            p->flags = render::particle::invert;

            // shock wave

            if ( (p = add_particle(time)) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1.0f,0.25f,0.0f,0.5f);
            p->color_velocity = -p->color * color4(0,1,3,3);
            p->size = 12.0f * scale;
            p->size_velocity = 192.0f * scale;
            p->flags = render::particle::invert;

            // fire

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(16.f * scale);

                p->position = position + vec2(cos(r),sin(r))*d;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = sqrt(_random.uniform_real()) * 128.f * strength;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,_random.uniform_real(.5f),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.5f+square(_random.uniform_real())*1.5f));
                p->size = _random.uniform_real(8.f, 24.f) * scale;
                p->size_velocity = 1.0f * strength;

                p->drag = _random.uniform_real(2.f, 4.f) * scale;
            }

            // debris

            for (int ii = 0; ii < 32 * scale; ++ii) {
                if ( (p = add_particle(time)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(2.f * scale);

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(128.f * scale);

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

                p->color = color4(1,_random.uniform_real(.5f),0,1);
                p->color_velocity = color4(0,0,0,_random.uniform_real(-2.5f, -1.5f));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = _random.uniform_real(.5f, 1.f);
                p->flags = render::particle::tail;
            }
            break;
        }

        default:
            break;
    }
}

//------------------------------------------------------------------------------
void world::add_trail_effect(effect_type type, vec2 position, vec2 old_position, vec2 direction, float strength)
{
    float   r, d;

    vec2 lerp = position - old_position;

    switch (type) {
        case effect_type::missile_trail: {
            int count = static_cast<int>(strength);
            render::particle* p;

            // fire

            for (int ii = 0; ii < count; ++ii) {
                float t = static_cast<float>(ii) / static_cast<float>(count);
                if ( (p = add_particle(frametime() + FRAMETIME * t)) == NULL )
                    return;

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real();
                p->position = position + vec2(std::cos(r)*d,std::sin(r)*d) + lerp * static_cast<float>(ii) / static_cast<float>(count);
                p->velocity = direction * _random.uniform_real(.25f, .75f) + vec2(_random.uniform_real(-24.f, 24.f),_random.uniform_real(-24.f, 24.f));

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

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real();

                p->position = position + vec2(std::cos(r),std::sin(r))*d + lerp * static_cast<float>(ii) / static_cast<float>(count);

                r = _random.uniform_real(2.f * math::pi<float>);
                d = _random.uniform_real(64.f);

                p->velocity = direction * _random.uniform_real(.25f, 1.f) + vec2(_random.uniform_real(-48.f, 48.f),_random.uniform_real(-48.f, 48.f));

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

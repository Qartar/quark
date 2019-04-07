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

    float value = expr.evaluate_one(count, r, inputs.data(), values.data());
    return static_cast<int>(value);
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

    p.time += time_delta::from_seconds(values[time]);

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
bool particle_effect::parse_layer_flags(parser::context& context, render::particle::flag_bits& flags) const
{
    flags = {};

    if (!context.expect_token("=")) {
        return false;
    }

    while (context.has_token()) {
        if (context.check_token("invert")) {
            flags = static_cast<render::particle::flag_bits>(flags | render::particle::invert);
        } else if (context.check_token("tail")) {
            flags = static_cast<render::particle::flag_bits>(flags | render::particle::tail);
        } else {
            parser::token t = std::get<parser::token>(context.next_token());
            context.set_error({t, "expected particle flag, found '" + t + "'"});
            return false;
        }
        if (context.peek_token(";")) {
            break;
        } else if (!context.expect_token("|")) {
            return false;
        }
    }

    return context.expect_token(";");
}

//------------------------------------------------------------------------------
bool particle_effect::parse_layer_value(parser::context& context, expression_parser& parser, expression::value& value) const
{
    if (!context.expect_token("=")) {
        return false;
    }

    auto result = parser.parse_expression(context);
    if (std::holds_alternative<parser::error>(result)) {
        return false;
    } else {
        value = std::get<expression::value>(result);
    }

    return context.expect_token(";");
}

//------------------------------------------------------------------------------
bool particle_effect::parse_layer_vector(parser::context& context, expression_parser& parser, expression::value* vector, std::size_t vector_size) const
{
    if (!context.expect_token("=")) {
        return false;
    }

    for (std::size_t ii = 0; ii < vector_size; ++ii) {
        if (ii && !context.expect_token(",")) {
            return false;
        }
        auto result = parser.parse_expression(context);
        if (std::holds_alternative<parser::error>(result)) {
            return false;
        } else {
            vector[ii] = std::get<expression::value>(result);
        }
    }

    return context.expect_token(";");
}

//------------------------------------------------------------------------------
bool particle_effect::parse_local_value(parser::context& context, expression_parser& parser) const
{
    auto localname = context.next_token();
    if (std::holds_alternative<parser::error>(localname)) {
        return false;
    }

    if (!context.expect_token("=")) {
        return false;
    }

    auto result = parser.parse_temporary(context);
    if (std::holds_alternative<parser::error>(result)) {
        return false;
    } else {
        parser.assign({std::get<parser::token>(localname).begin,
                       std::get<parser::token>(localname).end},
                      std::get<expression::value>(result));
    }

    return context.expect_token(";");
}

//------------------------------------------------------------------------------
namespace {

struct value_info {
    string::literal name;
    expression::value* value;
    std::size_t size;

    value_info(string::literal name, expression::value& value)
        : name(name), value(&value), size(1) {}
    template<std::size_t sz> value_info(string::literal name, expression::value (&value)[sz])
        : name(name), value(value), size(sz) {}
    bool operator==(parser::token const& token) const { return token == name; }
};

} // anonymous namespace

//------------------------------------------------------------------------------
bool particle_effect::parse_layer(parser::context& context, layer& layer) const
{
    layer = {};

    // parse name
    if (!context.peek_token("{")) {
        auto token = context.next_token();
        if (!std::holds_alternative<parser::token>(token)) {
            return false;
        }
        layer.name = string::buffer({std::get<parser::token>(token).begin,
                                     std::get<parser::token>(token).end});
    }

    // parse opening brace
    if (!context.expect_token("{")) {
        return false;
    }

    expression_parser parser({
        "posx", "posy", "velx", "vely", "dirx", "diry", "str", "idx",
    });

    const value_info values[] = {
        {"count", layer.count},
        {"time", layer.time},
        {"size", layer.size},
        {"size_velocity", layer.size_velocity},
        {"position", layer.position},
        {"velocity", layer.velocity},
        {"acceleration", layer.acceleration},
        {"drag", layer.drag},
        {"color", layer.color},
        {"color_velocity", layer.color_velocity},
    };

    // zero-initialize all values
    for (auto& v : values) {
        for (std::size_t ii = 0; ii < v.size; ++ii) {
            v.value[ii] = parser.add_constant(0.f);
        }
    }

    // parse fields until the closing brace
    while (context.has_token()) {
        if (context.peek_token("}")) {
            break;
        }

        auto result = context.next_token();
        if (!std::holds_alternative<parser::token>(result)) {
            return false;
        }

        auto token = std::get<parser::token>(result);
        if (token == "flags") {
            if (!parse_layer_flags(context, layer.flags)) {
                return false;
            }
        } else if (token == "let") {
            if (!parse_local_value(context, parser)) {
                return false;
            }
        } else {
            auto v = std::find(std::begin(values), std::end(values), token);

            if (v == std::end(values)) {
                context.set_error({token, "undeclared identifier '" + token + "'"});
                return false;
            } else if (v->size == 1) {
                if (!parse_layer_value(context, parser, *v->value)) {
                    return false;
                }
            } else {
                if (!parse_layer_vector(context, parser, v->value, v->size)) {
                    return false;
                }
            }
        }
    }

    std::vector<expression::value> map(parser.num_values());
    layer.expr = parser.compile(map.data());
    for (auto& v : values) {
        for (std::size_t ii = 0; ii < v.size; ++ii) {
            v.value[ii] = map[v.value[ii]];
        }
    }

    // parse closing brace
    return context.expect_token("}");
}

//------------------------------------------------------------------------------
bool particle_effect::parse(parser::context& context)
{
    if (context.has_error()) {
        return false;
    }

    // parse opening brace
    if (!context.expect_token("{")) {
        return false;
    }

    // parse layers until the closing brace
    while (context.has_token()) {
        if (context.peek_token("}")) {
            break;
        }

        auto result = context.next_token();
        if (!std::holds_alternative<parser::token>(result)) {
            return false;
        }

        auto token = std::get<parser::token>(result);
        if (token == "layer") {
            layer layer;
            if (!parse_layer(context, layer)) {
                return false;
            }
            layers.push_back(std::move(layer));
        } else {
            context.set_error({token, "unexpected token"});
            return false;
        }
    }

    // parse closing brace
    return context.expect_token("}");
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

    {
        constexpr std::size_t linenumber = __LINE__ + 2;
        string::literal definition =
R"(
{
    layer one {
        flags = tail;
        count = 50;
        size = 5 * str;
    }
    layer two {
        // this is a test!
        flags = invert | tail;
        position = 5 * 2, 10;
        let foo = 4;
        let bar = 6;
        /* this is another
        size = posx;
        test */
        size = in.pos.x;
    }
    layer three {
        flags = shazaam;
    }
}
)";
        parser::context context(definition, __FILE__, linenumber);
        particle_effect effect;
        if (!effect.parse(context)) {
            auto info = context.get_info(context.get_error().tok);
            log::message("%s(%zu,%zu): ", info.filename.c_str(), info.linenumber, info.column);
            log::error("%s\n", context.get_error().msg.c_str());
            log::message("%s\n", context.get_line(info.linenumber).c_str());
            for (std::size_t ii = 0; ii + 1 < info.column; ++ii) {
                log::message(" ");
            }
            for (std::size_t ii = 0, sz = context.get_error().tok.end - context.get_error().tok.begin; ii < sz; ++ii) {
                log::message("^");
            }
            log::message("\n");
        }
    }

    {
        constexpr std::size_t linenumber = __LINE__ + 2;
        string::literal definition =
R"(
{
    layer shock_wave {
        let scale = sqrt(str);

        count = 1;
        flags = invert;

        position = posx, posy;
        velocity = dirx * 48 * scale, diry * 48 * scale;
        acceleration = 0, 0;

        color = 1, 1, 0.5, 0.5;
        color_velocity = 0, -1, -1.5, -1.5;

        size = 12 * scale;
        size_velocity = 192 * scale;
    }

    layer fire {
        let scale = sqrt(str);

        count = 64 * scale;

        let r = random * 2 * pi;
        let d = random * 8 * scale;

        position = posx + cos(r) * d, posy + sin(r) * d;

        let r = random * 2 * pi;
        let d = sqrt(random) * 128 * str;

        velocity = (dirx * 0.5 + cos(r)) * d, (diry * 0.5 + sin(r)) * d;
        acceleration = 0, 0;

        color = 1, random, 0, 0.1;
        color_velocity = 0, 0, 0, -0.1 / (0.5 + (random ^ 2) * 2);

        size = (random * (24 - 8) + 8) * scale;
        size_velocity = str;

        drag = (random * (4 - 2) + 2) * scale;
    }

    layer debris {
        let scale = sqrt(str);

        count = 32 * scale;
        flags = tail;

        let r = random * 2 * pi;
        let d = random * 2 * scale;

        position = posx + cos(r) * d, posy + sin(r) * d;

        let r = random * 2 * pi;
        let d = sqrt(random) * 128 * scale;

        velocity = (dirx * 0.5 + cos(r)) * d, (diry * 0.5 + sin(r)) * d;
        acceleration = 0, 0;

        color = 1, random * .5 + .5, 0, 1;
        color_velocity = 0, 0, 0, random - 2.5;

        size = 0.5;
        size_velocity = 0;

        drag = random * .5 + .5;
    }
}
)";
        parser::context context(definition, __FILE__, linenumber);
        if (!_effect.parse(context)) {
            auto info = context.get_info(context.get_error().tok);
            log::message("%s(%zu,%zu): ", info.filename.c_str(), info.linenumber, info.column);
            log::error("%s\n", context.get_error().msg.c_str());
            log::message("%s\n", context.get_line(info.linenumber).c_str());
            for (std::size_t ii = 0; ii + 1 < info.column; ++ii) {
                log::message(" ");
            }
            for (std::size_t ii = 0, sz = context.get_error().tok.end - context.get_error().tok.begin; ii < sz; ++ii) {
                log::message("^");
            }
            log::message("\n");
        }
    }
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
#if 1
            add_particle_effect(&_effect, time, position, vec2_zero, direction, strength);
#else
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
#endif
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

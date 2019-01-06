// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_character.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_subsystem.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material ship::_material(0.5f, 1.0f, 5.0f);

vec2 main_body_vertices[] = {
    {11.0000000, 6.00000000 },
    {10.0000000, 7.00000000 },
    {8.00000000, 8.00000000 },
    {5.00000000, 9.00000000 },
    {-1.00000000, 9.00000000 },
    {-7.00000000, 5.00000000 },
    {-7.00000000, -5.00000000 },
    {-1.00000000, -9.00000000 },
    {-1.00000000, -9.00000000 },
    {5.00000000, -9.00000000 },
    {8.00000000, -8.00000000 },
    {10.0000000, -7.00000000 },
    {11.0000000, -6.00000000 },
    {11.0000000, 6.00000000 },
};

vec2 left_engine_vertices[] = {
    vec2(8, -8) + vec2{-1.00000000, 9.0000000 },
    vec2(8, -8) + vec2{-1.00000000, 10.0000000 },
    vec2(8, -8) + vec2{-16.0000000, 10.0000000 },
    vec2(8, -8) + vec2{-17.0000000, 9.00000000 },
    vec2(8, -8) + vec2{-17.0000000, 7.00000000 },
    vec2(8, -8) + vec2{-16.0000000, 6.00000000 },
    vec2(8, -8) + vec2{-9.0000000, 5.00000000 },
    vec2(8, -8) + vec2{-7.0000000, 5.00000000 },
};

vec2 right_engine_vertices[] = {
    vec2(8, 8) + vec2{-1.00000000, -9.0000000 },
    vec2(8, 8) + vec2{-1.00000000, -10.0000000 },
    vec2(8, 8) + vec2{-16.0000000, -10.0000000 },
    vec2(8, 8) + vec2{-17.0000000, -9.00000000 },
    vec2(8, 8) + vec2{-17.0000000, -7.00000000 },
    vec2(8, 8) + vec2{-16.0000000, -6.00000000 },
    vec2(8, 8) + vec2{-9.0000000, -5.00000000 },
    vec2(8, 8) + vec2{-7.0000000, -5.00000000 },
};

//------------------------------------------------------------------------------
ship::ship()
    : object(object_type::ship)
    , _usercmd{}
    , _shield(nullptr)
    , _dead_time(time_value::max)
    , _is_destroyed(false)
    , _shape({
        {std::make_unique<physics::convex_shape>(main_body_vertices)},
        {std::make_unique<physics::convex_shape>(left_engine_vertices), vec2(-8, 8)},
        {std::make_unique<physics::convex_shape>(right_engine_vertices), vec2(-8, -8)}})
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.f);

    _model = &ship_model;
}

//------------------------------------------------------------------------------
ship::~ship()
{
    get_world()->remove_body(&_rigid_body);
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);

    for (int ii = 0; ii < 3; ++ii) {
        _crew.push_back(get_world()->spawn<character>());
    }

    _reactor = get_world()->spawn<subsystem>(this, subsystem_info{subsystem_type::reactor, 8});
    _subsystems.push_back(_reactor);

    _engines = get_world()->spawn<game::engines>(this, engines_info{16.f, .125f, 8.f, .0625f, .5f, .5f});
    _subsystems.push_back(_engines);

    _shield = get_world()->spawn<shield>(&_shape, this);
    _subsystems.push_back(_shield);

    for (int ii = 0; ii < 2; ++ii) {
        weapon_info info = weapon::by_random(_random);
        _weapons.push_back(get_world()->spawn<weapon>(this, info, vec2(11.f, ii ? 6.f : -6.f)));
        _subsystems.push_back(_weapons.back());
    }

    std::vector<handle<subsystem>> assignments(_subsystems.begin(), _subsystems.end());
    for (auto& ch : _crew) {
        if (assignments.size()) {
            std::size_t index = _random.uniform_int(assignments.size());
            ch->assign(assignments[index]);
            assignments.erase(assignments.begin() + index);
        }
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

        vec2 position = get_position(time) - vec2(8.f * (_subsystems.size() - 2.f) * .5f, 40.f);

        for (auto const& subsystem : _subsystems) {
            // reactor subsystem ui is drawn explicitly
            if (subsystem->info().type == subsystem_type::reactor) {
                continue;
            }

            // draw power bar for each subsystem power level
            for (int ii = 0; ii < subsystem->maximum_power(); ++ii) {
                bool damaged = ii >= subsystem->maximum_power() - std::ceil(subsystem->damage() - .2f);
                bool powered = ii < subsystem->current_power();

                color4 c = subsystem_colors[powered][damaged]; c.a *= alpha;

                renderer->draw_box(vec2(7,3), position + vec2(0, 10.f + 4.f * ii), c);
            }

            for (auto const& ch : _crew) {
                if (ch->assignment() == subsystem) {
                    color4 c = ch->health() ? color4(1,1,1,alpha) : color4(1,.2f,0,alpha);
                    renderer->draw_box(vec2(7,3), position + vec2(0,10 - 4), c);
                }
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

    if (_dead_time > time) {
        for (auto& subsystem : _subsystems) {
            if (subsystem->damage()) {
                subsystem->repair(1.f / 15.f);
                break;
            }
        }
        _shield->recharge(1.f / 5.f);
    }

    //
    // Death sequence
    //

    if (time > _dead_time) {
        if (time - _dead_time < destruction_time) {
            float t = min(.8f, (time - _dead_time) / destruction_time);
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
            _crew.clear();
            _subsystems.clear();
            _weapons.clear();

            _is_destroyed = true;
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
void ship::damage(object* inflictor, vec2 /*point*/, float amount)
{
    // get list of subsystems that can take additional damage
    std::vector<subsystem*> subsystems;
    for (auto& subsystem : _subsystems) {
        if (subsystem->damage() < subsystem->maximum_power()) {
            subsystems.push_back(subsystem.get());
        }
    }

    // apply damage to a random subsystem
    if (subsystems.size()) {
        std::size_t idx = _random.uniform_int(subsystems.size());
        subsystems[idx]->damage(inflictor, amount * 6.f);
        for (auto& ch : _crew) {
            if (ch->assignment() == subsystems[idx]) {
                ch->damage(inflictor, amount);
            }
        }
    }
}

//------------------------------------------------------------------------------
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

} // namespace game

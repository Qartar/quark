// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_character.h"
#include "g_navigation.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_subsystem.h"
#include "r_model.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

#define QBZ(a,b,c,t)        \
    (((1-t)*(1-t)*(a)+2*(1-t)*t*(b)+t*t*(c)))

#define QBZ8(a,b,c)         \
    QBZ(a,b,c,0),           \
    QBZ(a,b,c,0.125f),      \
    QBZ(a,b,c,0.25f),       \
    QBZ(a,b,c,0.375f),      \
    QBZ(a,b,c,0.5f),        \
    QBZ(a,b,c,0.625f),      \
    QBZ(a,b,c,0.75f),       \
    QBZ(a,b,c,0.875)

#define SHIP(L,B) {         \
    QBZ8(vec2(0.5f * L, 0.f), vec2(0.3f * L, 0.5f * B), vec2(0.f, 0.5f * B)),   \
    QBZ8(vec2(0.f, 0.5f * B), vec2(-0.5f * L, 0.5f * B), vec2(-0.5f * L, 0.f)),   \
    QBZ8(vec2(-0.5f * L, 0.f), vec2(-0.5f * L, -0.5f * B), vec2(0.f, -0.5f * B)),   \
    QBZ8(vec2(0.f, -0.5f * B), vec2(0.3f * L, -0.5f * B), vec2(0.5f * L, 0.f)),   }

const vec2 ship_hulls[][32] = {
    // yamato-class battleship
    SHIP(263.f, 39.f),

    // iowa-class battleship
    SHIP(270.f, 33.f),

    // king george v-class battleship
    SHIP(227.f, 31.5f),

    // deutschland-class cruiser
    SHIP(186.f, 21.7f),

    // town-class cruiser
    SHIP(180.f, 19.f),

    // tribal-class destroyer
    SHIP(115.f, 11.f),
};

const ship_info ships_info[] =
{
    {
        string::buffer("Yamato-class battleship"),
        263.f, 39.f, 0.f,
        //{{{std::make_unique<physics::convex_shape>(SHIP(263.f, 39.f))}}},
        {{{std::make_unique<physics::convex_shape>(ship_hulls[0])}}},
        SHIP(263.f, 39.f),
        {
            // 46cm/45 Type 94
            {vec2(52,0), 7.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.5f, .46f, 20.7f},
            {vec2(30,0), 7.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.5f, .46f, 20.7f},
            {vec2(-65,0), 7.f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.5f, .46f, 20.7f},
        }
    },
    {
        string::buffer("Iowa-class battleship"),
        270.f, 33.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[1])}}},
        SHIP(270.f, 33.f),
        {
            // 16"/50 Mark 7
            {vec2(48,0), 6.5f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.25f, .406f, 20.f},
            {vec2(24,0), 6.5f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.25f, .406f, 20.f},
            {vec2(-48,0), 6.5f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 3, 2.25f, .406f, 20.f},
        }
    },
    {
        string::buffer("King George V-class battleship"),
        227.f, 31.5f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[2])}}},
        SHIP(227.f, 31.5f),
        {
            // BL 14-inch Mark VII
            {vec2(40,0), 6.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 4, 2.f, .3556f, 16.f},
            {vec2(16,0), 6.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 4, 2.f, .3556f, 16.f},
        }
    },
    {
        string::buffer("Deutschland-class heavy cruiser"),
        186.f, 21.7f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[3])}}},
        SHIP(186.f, 21.7f),
        {
            // 28 cm SK C/28
            {vec2(32,0), 5.5f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.75f, .28f, 13.9f},
            {vec2(-32,0), 5.5f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.75f, .28f, 13.9f},
        }
    },
    {
        string::buffer("Town-class light cruiser"),
        180.f, 19.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[4])}}},
        SHIP(180.f, 19.f),
        {
            // BL 6-inch Mark XXIII
            {vec2(32,0), 3.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.f, .152f, 7.6f},
            {vec2(16,0), 3.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.f, .152f, 7.6f},
            {vec2(-16,0), 3.f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.f, .152f, 7.6f},
            {vec2(-32,0), 3.f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 3, 1.f, .152f, 7.6f},
        }
    },
    {
        string::buffer("Tribal-class destroyer"),
        115.f, 11.f, 0.f,
        {{{std::make_unique<physics::convex_shape>(ship_hulls[5])}}},
        SHIP(115.f, 11.f),
        {
            // QF 4.7-inch Mark IX & XII
            {vec2(24,0), 2.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 2, 0.75f, .12f, 5.4f},
            {vec2(12,0), 2.f, 0, .75f * vec2(-math::pi, math::pi), math::pi, 2, 0.75f, .12f, 5.4f},
            {vec2(-12,0), 2.f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 2, 0.75f, .12f, 5.4f},
            {vec2(-24,0), 2.f, math::pi, .75f * vec2(-math::pi, math::pi), math::pi, 2, 0.75f, .12f, 5.4f},
        }
    },
};

static int ships_idx = 0;

//------------------------------------------------------------------------------
ship::ship()
    : _usercmd{}
    , _dead_time(time_value::max)
    , _is_destroyed(false)
    , _info(&ships_info[ships_idx++ % countof(ships_info)])
{
    _rigid_body = physics::rigid_body(&_info->shape, &_material, 1.f);

    _turrets.resize(_info->turrets.size(), {});

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

    _reactor = get_world()->spawn<subsystem>(this, subsystem_info{subsystem_type::reactor, 13});
    _subsystems.push_back(_reactor);

    _engines = get_world()->spawn<game::engines>(this, engines_info{16.f, .125f, 8.f, .0625f, .5f, .5f});
    _subsystems.push_back(_engines);

    _navigation = get_world()->spawn<game::navigation>(this);
    _subsystems.push_back(_navigation);

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
        constexpr color4 color(.8f,.9f,1.f,1.f);
        auto tx = get_transform(time);

        // draw hull outline
        {
            vec2 v0 = _info->outline[0] * tx;
            for (std::size_t ii = 1; ii < _info->outline.size(); ++ii) {
                vec2 v1 = _info->outline[ii] * tx;
                renderer->draw_line(v0, v1, color, color);
                v0 = v1;
            }
            vec2 v1 = _info->outline[0] * tx;
            renderer->draw_line(v0, v1, color, color);
        }

        // draw turrets
        for (std::size_t jj = 0, num = _turrets.size(); jj < num; ++jj) {
            auto const& turret = _info->turrets[jj];
            mat3 turret_tx = mat3::transform(turret.position, rot2(turret.orientation + _turrets[jj].traverse)) * tx;

            // draw turret outline
            vec2 f1 = vec2(turret.radius, .6f * turret.radius) * turret_tx;
            vec2 f2 = vec2(turret.radius, -.6f * turret.radius) * turret_tx;
            vec2 m1 = vec2(.3f * turret.radius, turret.radius) * turret_tx;
            vec2 m2 = vec2(.3f * turret.radius, -turret.radius) * turret_tx;
            vec2 m3 = vec2(-.3f * turret.radius, turret.radius) * turret_tx;
            vec2 m4 = vec2(-.3f * turret.radius, -turret.radius) * turret_tx;
            vec2 r1 = vec2(-2.f * turret.radius, .8f * turret.radius) * turret_tx;
            vec2 r2 = vec2(-2.f * turret.radius, -.8f * turret.radius) * turret_tx;

            renderer->draw_line(f1, f2, color, color);
            renderer->draw_line(f1, m1, color, color);
            renderer->draw_line(f2, m2, color, color);
            renderer->draw_line(m1, m3, color, color);
            renderer->draw_line(m2, m4, color, color);
            renderer->draw_line(m3, r1, color, color);
            renderer->draw_line(m4, r2, color, color);
            renderer->draw_line(r1, r2, color, color);

            // draw guns
            for (int ii = 0; ii < turret.num_guns; ++ii) {
                float x = turret.radius;
                float y = turret.spacing * (ii - .5f * (turret.num_guns - 1));
                vec2 v1 = vec2(x, y);

                vec2 pts[4] = {
                    (v1 + vec2(0, 1.5f * turret.caliber)) * turret_tx,
                    (v1 + vec2(0.9f * turret.length, .5f * turret.caliber)) * turret_tx,
                    (v1 + vec2(0.9f * turret.length, -.5f * turret.caliber)) * turret_tx,
                    (v1 + vec2(0, -1.5f * turret.caliber)) * turret_tx
                };
                renderer->draw_line(pts[0], pts[1], color, color);
                renderer->draw_line(pts[1], pts[2], color, color);
                renderer->draw_line(pts[2], pts[3], color, color);
            }
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
    }

    {
        vec2 cursor = _usercmd.cursor;
        auto tx = get_transform(time);
        for (std::size_t jj = 0, num = _turrets.size(); jj < num; ++jj) {
            auto const& turret = _info->turrets[jj];
            vec2 dir = normalize(cursor - turret.position * tx);

            float angle = atan2f(dir.y, dir.x) - get_rotation(time).radians() - _info->turrets[jj].orientation;
            angle -= math::twopi * std::round(angle / math::twopi); // normalize to [-pi,pi)
            _turrets[jj].traverse = clamp(angle, _info->turrets[jj].traverse[0], _info->turrets[jj].traverse[1]);
        }
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
                bounds b = _rigid_body.get_shape()->calculate_bounds(mat3_identity);
                vec2 v;
                do {
                    v = b.mins() + b.size() * vec2(_random.uniform_real(), _random.uniform_real());
                } while (!_rigid_body.get_shape()->contains_point(v));

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

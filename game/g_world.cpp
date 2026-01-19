// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_faction.h"
#include "g_projectile.h"
#include "g_ship.h"
#include "g_player.h"
#include "p_collide.h"
#include "p_trace.h"
#include "cm_ballistics.h"

#include <algorithm>
#include <set>

//------------------------------------------------------------------------------
struct ballistic_data
{
    float initial_velocity;
    float initial_angle;
    float range;
    float time;
    float impact_angle;
    float impact_velocity;
    float radius;
};

//------------------------------------------------------------------------------
void simulate_ballistic_coefficient(ballistic_data& data, ballistics::curve curve, time_delta dt, float bc)
{
    vec3 r = vec3_zero;
    vec3 v = vec3(cos(math::deg2rad(data.initial_angle)),
        0,
        sin(math::deg2rad(data.initial_angle))) * data.initial_velocity;

    data.time = ballistics::simulate(r, v, curve, bc, dt).to_seconds();
    data.range = r.x;
    data.impact_angle = math::rad2deg(atan2f(-v.z, v.x));
    data.impact_velocity = length(v);
}

//------------------------------------------------------------------------------
float compare_ballistic_data(ballistic_data const& b1, ballistic_data const& b2)
{
    float num = 0.f;
    float den = 0.f;

    if (b1.range && b2.range) {
        num += square(b1.range - b2.range) / (b1.range * b2.range);
        den += 1.f;
    }

    if (b1.time && b2.time) {
        num += square(b1.time - b2.time) / (b1.time * b2.time);
        den += 1.f;
    }

    if (b1.impact_angle && b2.impact_angle) {
        num += square(b1.impact_angle - b2.impact_angle) / (b1.impact_angle * b2.impact_angle);
        den += 1.f;
    }

    if (b1.impact_velocity && b2.impact_velocity) {
        num += square(b1.impact_velocity - b2.impact_velocity) / (b1.impact_velocity * b2.impact_velocity);
        den += 1.f;
    }

    return den ? num / den : 0.f;
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient(ballistic_data const& b, ballistics::curve c, time_delta dt, float& bc, float& rms)
{
    ballistic_data d;

    bc = 1e3f;
    rms = 0.f;
    float den = 0.f;

    if (b.range) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.range / d.range;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.time) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.time / d.time;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.impact_angle) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= d.impact_angle / b.impact_angle;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.impact_velocity) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.impact_velocity / d.impact_velocity;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (den) {
        rms /= den;
    }
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient(ballistic_data const* b, std::size_t n, ballistics::curve c, time_delta dt, float& bc, float& rms)
{
    rms = 0.f;
    for (std::size_t ii = 0; ii < n; ++ii) {
        float tmp = 0.f;
        solve_ballistic_coefficient(b[ii], c, dt, bc, tmp);
        rms += tmp;
    }
    rms /= n;
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient_cmd(parser::text const& args)
{
    (void)args;
#if 0
    {
        ballistic_data const data[] = {
            { 780.f, 10.f, 16830.f, 26.05f },
            { 780.f, 20.f, 27920.f, 49.21f },
            { 780.f, 30.f, 35830.f, 70.27f },
            { 780.f, 40.f, 40700.f, 89.42f },
            { 780.f, 45.f, 42030.f, 98.6f },
        };
    }
#endif
    {
        ballistic_data const data[] = {
            // 46 cm/45 Type 94 naval gun (Yamato)
            { 780.f,  2.4f,  5000.f, 0.f,  3.3f, 690, .46f },
            { 780.f,  5.4f, 10000.f, 0.f,  7.2f, 620, .46f },
            { 780.f,  8.6f, 15000.f, 0.f, 11.5f, 562, .46f },
            { 780.f, 12.6f, 20000.f, 0.f, 16.5f, 521, .46f },
            { 780.f, 17.2f, 25000.f, 0.f, 23.0f, 490, .46f },
            { 780.f, 23.2f, 30000.f, 0.f, 31.4f, 475, .46f },
            // 14-inch (35.6 cm) Mark VII (King George V)
            { 732.f,  2.5f,   4570.f,  6.59f,  2.8f, 658, .356f },
            { 732.f,  5.5f,   9140.f, 14.06f,  6.5f, 587, .356f },
            { 732.f,  9.25f, 13720.f, 22.57f, 11.5f, 526, .356f },
            { 732.f, 13.75f, 18290.f, 32.41f, 18.2f, 476, .356f },
            { 732.f, 19.25f, 22400.f, 43.86f, 26.4f, 445, .356f },
            { 732.f, 26.2f,  27430.f, 57.43f, 35.6f, 436, .356f },
            { 732.f, 36.0f,  32000.f, 74.97f, 46.1f, 452, .356f },
            { 732.f, 40.7f,  33380.f, 82.42f, 50.3f, 464, .356f },
            // 28 cm SK C/28 naval gun (Deutschland)
            { 910.f,  1.9f,  5000.f, 0.f,  2.4f, 752, .28f },
            { 910.f,  4.5f, 10000.f, 0.f,  6.0f, 611, .28f },
            { 910.f,  8.0f, 15000.f, 0.f, 11.8f, 493, .28f },
            { 910.f, 12.5f, 20000.f, 0.f, 21.4f, 407, .28f },
            { 910.f, 18.6f, 25000.f, 0.f, 34.2f, 360, .28f },
            { 910.f, 26.3f, 30000.f, 0.f, 46.4f, 353, .28f },
            { 910.f, 36.4f, 35000.f, 0.f, 56.0f, 380, .28f },
            // 6"/50 (15.2 cm) BL Mark XXIII (Town)
            { 823.f,  2.3f,  4570.f,  6.6f,  3.0f, 591, .152f },
            { 823.f,  6.2f,  9140.f, 15.9f, 10.0f, 418, .152f },
            { 823.f, 13.1f, 13720.f, 29.4f, 23.6f, 335, .152f },
            { 823.f, 24.1f, 18290.f, 47.2f, 39.9f, 331, .152f },
            { 823.f, 41.1f, 22400.f, 71.4f, 56.5f, 353, .152f },
        };

        const char* curves[] = { "G1", "G2", "G5", "G6", "G7", "G8" };

        for (std::size_t ii = 0, jj = 0; jj < countof(data); ++ii) {
            while (jj < countof(data) && data[ii].initial_velocity == data[jj].initial_velocity) {
                ++jj;
            }

            float best_bc = 0.f, best_rms = FLT_MAX;
            ballistics::curve best_curve = ballistics::curve::G1;
            for (std::size_t kk = 0; kk < 6; ++kk) {
                float bc, rms;
                ballistics::curve c = static_cast<ballistics::curve>(static_cast<std::size_t>(ballistics::curve::G1) + kk);
                solve_ballistic_coefficient(data + ii, jj - ii, c, FRAMETIME, bc, rms);
                log::message(" %s %.16f %.16f %.16f\n", curves[static_cast<std::size_t>(c)], rms, bc, bc / data[ii].radius);
                if (rms < best_rms) {
                    best_bc = bc;
                    best_rms = rms;
                    best_curve = c;
                }
            }
            log::message("%zu-%zu %.16f %.16f %s %.16f\n", ii, jj - 1, best_rms, best_bc, curves[static_cast<std::size_t>(best_curve)], best_bc / data[ii].radius);

            for (std::size_t kk = ii; kk < jj; ++kk) {
                ballistic_data d = data[kk];
                simulate_ballistic_coefficient(d, best_curve, FRAMETIME, best_bc);

                log::message(" %zu %.16f %.1fs (%g vs %g m) (%g vs %g m/s) (%2.1f vs %2.1f)\n", kk, best_bc,
                    d.time,
                    d.range, data[kk].range,
                    d.impact_velocity, data[kk].impact_velocity,
                    d.impact_angle, data[kk].impact_angle);
            }
            log::message("\n");

            ii = jj - 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
namespace game {

std::array<world*, world::max_worlds> world::_singletons{};

//------------------------------------------------------------------------------
world::world()
    : _sequence(0)
    , _physics(
        std::bind(&world::physics_filter_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&world::physics_collide_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
    static console_command cmd("solve_ballistics", &solve_ballistic_coefficient_cmd);
    for (_index = 0; _index < max_worlds; ++_index) {
        if (!_singletons[_index]) {
            _singletons[_index] = this;
            break;
        }
    }

    assert(_index < max_worlds);
}

//------------------------------------------------------------------------------
world::~world()
{
    assert(_singletons[_index] == this);
    _singletons[_index] = nullptr;
}

//------------------------------------------------------------------------------
void world::init()
{
    reset();
}

//------------------------------------------------------------------------------
void world::shutdown()
{
    clear();
}

//------------------------------------------------------------------------------
void world::reset()
{
    clear();

    _sequence = 0;
    _framenum = 0;

    faction* blufor = spawn<faction>("blufor", color4(.6f, .8f, 1.f, 1.f));
    faction* opfor = spawn<faction>("opfor", color4(1.f, .6f, .6f, 1.f));

    for (int ii = 0; ii < 6; ++ii) {
        float angle = float(ii) * (math::pi * 2.f / 6.f);
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>(blufor);
        sh->set_position(-dir * 1024.f, true);
        sh->set_rotation(rot2(angle + math::pi * .75f), true);

        // spawn ai controller to control the ship
        spawn<aicontroller>(sh);
    }

    for (int ii = 0; ii < 6; ++ii) {
        float angle = float(ii) * (math::pi * 2.f / 6.f);
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>(opfor);
        sh->set_position(vec2(16384, 0) - dir * 1024.f, true);
        sh->set_rotation(rot2(angle + math::pi * .75f), true);

        // spawn ai controller to control the ship
        spawn<aicontroller>(sh);
    }
}

//------------------------------------------------------------------------------
void world::clear()
{
    _objects.clear();
    // assign with empty queue because std::queue has no clear method
    _removed = std::queue<handle<game::object>>{};

    _particles.clear();
}

//------------------------------------------------------------------------------
object_range<object const> world::objects() const
{
    return object_range<object const>(
        _objects.data(),
        _objects.data() + _objects.size()
    );
}

//------------------------------------------------------------------------------
object_range<object> world::objects()
{
    return object_range<object>(
        _objects.data(),
        _objects.data() + _objects.size()
    );
}

//------------------------------------------------------------------------------
void world::remove(handle<object> object)
{
    _removed.push(object);
}

//------------------------------------------------------------------------------
void world::draw(render::system* renderer, time_value time) const
{
    renderer->draw_starfield();

    for (auto& obj : _objects) {
        // objects array is sparse
        if (!obj.get()) {
            continue;
        }
        obj->draw(renderer, time);
    }

    draw_particles(renderer, time);
}

//------------------------------------------------------------------------------
void world::run_frame()
{
    _message.reset();

    ++_framenum;

    while (_removed.size()) {
        if (_removed.front()) {
            _objects[_removed.front().get_index()] = nullptr;
        }
        _removed.pop();
    }

    for (std::size_t ii = 0; ii < _objects.size(); ++ii) {
        // objects array is sparse
        if (!_objects[ii].get()) {
            continue;
        }

        // objects can spawn other objects, do not think this frame
        if (_objects[ii]->_spawn_time == frametime()) {
            continue;
        }

        _objects[ii]->think();

        _objects[ii]->_old_position = _objects[ii]->get_position();
        _objects[ii]->_old_rotation = _objects[ii]->get_rotation();
    }

    _physics.step(FRAMETIME.to_seconds());
}

//------------------------------------------------------------------------------
void world::read_snapshot(network::message& message)
{
    while (_removed.size()) {
        if (_removed.front()) {
            _objects[_removed.front().get_index()] = nullptr;
        }
        _removed.pop();
    }

    while (message.bytes_remaining()) {
        message_type type = static_cast<message_type>(message.read_byte());
        if (type == message_type::none) {
            break;
        }

        switch (type) {
            case message_type::frame:
                read_frame(message);
                break;

            case message_type::sound:
                read_sound(message);
                break;

            case message_type::effect:
                read_effect(message);
                break;

            default:
                break;
        }
    }
}

//------------------------------------------------------------------------------
void world::read_frame(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void world::read_sound(network::message const& message)
{
    int asset = message.read_long();
    vec2 position = message.read_vector();
    float volume = message.read_float();

    add_sound(static_cast<sound::asset>(asset), position, volume);
}

//------------------------------------------------------------------------------
void world::read_effect(network::message const& message)
{
    float time = message.read_float();
    int type = message.read_byte();
    vec2 pos = message.read_vector();
    vec2 vel = message.read_vector();
    float strength = message.read_float();

    add_effect(time_value::from_seconds(time), static_cast<game::effect_type>(type), pos, vel, strength);
}

//------------------------------------------------------------------------------
void world::write_snapshot(network::message& message) const
{
    message.write_byte(svc_snapshot);

    // write frame
    message.write_byte(narrow_cast<uint8_t>(message_type::frame));
    message.write_long(_framenum);

    // write active objects
    // ...

    // write sounds and effects
    message.write(_message);
    message.write_byte(narrow_cast<uint8_t>(message_type::none));

    _message.rewind();
}

//------------------------------------------------------------------------------
void world::write_sound(sound::asset sound_asset, vec2 position, float volume)
{
    _message.write_byte(narrow_cast<uint8_t>(message_type::sound));
    _message.write_long(narrow_cast<int>(sound_asset));
    _message.write_vector(position);
    _message.write_float(volume);
}

//------------------------------------------------------------------------------
void world::write_effect(time_value time, effect_type type, vec2 position, vec2 direction, float strength)
{
    _message.write_byte(narrow_cast<uint8_t>(message_type::effect));
    _message.write_float(time.to_seconds());
    _message.write_byte(narrow_cast<uint8_t>(type));
    _message.write_vector(position);
    _message.write_vector(direction);
    _message.write_float(strength);
}

//------------------------------------------------------------------------------
game::object* world::trace(physics::contact& contact, vec2 start, vec2 end, game::object const* ignore) const
{
    // TODO: Should either expose trace results to the caller or push the filtering
    // into the physics world to avoid an arbitrarily sized results array here.
    physics::world::trace_result tr[32];

    std::size_t num_results = _physics.trace(start, end, tr, ignore ? countof(tr) : 1);
    for (std::size_t ii = 0; ii < num_results; ++ii) {
        game::object* obj = handle<object>(tr[ii].body->get_handle_bits()).get();
        if (ignore == obj || (ignore && ignore == obj->_owner)) {
            continue;
        }
        contact = tr[ii].c;
        return obj;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void world::add_sound(sound::asset sound_asset, vec2 position, float volume)
{
    write_sound(sound_asset, position, volume);
    pSound->play(sound_asset, vec3(position), volume, 1.0f);
}

//------------------------------------------------------------------------------
void world::add_body(game::object* owner, physics::rigid_body* body)
{
    _physics.add_body(body);
    body->set_handle_bits(owner->_self._value);
}

//------------------------------------------------------------------------------
void world::remove_body(physics::rigid_body* body)
{
    _physics.remove_body(body);
    body->set_handle_bits(0);
}

//------------------------------------------------------------------------------
bool world::physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b)
{
    game::object* obj_a = handle<object>(body_a->get_handle_bits()).get();
    game::object* obj_b = handle<object>(body_b->get_handle_bits()).get();

    if (obj_b->is_type<projectile>()) {
        return false;
    }

    game::object const* owner_a = obj_a->_owner ? obj_a->_owner.get() : obj_a;
    game::object const* owner_b = obj_b->_owner ? obj_b->_owner.get() : obj_b;

    if (owner_a == owner_b) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool world::physics_collide_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision)
{
    game::object* obj_a = handle<object>(body_a->get_handle_bits()).get();
    game::object* obj_b = handle<object>(body_b->get_handle_bits()).get();

    return obj_a->touch(obj_b, &collision);
}

} // namespace game

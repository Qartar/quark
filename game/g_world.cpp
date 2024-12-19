// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_projectile.h"
#include "g_ship.h"
#include "g_player.h"
#include "p_collide.h"
#include "p_trace.h"

#include "cm_trajectory.h"

#include <algorithm>
#include <set>

////////////////////////////////////////////////////////////////////////////////
namespace game {

std::array<world*, world::max_worlds> world::_singletons{};

//------------------------------------------------------------------------------
world::world()
    : _sequence(0)
    , _system(random())
    , _physics(
        std::bind(&world::physics_filter_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&world::physics_collide_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
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
#if 0
    for (int ii = 0; ii < 6; ++ii) {
        float angle = float(ii) * (math::pi<float> * 2.f / 6.f);
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>(ship::by_random(_random));
        sh->set_position(-dir * 2560.f, true);
        sh->set_rotation(angle, true);

        // spawn ai controller to control the ship
        if (ii == 0) {
            spawn<player>(sh);
        } else {
            spawn<aicontroller>(sh);
        }
    }
#endif
}

//------------------------------------------------------------------------------
void world::clear()
{
    _objects.clear();
    // assign with empty queue because std::queue has no clear method
    _removed = std::queue<handle<game::object>>{};

    _physics_objects.clear();
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

#if 1
    std::vector<vec2> positions;
    positions.resize(_system.num_bodies());
    _system.calculate_all_positions(time * 10000.f, positions.data());

    for (std::size_t ii = 0; ii < positions.size(); ++ii) {
        renderer->draw_arc(positions[ii] * 10.f, max(0.001f, _system.bodies()[ii].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(1,.2f,0,1));
#if 1
        auto const& orbit = _system.bodies()[ii].orbit;
        if (orbit.semimajor_axis == 0.f) {
            continue;
        }
        if (_system.bodies()[ii].mass < 1e15f) {
            continue;
        }

        float semilatus_rectum = orbit.semimajor_axis * (1.f - square(orbit.eccentricity));
        vec2 p0 = semilatus_rectum / (1.f + orbit.eccentricity) * vec2(cos(orbit.longitude_of_periapsis), sin(orbit.longitude_of_periapsis));
        for (std::size_t jj = 0; jj < 64; ++jj) {
            float f = float(jj + 1) * (2.f * math::pi<float> / float(64));
            vec2 p1 = semilatus_rectum / (1.f + orbit.eccentricity * cos(f)) * vec2(cos(f + orbit.longitude_of_periapsis), sin(f + orbit.longitude_of_periapsis));

            renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1.f,0,0,.2f), color4(1.f,0,0,.2f));
            p0 = p1;
        }
#endif
    }

#if 1
    std::size_t idx_a = config::integer("src", 1, 0, "");
    std::size_t idx_b = config::integer("dst", 3, 0, "");
    system::orbit const& a = _system.bodies()[idx_a].orbit;
    system::orbit const& b = _system.bodies()[idx_b].orbit;

    vec2 r0 = a.calculate_position(time * 10000.f);
    vec2 r1 = b.calculate_position(time * 10000.f);
    // d = vt + at^2/2
    // t = sqrt(2d/a)
    // t = 2sqrt(2(d/2)/a) = 2sqrt(d/a)
    const float accel = config::scalar("accel", 10.f, 0, "") * 1e-9f; // 10m/s^2 in gigameters
    time_delta dt = time_delta::from_seconds(2.f * sqrt((r1 - r0).length() / accel));
    for (std::size_t ii = 0; ii < 8; ++ii) {
        r1 = b.calculate_position(time * 10000.f + dt);
        dt = time_delta::from_seconds(2.f * sqrt((r1 - r0).length() / accel));
    }

    //renderer->draw_line(r0 * 10.f, r1 * 10.f, color4(.6f,10,0,1.f), color4(.6f,10,0,1.f));

#if 0 // draw velocity-free course
    {
        vec2 p0 = r0;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() * ii / 64.f;
            float x = .5f * accel * t * t;
            vec2 p1 = r0 + (r1 - r0).normalize() * x;
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(.6f,1,0,1.f), color4(.6f,1,0,1.f));
            }
            p0 = p1;
        }
        float ht = .5f * dt.to_seconds();
        float hx = .5f * accel * square(ht);
        float hv = accel * ht;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() * ii / 64.f;
            float x = hx + hv * t - .5f * accel * t * t;
            vec2 p1 = r0 + (r1 - r0).normalize() * x;
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(.6f,1,0,1.f), color4(.6f,1,0,1.f));
            }
            p0 = p1;
        }

        renderer->draw_arc(r1 * 10.f, max(0.001f, _system.bodies()[idx_b].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(.6f,1,0,1.f));
    }
#endif

#if 0 // draw gravity-free course
    {
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        vec2 p0 = r0;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() * ii / 64.f;
            float x = .5f * accel * t * t;
            vec2 p1 = r0 + (r1 - r0).normalize() * x + v0 * t;
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.6f,0,1.f), color4(1,.6f,0,1.f));
            }
            p0 = p1;
        }
        float ht = .5f * dt.to_seconds();
        float hx = .5f * accel * square(ht);
        float hv = accel * ht;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() * ii / 64.f;
            float x = hx + hv * t - .5f * accel * t * t;
            vec2 p1 = r0 + (r1 - r0).normalize() * x + v0 * (t + ht);
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.6f,0,1.f), color4(1,.6f,0,1.f));
            }
            p0 = p1;
        }
    }
#endif

#if 1 // draw source body full orbit
    {
        float mu = (_system.bodies()[0].mass + _system.bodies()[idx_a].mass) * system::gravitational_constant;
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        trajectory tr(mu, time_value::zero, r0, v0);
        vec2 p0 = r0;

        for (std::size_t ii = 1; ii <= 128; ++ii) {
            time_value t = time_value::zero + a.period * ii / 128.f;
            vec2 p1, v1; tr.calculate(t, &p1, &v1);
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(0,.6f,1,1.f), color4(0,.6f,1,1.f));
            }
            p0 = p1;
        }
    }
#endif

#if 0 // draw target body partial orbit
    {
        float mu = (_system.bodies()[0].mass + _system.bodies()[idx_b].mass) * system::gravitational_constant;
        vec2 p0 = b.calculate_position(time * 10000.f);
        vec2 v0 = b.calculate_velocity(time * 10000.f);
        trajectory tr(mu, time_value::zero, p0, v0);

        for (std::size_t ii = 1; ii <= 64; ++ii) {
            time_value t = time_value::zero + dt * ii / 64.f;
            vec2 p1, v1; tr.calculate(t, &p1, &v1);
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.2f,0,1), color4(1,.2f,0,1));
            }
            p0 = p1;
        }
    }
#endif

#if 1 // draw low-resolution gravity course
    {
        float mu = _system.bodies()[0].mass * system::gravitational_constant;
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        time_value step = time_value::zero + dt / 64;
        vec2 p0 = r0;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() / 64.f;

            trajectory tr(mu, time_value::zero, p0, v0);
            vec2 p1, v1; tr.calculate(step, &p1, &v1);

            p1 += (r1 - r0).normalize() * .5f * accel * t * t;
            v1 += (r1 - r0).normalize() * accel * t;

            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,0,.6f,1.f), color4(1,0,.6f,1.f));
            }
            p0 = p1;
            v0 = v1;
        }

        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dt.to_seconds() / 64.f;

            trajectory tr(mu, time_value::zero, p0, v0);
            vec2 p1, v1; tr.calculate(step, &p1, &v1);

            p1 -= (r1 - r0).normalize() * .5f * accel * t * t;
            v1 -= (r1 - r0).normalize() * accel * t;

            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,0,.6f,1.f), color4(1,0,.6f,1.f));
            }
            p0 = p1;
            v0 = v1;
        }
    }
#endif

#if 1 // draw high-resolution gravity course
    {
        float mu = _system.bodies()[0].mass * system::gravitational_constant;
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        time_value step = time_value::zero + dt / 1024;
        vec2 p0 = r0;
        for (std::size_t ii = 1; ii <= 512; ++ii) {
            float t = dt.to_seconds() / 1024.f;

            trajectory tr(mu, time_value::zero, p0, v0);
            vec2 p1, v1; tr.calculate(step, &p1, &v1);

            p1 += (r1 - r0).normalize() * .5f * accel * t * t;
            v1 += (r1 - r0).normalize() * accel * t;

            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(.6f,0,1,1.f), color4(.6f,0,1,1.f));
            }
            p0 = p1;
            v0 = v1;
        }

        for (std::size_t ii = 1; ii <= 512; ++ii) {
            float t = dt.to_seconds() / 1024.f;

            trajectory tr(mu, time_value::zero, p0, v0);
            vec2 p1, v1; tr.calculate(step, &p1, &v1);

            p1 -= (r1 - r0).normalize() * .5f * accel * t * t;
            v1 -= (r1 - r0).normalize() * accel * t;

            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(.6f,0,1,1.f), color4(.6f,0,1,1.f));
            }
            p0 = p1;
            v0 = v1;
        }
    }
#endif

#if 0 // draw quadratic intercept grid
    {
        r1 = b.calculate_position(time * 10000.f);
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        vec2 v1 = b.calculate_velocity(time * 10000.f);
        dt = time_delta::from_seconds(2.f * sqrt((r1 - r0).length() / accel));
        time_delta dtm = dt / 2;
        vec2 a0 = (r1 - r0).normalize() * accel;
        vec2 a1 = -a0;

        constexpr std::size_t N = 65;
        vec2 grid[N][N];

        vec2 ddx = (r1 - r0) / N;
        vec2 ddy = ddx.cross(1);

        for (std::size_t jj = 0; jj < N; ++jj) {
            for (std::size_t ii = 0; ii < N; ++ii) {
                r1 = b.calculate_position(time * 10000.f + dt);
                v1 = b.calculate_velocity(time * 10000.f + dt);
                vec2 dr0 = r1 - r0 + (ddx * (float(jj) - N/2)) + (ddy * (float(ii) - N/2));
                float dx = dr0.normalize_length();

                // DEBUG eliminate all lateral velocity
                {
                    //v0 = a.calculate_velocity(time * 10000.f);
                    //v1 = dr0 * dot(v1, dr0);
                    //v0 = dr0 * dot(v0, dr0);
                }

                float vx0 = dot(v0, dr0);
                float vx1 = dot(v1, dr0);
                float dvy = cross(v1 - v0, dr0);
                float ay = dvy / dt.to_seconds();
                float ax = accel * sqrt(1.f - square(ay / accel));
                // solve quadratic
                {
                    float A = ax * (-1.f / 4.f);
                    float B = (vx1 + vx0) * (-1.f / 2.f);
                    float C = dx + square(vx1 - vx0) / (4.f * ax);
                    float D = B * B - 4.f * A * C;
                    float Q = -.5f * (B + std::copysign(std::sqrt(D), B));
                    dt = time_delta::from_seconds(max(Q / A, C / Q));
                }
                dtm = dt / 2 + time_delta::from_seconds((vx1 - vx0) / (2.f * ax));
                a0 = dr0.cross(ay) + dr0 * ax;
                a1 = dr0.cross(ay) - dr0 * ax;

                grid[ii][jj] = r0 + v0 * dt.to_seconds() + .5f * a0 * square(dtm.to_seconds())
                    + a0 * dtm.to_seconds() * (dt - dtm).to_seconds()
                    + .5f * a1 * square((dt - dtm).to_seconds());

                //renderer->draw_arc(end * 10.f, max(0.001f, _system.bodies()[idx_b].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(0,.6f,1,1.f));
            }
        }

        for (std::size_t jj = 0; jj < N; jj += 2) {
            for (std::size_t ii = 0; ii + 1 < N; ++ii) {
                renderer->draw_line(grid[ii][jj] * 10.f, grid[ii + 1][jj] * 10.f, color4(0,.6f,1,.5f), color4(0,.6f,1,.5f));
                renderer->draw_line(grid[jj][ii] * 10.f, grid[jj][ii + 1] * 10.f, color4(0,.6f,1,.5f), color4(0,.6f,1,.5f));
            }
        }
    }
#endif

#if 1 // draw quadratic intercept course
    {

        r1 = b.calculate_position(time * 10000.f);
        vec2 v0 = a.calculate_velocity(time * 10000.f);
        vec2 v1 = b.calculate_velocity(time * 10000.f);
        dt = time_delta::from_seconds(2.f * sqrt((r1 - r0).length() / accel));
        time_delta dtm = dt / 2;
        vec2 a0 = (r1 - r0).normalize() * accel;
        vec2 a1 = -a0;
        vec2 err = vec2_zero;
        for (std::size_t ii = 0; ii < 8; ++ii) {
            r1 = b.calculate_position(time * 10000.f + dt);
            v1 = b.calculate_velocity(time * 10000.f + dt);
            vec2 dr0 = r1 - r0 - err;
            float dx = dr0.normalize_length();

            // DEBUG eliminate all lateral velocity
            {
                //v0 = a.calculate_velocity(time * 10000.f);
                //v1 = dr0 * dot(v1, dr0);
                //v0 = dr0 * dot(v0, dr0);
            }

            float vx0 = dot(v0, dr0);
            float vx1 = dot(v1, dr0);
            float dvy = cross(v1 - v0, dr0);
            float ay = dvy / dt.to_seconds();
            float ax = accel * sqrt(1.f - square(ay / accel));
            // solve quadratic
            {
                float A = ax * (-1.f / 4.f);
                float B = (vx1 + vx0) * (-1.f / 2.f);
                float C = dx + square(vx1 - vx0) / (4.f * ax);
                float D = B * B - 4.f * A * C;
                float Q = -.5f * (B + std::copysign(std::sqrt(D), B));
                dt = time_delta::from_seconds(max(Q / A, C / Q));
            }
            dtm = dt / 2 + time_delta::from_seconds((vx1 - vx0) / (2.f * ax));
            a0 = dr0.cross(ay) + dr0 * ax;
            a1 = dr0.cross(ay) - dr0 * ax;

            err += r0 + v0 * dt.to_seconds() + .5f * a0 * square(dtm.to_seconds())
                + a0 * dtm.to_seconds() * (dt - dtm).to_seconds()
                + .5f * a1 * square((dt - dtm).to_seconds()) - r1;
#if 0
            vec2 end = r0 + v0 * dt.to_seconds() + .5f * a0 * square(dtm.to_seconds())
                + a0 * dtm.to_seconds() * (dt - dtm).to_seconds()
                + .5f * a1 * square((dt - dtm).to_seconds());
            renderer->draw_arc(end * 10.f, max(0.001f, _system.bodies()[idx_b].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(0,.6f,1,1.f));
#endif
        }

        vec2 p0 = r0;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = dtm.to_seconds() * ii / 32.f;
            vec2 p1 = r0 + v0 * t + .5f * a0 * t * t;
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.6f,0,1.f), color4(1,.6f,0,1.f));
            }
            p0 = p1;
        }
        float ht = dtm.to_seconds();
        vec2 hr = r0 + v0 * ht + .5f * a0 * square(ht);
        vec2 hv = v0 + a0 * ht;
        for (std::size_t ii = 1; ii <= 32; ++ii) {
            float t = (dt - dtm).to_seconds() * ii / 32.f;
            vec2 p1 = hr + hv * t + .5f * a1 * t * t;
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.6f,0,1.f), color4(1,.6f,0,1.f));
            }
            p0 = p1;
        }

        r1 = b.calculate_position(time * 10000.f + dt);
        renderer->draw_arc(r1 * 10.f, max(0.001f, _system.bodies()[idx_b].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(1,.6f,0,1.f));
    }
#endif

#if 1 // draw target body partial orbit
    {
        float mu = (_system.bodies()[0].mass + _system.bodies()[idx_b].mass) * system::gravitational_constant;
        vec2 p0 = b.calculate_position(time * 10000.f);
        vec2 v0 = b.calculate_velocity(time * 10000.f);
        trajectory tr(mu, time_value::zero, p0, v0);

        for (std::size_t ii = 1; ii <= 64; ++ii) {
            time_value t = time_value::zero + dt * ii / 64.f;
            vec2 p1, v1; tr.calculate(t, &p1, &v1);
            if (!(ii & 1)) {
                renderer->draw_line(p0 * 10.f, p1 * 10.f, color4(1,.2f,0,1), color4(1,.2f,0,1));
            }
            p0 = p1;
        }
    }
#endif

    //vec2 r2 = a.calculate_position(time * 10000.f + dt);
    //renderer->draw_arc(r2 * 10.f, max(0.001f, _system.bodies()[idx_a].radius) * 100.f, 0.f, 0.f, 2.f * math::pi<float>, color4(1,.6f,0,1.f));
#endif

#endif
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

        vec2 old_position = _objects[ii]->get_position();
        float old_rotation = _objects[ii]->get_rotation();

        _objects[ii]->think();

        _objects[ii]->_old_position = old_position;
        _objects[ii]->_old_rotation = old_rotation;
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
    struct candidate {
        float fraction;
        physics::contact contact;
        game::object* object;

        bool operator<(candidate const& other) const {
            return fraction < other.fraction;
        }
    };

    std::set<candidate> candidates;
    for (auto& other : _physics_objects) {
        if (other.second == ignore || other.second->_owner == ignore) {
            continue;
        }

        auto tr = physics::trace(other.first, start, end);
        if (tr.get_fraction() < 1.0f) {
            candidates.insert(candidate{
                tr.get_fraction(),
                tr.get_contact(),
                other.second}
            );
        }
    }

    if (candidates.size()) {
        contact = candidates.begin()->contact;
        return candidates.begin()->object;
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
    _physics_objects[body] = owner;
}

//------------------------------------------------------------------------------
void world::remove_body(physics::rigid_body* body)
{
    _physics.remove_body(body);
    _physics_objects.erase(body);
}

//------------------------------------------------------------------------------
bool world::physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b)
{
    game::object* obj_a = _physics_objects[body_a];
    game::object* obj_b = _physics_objects[body_b];

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
    game::object* obj_a = _physics_objects[body_a];
    game::object* obj_b = _physics_objects[body_b];

    return obj_a->touch(obj_b, &collision);
}

} // namespace game

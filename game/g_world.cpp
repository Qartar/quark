// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_faction.h"
#include "g_navigation.h"
#include "g_projectile.h"
#include "g_ship.h"
#include "g_player.h"
#include "p_collide.h"
#include "p_trace.h"
#include "cm_ballistics.h"

#include <algorithm>
#include <set>

////////////////////////////////////////////////////////////////////////////////
namespace game {

std::array<world*, world::max_worlds> world::_singletons{};

//------------------------------------------------------------------------------
world::world()
    : _sequence(0)
    , _physics(
        std::bind(&world::physics_filter_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&world::physics_collide_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _timescale("timescale", 1, config::server, "rate of game time relative to real time")
    , _prev_timescale(1)
{
    static console_command cmd("solve_ballistics", &ballistics::solve_ballistic_coefficient_cmd);
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
    clear();
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

    faction* blufor = spawn<faction>("blufor", color4(.6f, .8f, 1.f, 1.f));
    faction* opfor = spawn<faction>("opfor", color4(1.f, .6f, .6f, 1.f));

    for (int ii = 0; ii < 6; ++ii) {
        float angle = float(ii) * (math::pi * 2.f / 6.f) + math::pi / 12.f;
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>(blufor);
        sh->set_position(-dir * 1024.f, true);
        sh->set_rotation(rot2(math::pi * .5f), true);

        sh->navigation()->set_heading(rot2(0,1));
    }

    for (int ii = 0; ii < 6; ++ii) {
        float angle = float(ii) * (math::pi * 2.f / 6.f) + math::pi / 12.f;
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>(opfor);
        sh->set_position(vec2(16384, 0) - dir * 1024.f, true);
        sh->set_rotation(rot2(math::pi * .5f), true);

        sh->navigation()->set_heading(rot2(0,1));
    }
}

//------------------------------------------------------------------------------
void world::clear()
{
    _objects.clear();
    // assign with empty queue because std::queue has no clear method
    _removed = std::queue<handle<game::object>>{};

    _particles.clear();

    _sequence = 0;
    _framenum = 0;
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

        _objects[ii]->_old_position = _objects[ii]->get_position();
        _objects[ii]->_old_rotation = _objects[ii]->get_rotation();

        _objects[ii]->think();
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
    _message.write_float(float(time.to_seconds()));
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
game::object* world::point_query(vec2 point) const
{
    physics::rigid_body* body = _physics.point_query(point);
    if (body) {
        return handle<object>(body->get_handle_bits()).get();
    }
    return nullptr;
}

//------------------------------------------------------------------------------
std::size_t world::bounds_query(bounds b, game::object** objects, std::size_t max_objects_) const
{
    physics::rigid_body** bodies = reinterpret_cast<physics::rigid_body**>(objects);
    std::size_t num_bodies = _physics.bounds_query(b, bodies, max_objects_);
    std::size_t num_objects = 0;

    for (std::size_t ii = 0; ii < num_bodies; ++ii) {
        if (bodies[ii]) {
            objects[num_objects] = handle<object>(bodies[ii]->get_handle_bits()).get();
            if (objects[num_objects]) {
                num_objects++;
            }
        }
    }
    return num_objects;
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

//------------------------------------------------------------------------------
void world::on_speed_up()
{
    _timescale = clamp(3.f * _timescale, 1.f, 81.f);
}

//------------------------------------------------------------------------------
void world::on_speed_down()
{
    if (_timescale > 1.f) {
        _prev_timescale = 1.f;
        _timescale = (1.f / 3.f) * _timescale;
    } else {
        _timescale = 0.f;
    }
}

//------------------------------------------------------------------------------
void world::on_pause()
{
    if (_timescale) {
        _prev_timescale = _timescale;
        _timescale = 0.f;
    } else {
        _timescale = _prev_timescale;
    }
}

} // namespace game

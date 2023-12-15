// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "p_collide.h"
#include "p_trace.h"

#include "g_train.h"

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
    , _framenum(0)
    , _rail_network(this)
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

    _rail_network.add_segment(
        clothoid::segment::from_line(vec2(-200, 0), vec2(1, 0), 400));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(200, 0), vec2(1, 0), math::pi<float> * 75.f * (100.f / 103.262383f), 0, 1.f/75.f / (100.f / 103.262383f)));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(377.952576f, 100), vec2(0, 1), math::pi<float> * 75.f * (100.f / 103.262383f), 1.f/75.f / (100.f / 103.262383f), 0));
    _rail_network.add_segment(
        clothoid::segment::from_line(vec2(200, 200), vec2(-1, 0), 400));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(-200, 200), vec2(-1, 0), math::pi<float> *75.f * (100.f / 103.262383f), 0, 1.f / 75.f / (100.f / 103.262383f)));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(-377.952576f, 100), vec2(0, -1), math::pi<float> *75.f * (100.f / 103.262383f), 1.f / 75.f / (100.f / 103.262383f), 0));

    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(200, 0), vec2(1, 0), math::pi<float> * 25.f, -1.f/50.f));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(250, -50), vec2(0, -1), math::pi<float> * 25.f, -1.f/50.f));
    _rail_network.add_segment(
        clothoid::segment::from_line(vec2(200, -100), vec2(-1, 0), 400));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(-200, -100), vec2(-1, 0), math::pi<float> * 25.f, -1.f/50.f));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(-250, -50), vec2(0, 1), math::pi<float> * 25.f, -1.f/50.f));

    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(-200, 200), vec2(-1, 0), math::pi<float> * 100.f, 0, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(-509.348999f, 240.672592f), vec2(-0.923879504f, 0.382683456f), math::pi<float> * 100.f, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(-725.827454f, 457.151031f), vec2(-0.382683396f, 0.923879504f), math::pi<float> * 100.f, -1.f/400.f, 0));
    _rail_network.add_segment(
        clothoid::segment::from_line(vec2{-766.5f, 766.5f}, vec2(0, 1), 400.f));

    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2(-766.5f, 1166.5f), vec2(0, 1), math::pi<float> * 100.f, 0, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2(-725.827393f, 1475.849f), vec2(0.382683456f, 0.923879504f), math::pi<float> * 200.f, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2{-203.202209f, 1692.32751f}, vec2{0.923879504f, -0.382683486f}, math::pi<float> * 100.f, -1.f/400.f, 0));

    _rail_network.add_segment(
        clothoid::segment::from_line(vec2{44.3004303f, 1502.34460f}, vec2{0.707106709f, -0.707106829f}, 400.f + 295.148468f));

    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2{535.844543f, 1010.80042f}, vec2{0.707106709f, -0.707106829f}, math::pi<float> * 100.f, 0, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_arc(vec2{725.827454f, 763.297729f}, vec2{0.382683307f, -0.923879564f}, math::pi<float> * 200.f, -1.f/400.f));
    _rail_network.add_segment(
        clothoid::segment::from_transition(vec2{509.348938f, 240.672577f}, vec2{-0.923879564f, -0.382683277f}, math::pi<float> * 100.f, -1.f/400.f, 0));

    auto A = _rail_network.add_station(vec2(-100, 200), "A");
    auto B = _rail_network.add_station(vec2(100, 0), "B");
    auto C = _rail_network.add_station(vec2(-100, -100), "C");
    auto D = _rail_network.add_station(vec2(-766.5f, 1166.5f), "D");

    spawn<train>(16)->set_schedule({B, D});
    spawn<train>(8)->set_schedule({B, C, A});
    spawn<train>(4)->set_schedule({B, A});
}

//------------------------------------------------------------------------------
void world::clear()
{
    _objects.clear();
    // assign with empty queue because std::queue has no clear method
    _removed = std::queue<handle<game::object>>{};

    _physics_objects.clear();
    _particles.clear();

    _rail_network.clear();
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

    _rail_network.draw(renderer, time);

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

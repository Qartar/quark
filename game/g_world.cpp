// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_projectile.h"
#include "g_ship.h"
#include "p_collide.h"
#include "p_trace.h"

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
world::world()
    : _spawn_id(0)
    , _border_material{0,0}
    , _border_shapes{{vec2(0,0)}, {vec2(0,0)}}
    , _arena_width("g_arenaWidth", 640, config::archive|config::server|config::reset, "arena width")
    , _arena_height("g_arenaHeight", 480, config::archive|config::server|config::reset, "arena height")
    , _physics(
        std::bind(&world::physics_filter_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&world::physics_collide_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{}

//------------------------------------------------------------------------------
void world::init()
{
    reset();
}

//------------------------------------------------------------------------------
void world::shutdown()
{
    _objects.clear();
    _pending.clear();
    _removed.clear();
}

//------------------------------------------------------------------------------
void world::reset()
{
    clear_particles();

    _mins = vec2(0,0);
    _maxs = vec2(vec2i(_arena_width, _arena_height));
    _framenum = 0;

    _border_shapes[0] = physics::box_shape(vec2(vec2i(_border_thickness + _arena_width, _border_thickness)));
    _border_shapes[1] = physics::box_shape(vec2(vec2i(_border_thickness, _border_thickness + _arena_height)));

    _objects.clear();
    _pending.clear();
    _removed.clear();

    _spawn_id = 0;

    for (int ii = 0; ii < 3; ++ii) {
        float angle = float(ii) * (math::pi<float> * 2.f / 3.f);
        vec2 dir = vec2(std::cos(angle), std::sin(angle));

        ship* sh = spawn<ship>();
        sh->set_position(vec2(vec2i(_arena_width, _arena_height)) * .5f - dir * 96.f, true);
        sh->set_rotation(angle, true);
    }
}

//------------------------------------------------------------------------------
void world::remove(game::object* object)
{
    _removed.insert(object);
}

//------------------------------------------------------------------------------
void world::draw(render::system* renderer, time_value time) const
{
    for (auto& obj : _objects) {
        obj->draw(renderer, time);
    }

    draw_particles(renderer, time);
}

//------------------------------------------------------------------------------
void world::run_frame()
{
    _message.reset();

    ++_framenum;

    for (auto obj : _removed) {
        _objects.erase(std::find_if(_objects.begin(), _objects.end(), [=](auto& it){
            return it.get() == obj;
        }));
    }
    _removed.clear();

    for (auto& obj : _objects) {
        obj->think();
    }

    for (auto& obj : _objects) {
        obj->_old_position = obj->get_position();
        obj->_old_rotation = obj->get_rotation();
    }
    _physics.step(FRAMETIME.to_seconds());

    for (auto& obj : _pending) {
        _objects.push_back(std::move(obj));
    }
    _pending.clear();
}

//------------------------------------------------------------------------------
void world::read_snapshot(network::message& message)
{
    for (auto& obj : _pending) {
        _objects.push_back(std::move(obj));
    }
    _pending.clear();

    for (auto obj : _removed) {
        _objects.erase(std::find_if(_objects.begin(), _objects.end(), [=](auto& it){
            return it.get() == obj;
        }));
    }
    _removed.clear();

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

    for (auto& obj : _pending) {
        _objects.push_back(std::move(obj));
    }
    _pending.clear();
}

//------------------------------------------------------------------------------
void world::read_frame(network::message const& message)
{
    _framenum = message.read_long();
    _mins = message.read_vector();
    _maxs = message.read_vector();

    // read active objects
    for (std::size_t ii = 0; ; ++ii) {
        std::size_t spawn_id = message.read_long();

        if (!spawn_id) {
            for (; ii < _objects.size(); ++ii) {
                remove(_objects[ii].get());
            }
            break;
        }

        auto type = static_cast<object_type>(message.read_byte());

        while (ii < _objects.size() && _objects[ii]->_spawn_id < spawn_id) {
            remove(_objects[ii++].get());
        }

        if (ii >= _objects.size()) {
            game::object* obj = spawn_snapshot(spawn_id, type);
            obj->read_snapshot(message);
            obj->set_position(obj->get_position(), true);
        } else /*if (ii < _objects.size())*/ {
            assert(_objects[ii]->_spawn_id == spawn_id);
            _objects[ii]->read_snapshot(message);
        }
    }
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
    message.write_vector(_mins);
    message.write_vector(_maxs);

    // write active objects
    for (auto const& obj : _objects) {
        message.write_long(narrow_cast<int>(obj->_spawn_id));
        message.write_byte(narrow_cast<uint8_t>(obj->_type));
        obj->write_snapshot(message);
    }
    message.write_long(0);

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
game::object* world::spawn_snapshot(std::size_t spawn_id, object_type type)
{
    switch (type) {
        case object_type::projectile: {
            auto proj = spawn<game::projectile>(nullptr, 1.0f, weapon_type::cannon);
            proj->_spawn_id = _spawn_id = spawn_id;
            return proj;
        }

        case object_type::obstacle: {
            auto obj = spawn<game::object>(object_type::object);
            obj->_spawn_id = _spawn_id = spawn_id;
            return obj;
        }

        case object_type::object:
        default:
            return nullptr;
    }
}

//------------------------------------------------------------------------------
game::object* world::find_object(std::size_t spawn_id) const
{
    // search in active objects
    for (auto& obj : _objects) {
        if (obj->_spawn_id == spawn_id) {
            return obj.get();
        }
    }

    // look in pending objects
    for (auto& obj : _pending) {
        if (obj->_spawn_id == spawn_id) {
            return obj.get();
        }
    }

    return nullptr;
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
    for (auto& other : _objects) {
        if (other.get() == ignore || other.get()->_owner == ignore) {
            continue;
        }

        auto tr = physics::trace(&other->rigid_body(), start, end);
        if (tr.get_fraction() < 1.0f) {
            candidates.insert(candidate{
                tr.get_fraction(),
                tr.get_contact(),
                other.get()}
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

    if (obj_b->_type == object_type::projectile) {
        return false;
    }

    game::object const* owner_a = obj_a->_owner ? obj_a->_owner : obj_a;
    game::object const* owner_b = obj_b->_owner ? obj_b->_owner : obj_b;

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

// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "p_collide.h"
#include "p_trace.h"

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
void world::init()
{
    char const* command;

    clear_particles();

    if ( (command = strstr( g_Application->init_string(), "particles=" )) ) {
        _use_particles = ( atoi(command+10) > 0 );
    } else {
        _use_particles = true;
    }

    _border_material = physics::material(0, 0);

    reset();
}

//------------------------------------------------------------------------------
void world::shutdown()
{
}

//------------------------------------------------------------------------------
void world::reset()
{
    _mins = vec2(0,0);
    _maxs = vec2(_arena_width, _arena_height);
    _framenum = 0;

    _border_shapes[0] = physics::box_shape(vec2(_border_thickness + _arena_width, _border_thickness));
    _border_shapes[1] = physics::box_shape(vec2(_border_thickness, _border_thickness + _arena_height));

    _objects.clear();
    _pending.clear();
    _removed.clear();

    for (int ii = 0; ii < MAX_PLAYERS; ++ii) {
        _players[ii] = nullptr;
    }

    _spawn_id = 0;

    // Initialize border objects
    {
        vec2 mins = vec2(-_border_thickness / 2, -_border_thickness / 2);
        vec2 maxs = vec2(_arena_width, _arena_height) - mins;

        vec2 positions[] = {
            {(mins.x+maxs.x)/2,mins.y}, // bottom
            {(mins.x+maxs.x)/2,maxs.y}, // top
            {mins.x,(mins.y+maxs.y)/2}, // left
            {maxs.x,(mins.y+maxs.y)/2}, // right
        };

        physics::shape* shapes[] = {
            &_border_shapes[0],
            &_border_shapes[0],
            &_border_shapes[1],
            &_border_shapes[1],
        };

        for (int ii = 0; ii < 4; ++ii) {
            physics::rigid_body body(shapes[ii], &_border_material, 0.0f);
            body.set_position(positions[ii]);
            spawn<obstacle>(std::move(body));
        }
    }
}

//------------------------------------------------------------------------------
void world::remove(game::object* object)
{
    _removed.insert(object);
}

//------------------------------------------------------------------------------
void world::draw(render::system* renderer, float time) const
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

    for (auto& obj : _objects) {
        obj->think();
    }

    for (auto& obj : _objects) {
        move_object(obj.get());
    }

    for (auto obj : _removed) {
        _objects.erase(std::find_if(_objects.begin(), _objects.end(), [=](auto& it){
            return it.get() == obj;
        }));
    }
    _removed.clear();

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

    for (auto obj : _removed) {
        _objects.erase(std::find_if(_objects.begin(), _objects.end(), [=](auto& it){
            return it.get() == obj;
        }));
    }
    _removed.clear();

    for (auto& obj : _pending) {
        _objects.push_back(std::move(obj));
    }
    _pending.clear();
}

//------------------------------------------------------------------------------
void world::read_frame(network::message const& message)
{
    network::delta_message delta_message(nullptr, &message, nullptr);

    _framenum = delta_message.read_long();

    // read active objects
    for (std::size_t ii = 0; ; ++ii) {
        std::size_t spawn_id = delta_message.read_long();

        if (!spawn_id) {
            for (; ii < _objects.size(); ++ii) {
                remove(_objects[ii].get());
            }
            break;
        }

        auto type = static_cast<object_type>(delta_message.read_byte());

        while (ii < _objects.size() && _objects[ii]->_spawn_id < spawn_id) {
            remove(_objects[ii++].get());
        }

        if (ii >= _objects.size()) {
            game::object* obj = spawn_snapshot(spawn_id, type);
            obj->read_snapshot(delta_message);
            obj->set_position(obj->get_position(), true);
        } else /*if (ii < _objects.size())*/ {
            assert(_objects[ii]->_spawn_id == spawn_id);
            _objects[ii]->read_snapshot(delta_message);
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
    int type = message.read_byte();
    vec2 pos = message.read_vector();
    vec2 vel = message.read_vector();
    float strength = message.read_float();

    add_effect(static_cast<game::effect_type>(type), pos, vel, strength);
}

//------------------------------------------------------------------------------
void world::write_snapshot(network::message& message) const
{
    message.write_byte(svc_snapshot);

    // write frame
    message.write_byte(static_cast<int>(message_type::frame));
    network::delta_message delta_message(nullptr, &message, nullptr);
    delta_message.write_long(_framenum);

    // write active objects
    for (auto const& obj : _objects) {
        delta_message.write_long(obj->_spawn_id);
        delta_message.write_byte(static_cast<int>(obj->_type));
        obj->write_snapshot(delta_message);
    }
    delta_message.write_long(0);

    // write sounds and effects
    message.write(_message);
    message.write_byte(static_cast<int>(message_type::none));

    _message.rewind();
}

//------------------------------------------------------------------------------
void world::write_sound(sound::asset sound_asset, vec2 position, float volume)
{
    _message.write_byte(static_cast<int>(message_type::sound));
    _message.write_long(static_cast<int>(sound_asset));
    _message.write_vector(position);
    _message.write_float(volume);
}

//------------------------------------------------------------------------------
void world::write_effect(effect_type type, vec2 position, vec2 direction, float strength)
{
    _message.write_byte(static_cast<int>(message_type::effect));
    _message.write_byte(static_cast<int>(type));
    _message.write_vector(position);
    _message.write_vector(direction);
    _message.write_float(strength);
}

//------------------------------------------------------------------------------
game::object* world::spawn_snapshot(std::size_t spawn_id, object_type type)
{
    switch (type) {
        case object_type::tank: {
            auto tank = spawn<game::tank>();
            tank->_spawn_id = _spawn_id = spawn_id;
            return tank;
        }

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
game::tank* world::spawn_player(int player_index)
{
    assert(_players[player_index] == nullptr);
    _players[player_index] = spawn<game::tank>();
    _players[player_index]->_player_index = player_index;
    return _players[player_index];
}

//------------------------------------------------------------------------------
void world::remove_player(int player_index)
{
    assert(_players[player_index]);
    remove(_players[player_index]);
    _players[player_index] = nullptr;
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
void world::move_object(game::object *object)
{
    object->_old_position = object->get_position();
    object->_old_rotation = object->get_rotation();

    if (object->get_linear_velocity().length_sqr() < 1e-12f
            && std::abs(object->get_angular_velocity()) < 1e-6f) {
        return;
    }

    if (object->_type == object_type::projectile) {
        game::object* best_object = NULL;
        float best_fraction = 1.f;
        physics::contact contact;

        vec2 start = object->get_position();
        vec2 end = start + object->get_linear_velocity() * FRAMETIME;

        for (auto& other : _objects) {
            if (other.get() == object) {
                continue;
            }

            if (object->_owner == other.get()) {
                continue;
            }

            auto tr = physics::trace(&other->rigid_body(), start, end);

            if (tr.get_fraction() < best_fraction) {
                best_fraction = tr.get_fraction();
                best_object = other.get();
                contact = tr.get_contact();
            }
        }

        if (best_object) {
            object->set_position(start + (end - start) * best_fraction);
            object->touch(best_object, &contact);
        } else {
            object->set_position(end);
        }
    } else {
        for (auto& other : _objects) {
            if (other.get() == object) {
                continue;
            }

            if (other->_owner == object) {
                continue;
            }

            auto c = physics::collide(&object->rigid_body(), &other->rigid_body());

            if (c.has_contact()) {
                object->apply_impulse(
                    -c.get_contact().impulse,
                    c.get_contact().point
                );

                other->apply_impulse(
                    c.get_contact().impulse,
                    c.get_contact().point
                );

                object->touch(other.get(), &c.get_contact());
            }
        }

        object->set_position(object->get_position() + object->get_linear_velocity() * FRAMETIME);
        object->set_rotation(object->get_rotation() + object->get_angular_velocity() * FRAMETIME);
    }
}

//------------------------------------------------------------------------------
void world::add_sound(sound::asset sound_asset, vec2 position, float volume)
{
    write_sound(sound_asset, position, volume);
    pSound->play(sound_asset, vec3(position), volume, 1.0f);
}

//------------------------------------------------------------------------------
void world::add_effect(effect_type type, vec2 position, vec2 direction, float strength)
{
    write_effect(type, position, direction, strength);

    float   r, d;

    switch (type) {
        case effect_type::smoke: {
            int count = strength;
            render::particle* p;

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand();
                p->position = position + vec2(cos(r)*d,sin(r)*d);
                p->velocity = direction * (0.25 + frand()*0.75) + vec2(crand()*24,crand()*24);

                p->size = 2.0f + frand()*4.0f;
                p->size_velocity = 2.0 + frand()*2.0f;

                p->color = color4(0.5,0.5,0.5,0.1+frand()*0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a / (1+frand()*1.0f));

                p->drag = 2.5f + frand() * 1.5f;
            }
            break;
        }

        case effect_type::sparks: {
            render::particle* p;

            for (int ii = 0; ii < 4; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                p->position = position + vec2(crand()*2,crand()*2);

                r = frand()*M_PI*2.0f;
                d = frand()*128;

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

                p->color = color4(1,0.5+frand()*0.5,0,strength*(0.5f+frand()));
                p->color_velocity = color4(0,-1.0f,0,-2.0f - frand());
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = 0.5f + frand() * 0.5f;
                p->flags = render::particle::tail;
            }

            for (int ii = 0; ii < 2; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*24;

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = frand()*M_PI*2.0f;
                d = frand()*24;

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

                p->size = 4.0f + frand()*8.0f;
                p->size_velocity = 2.0;

                p->color = color4(0.5,0.5,0.5,0.1+frand()*0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a / (2+frand()*1.5f));

                p->drag = 0.5f + frand() * 2.0f;
            }
            break;
        }

        case effect_type::cannon_impact:
        case effect_type::missile_impact:
        case effect_type::explosion: {
            render::particle* p;
            float scale = std::sqrt(strength);

            // shock wave

            if ( (p = add_particle()) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1.0f,1.0f,0.5f,0.5f);
            p->color_velocity = -p->color * color4(0,1,3,3);
            p->size = 12.0 * scale;
            p->size_velocity = 192.0f * scale;
            p->flags = render::particle::invert;

            // smoke

            for (int ii = 0; ii < 96 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*12 * scale;

                p->position = position + vec2(cos(r),sin(r))*d;

                r = frand()*M_PI*2.0f;
                d = sqrt(frand()) * 128.0f * strength;

                p->velocity = vec2(cos(r),sin(r)) * d;
                p->velocity += direction * d * 0.5f;

                p->size = (4.0f + frand()*8.0f) * scale;
                p->size_velocity = 2.0 * strength;

                p->color = color4(0.5,0.5,0.5,0.1+frand()*0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a / (2+frand()*1.5f));

                p->drag = (3.0f + frand() * 1.0f) * scale;
            }

            // fire

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*8 * scale;

                p->position = position + vec2(cos(r),sin(r))*d;

                r = frand()*M_PI*2.0f;
                d = sqrt(frand()) * 128.0f * strength;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,frand(),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.5+frand()*frand()*2.5f));
                p->size = (8.0 + frand()*16.0f) * scale;
                p->size_velocity = 1.0f * strength;

                p->drag = (2.0f + frand() * 2.0f) * scale;
            }

            // debris

            for (int ii = 0; ii < 32 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*2 * scale;

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = frand()*M_PI*2.0f;
                d = frand()*128 * scale;

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

                p->color = color4(1,0.5+frand()*0.5,0,1);
                p->color_velocity = color4(0,0,0,-1.5f-frand());
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = 0.5f + frand() * 0.5f;
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::blaster: {
            render::particle* p;

            // vortex

            if ( (p = add_particle()) == NULL )
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
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*4;

                p->position = position + vec2(cos(r),sin(r))*d;

                r = frand()*M_PI*2.0f;
                d = sqrt(frand()) * 32.0f;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,0.5f*frand(),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.25f+frand()*frand()));
                p->size = 2.0 + frand()*4.0f;
                p->size_velocity = 0.5f;
                p->flags = render::particle::invert;

                p->drag = 3.0f + frand() * 3.0f;

                // vortex

                render::particle* p2;

                if ( (p2 = add_particle()) == NULL )
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
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*0.5f;

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = frand()*M_PI*2.0f;
                d = frand()*64.0f;

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * frand() * 96.0f;

                r = frand()*M_PI*2.0f;
                d = 64.0f + frand()*64.0f;

                p->acceleration = vec2(cos(r), sin(r))*d;

                p->color = color4(1,frand()*0.5,0,1);
                p->color_velocity = color4(0,0,0,-1.5f-2.0f*frand());
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = 2.0f + frand() * 2.0f;
                p->flags = render::particle::tail;
            }
            break;
        }

        case effect_type::blaster_impact: {
            render::particle* p;
            float scale = std::sqrt(strength);

            // vortex

            if ( (p = add_particle()) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1,0,0,0);
            p->color_velocity = color4(0,1,0,2.5f);
            p->size = 96.0f * scale;
            p->size_velocity = -480.0f * scale;
            p->flags = render::particle::invert;

            // shock wave

            if ( (p = add_particle()) == NULL )
                return;

            p->position = position;
            p->velocity = direction * 48.0f * scale;

            p->color = color4(1.0f,0.25f,0.0f,0.5f);
            p->color_velocity = -p->color * color4(0,1,3,3);
            p->size = 12.0 * scale;
            p->size_velocity = 192.0f * scale;
            p->flags = render::particle::invert;

            // smoke

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*12 * scale;

                p->position = position + vec2(cos(r),sin(r))*d;

                r = frand()*M_PI*2.0f;
                d = sqrt(frand()) * 128.0f * strength;

                p->velocity = vec2(cos(r),sin(r)) * d;
                p->velocity += direction * d * 0.5f;

                p->size = (4.0f + frand()*8.0f) * scale;
                p->size_velocity = 2.0 * strength;

                p->color = color4(0.5,0.5,0.5,0.1+frand()*0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a / (2+frand()*1.5f));

                p->drag = (3.0f + frand() * 1.0f) * scale;
            }

            // fire

            for (int ii = 0; ii < 64 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*16 * scale;

                p->position = position + vec2(cos(r),sin(r))*d;

                r = frand()*M_PI*2.0f;
                d = sqrt(frand()) * 128.0f * strength;

                p->velocity = vec2(cos(r),sin(r))*d;
                p->velocity += direction * d * 0.5f;

                p->color = color4(1.0f,0.5f*frand(),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.5+frand()*frand()*1.5f));
                p->size = (8.0 + frand()*16.0f) * scale;
                p->size_velocity = 1.0f * strength;

                p->drag = (2.0f + frand() * 2.0f) * scale;
            }

            // debris

            for (int ii = 0; ii < 32 * scale; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand()*2 * scale;

                p->position = position + vec2(cos(r)*d,sin(r)*d);

                r = frand()*M_PI*2.0f;
                d = frand()*128 * scale;

                p->velocity = vec2(cos(r)*d,sin(r)*d);
                p->velocity += direction * d * 0.5f;

                p->color = color4(1,frand()*0.5,0,1);
                p->color_velocity = color4(0,0,0,-1.5f-frand());
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = 0.5f + frand() * 0.5f;
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
            int count = strength;
            render::particle* p;

            // smoke

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand();
                p->position = position + vec2(cos(r)*d,sin(r)*d) + lerp * ii / count;
                p->velocity = direction * (0.25 + frand()*0.75) + vec2(crand()*24,crand()*24);

                p->size = 2.0f + frand()*4.0f;
                p->size_velocity = 2.0 + frand()*2.0f;

                p->color = color4(0.5,0.5,0.5,0.1+frand()*0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a / (1+frand()*1.0f));

                p->drag = 2.5f + frand() * 1.5f;
                p->time += FRAMETIME * ii / count;
            }

            // fire

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand();
                p->position = position + vec2(cos(r)*d,sin(r)*d) + lerp * ii / count;
                p->velocity = direction * (0.25 + frand()*0.75) + vec2(crand()*24,crand()*24);

                p->size = 4.0f + frand()*2.0f;
                p->size_velocity = 4.0 + frand()*4.0f;

                p->color = color4(1.0f,0.5f+0.5f*frand(),0.0f,0.1f);
                p->color_velocity = color4(0,0,0,-p->color.a/(0.15+0.15f*frand()));

                p->drag = (1.5f + frand() * 1.0f);
                p->time += FRAMETIME * ii / count;
            }

            // debris

            for (int ii = 0; ii < count; ++ii) {
                if ( (p = add_particle()) == NULL )
                    return;

                r = frand()*M_PI*2.0f;
                d = frand();

                p->position = position + vec2(cos(r),sin(r))*d + lerp * ii / count;

                r = frand()*M_PI*2.0f;
                d = frand() * 64.0f;

                p->velocity = direction * (0.25 + frand()*0.75) + vec2(crand()*48,crand()*48);

                p->color = color4(1,0.5+frand()*0.5,0,1);
                p->color_velocity = color4(0,0,0,-3.0f-15.0f*(1.0f-square(frand())));
                p->size = 0.5f;
                p->size_velocity = 0.0f;
                p->drag = 1.5f + frand() * 1.5f;
                p->time += FRAMETIME * ii / count;
                p->flags = render::particle::tail;
            }

            break;
        }
        default:
            break;
    }
}

} // namespace game

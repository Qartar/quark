// g_projectile.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_projectile.h"
#include "g_ship.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::circle_shape projectile::_shape(1.0f);
physics::material projectile::_material(0.5f, 1.0f);

//------------------------------------------------------------------------------
projectile::projectile(object* owner, float damage, weapon_type type)
    : object(object_type::projectile, owner)
    , _damage(damage)
    , _type(type)
    , _impact_time(time_value::max)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1e-3f);
    _channel = pSound->allocate_channel();
    _sound_cannon_impact = pSound->load_sound("assets/sound/cannon_impact.wav");
    _sound_blaster_impact = pSound->load_sound("assets/sound/blaster_impact.wav");
    _sound_missile_flight = pSound->load_sound("assets/sound/missile_flight.wav");
}

//------------------------------------------------------------------------------
projectile::~projectile()
{
    if (_channel) {
        _channel->stop();
        pSound->free_channel(_channel);
    }
}

//------------------------------------------------------------------------------
void projectile::think()
{
    if (_world->frametime() - _spawn_time > fuse_time) {
        _world->remove(this);
        return;
    }

    if (_type == weapon_type::missile) {
        update_homing();
    }

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void projectile::update_homing()
{
    vec2 direction = get_linear_velocity();
    float speed = direction.normalize_length();
    float bestValue = .5f * math::sqrt2<float>;
    game::object* bestTarget = nullptr;

    for (auto& obj : _world->objects()) {
        if (obj->_type != object_type::ship) {
            continue;
        }

        vec2 displacement = obj->get_position() - get_position();
        float value = displacement.normalize().dot(direction);
        if (value > bestValue) {
            bestValue = value;
            bestTarget = obj.get();
        }
    }

    if (bestTarget && bestValue < 0.995f) {
        vec2 target_pos = bestTarget->get_position();
        vec2 closest_point = get_position() + direction * (target_pos - get_position()).dot(direction);
        vec2 displacement = (target_pos - closest_point).normalize();
        vec2 delta = displacement * speed * 0.5f * FRAMETIME.to_seconds();
        vec2 new_velocity = (get_linear_velocity() + delta).normalize() * speed;
        set_linear_velocity(new_velocity);
    }
}

//------------------------------------------------------------------------------
void projectile::update_effects()
{
    time_value time = _world->frametime();

    if (time > _impact_time) {
        return;
    }

    float a = min(1.f, (_spawn_time + fuse_time - _world->frametime()) / fade_time);
    vec2 p1 = get_position() - get_linear_velocity()
        * (std::min(FRAMETIME, time - _spawn_time) / time_delta::from_seconds(1));
    vec2 p2 = get_position();

    if (_type == weapon_type::missile) {
        _world->add_trail_effect(
            effect_type::missile_trail,
            p2,
            p1,
            get_linear_velocity() * -0.5f,
            4.f * a );
    }
}

//------------------------------------------------------------------------------
void projectile::update_sound()
{
    if (_type == weapon_type::missile) {
        if (_channel) {
            if (!_channel->playing()) {
                _channel->loop(_sound_missile_flight);
            }
            _channel->set_volume(0.1f);
            _channel->set_attenuation(0.0f);
            _channel->set_origin(vec3(get_position()));
        }
    } else if (_channel && _channel->playing()) {
        _channel->stop();
    }
}

//------------------------------------------------------------------------------
bool projectile::touch(object *other, physics::collision const* collision)
{
    auto sound = _type == weapon_type::cannon ? _sound_cannon_impact :
                 _type == weapon_type::missile ? _sound_cannon_impact :
                 _type == weapon_type::blaster ? _sound_blaster_impact : sound::asset::invalid;

    auto effect = _type == weapon_type::cannon ? effect_type::cannon_impact :
                  _type == weapon_type::missile ? effect_type::missile_impact :
                  _type == weapon_type::blaster ? effect_type::blaster_impact : effect_type::smoke;

    if (other && other->_type != object_type::projectile && !other->touch(this, collision)) {
        return false;
    }

    // calculate impact time
    {
        vec2 displacement = (collision->point - get_position());
        vec2 relative_velocity = get_linear_velocity();
        if (other) {
            relative_velocity -= other->get_linear_velocity();
        }
        float delta_time = displacement.dot(relative_velocity) / relative_velocity.length_sqr();
        _impact_time = _world->frametime() + time_delta::from_seconds(1) * delta_time;
    }

    float factor = (other && other->_type == object_type::shield) ? .5f : 1.f;

    if (collision) {
        _world->add_sound(sound, collision->point, factor * _damage);
        _world->add_effect(_impact_time, effect, collision->point, -collision->normal, .5f * factor * _damage);
    } else {
        _world->add_sound(sound, get_position(), factor * _damage);
        _world->add_effect(_impact_time, effect, get_position(), vec2_zero, .5f * factor * _damage);
    }

    if (other && other->_type == object_type::ship) {
        static_cast<ship*>(other)->damage(this, collision ? collision->point : get_position(), _damage);
    }

    _world->remove(this);
    return true;
}

//------------------------------------------------------------------------------
void projectile::draw(render::system* renderer, time_value time) const
{
    constexpr time_delta tail_time = time_delta::from_seconds(.02f);

    if (time - tail_time > _impact_time) {
        return;
    }

    float a = min(1.f, (_spawn_time + fuse_time - time) / fade_time);
    vec2 p1 = get_position(std::max(_spawn_time, time - tail_time));
    vec2 p2 = get_position(std::min(_impact_time, time));

    switch (_type) {
        case weapon_type::cannon:
            renderer->draw_line(p2, p1, color4(1,0.5,0,a), color4(1,0.5,0,0));
            break;

        case weapon_type::missile:
            renderer->draw_line(p2, p1, color4(1,1,1,a), color4(0,0,0,0));
            break;

        case weapon_type::blaster:
            renderer->draw_line(p2, p1, color4(1,0.1f,0,a), color4(1,0.7f,0,0));
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------
void projectile::read_snapshot(network::message const& message)
{
    _old_position = get_position();

    _owner = _world->find_object(message.read_long());
    _damage = message.read_float();
    _type = static_cast<weapon_type>(message.read_byte());
    set_position(message.read_vector());
    set_linear_velocity(message.read_vector());

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void projectile::write_snapshot(network::message& message) const
{
    message.write_long(narrow_cast<int>(_owner->spawn_id() & 0xffffffff));
    message.write_float(_damage);
    message.write_byte(narrow_cast<uint8_t>(_type));
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
}

} // namespace game

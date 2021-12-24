// g_projectile.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_projectile.h"
#include "g_ship.h"
#include "g_shield.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type projectile::_type(object::_type);
physics::circle_shape projectile::_shape(1.0f);
physics::material projectile::_material(0.5f, 1.0f);

//------------------------------------------------------------------------------
projectile::projectile(object* owner, projectile_info info)
    : object(owner)
    , _info(info)
    , _impact_time(time_value::max)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1e-3f);
    _channel = pSound->allocate_channel();
}

//------------------------------------------------------------------------------
projectile::~projectile()
{
    get_world()->remove_body(&_rigid_body);

    if (_channel) {
        _channel->stop();
        pSound->free_channel(_channel);
    }
}

//------------------------------------------------------------------------------
void projectile::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);
}

//------------------------------------------------------------------------------
void projectile::think()
{
    if (get_world()->frametime() - _spawn_time > _info.fuse_time) {
        get_world()->remove(this);
        return;
    }

    if (_info.homing) {
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

    for (auto* obj : get_world()->objects()) {
        if (!obj->is_type<ship>()) {
            continue;
        }

        vec2 displacement = obj->get_position() - get_position();
        float value = displacement.normalize().dot(direction);
        if (value > bestValue) {
            bestValue = value;
            bestTarget = obj;
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
    time_value time = get_world()->frametime();

    if (time > _impact_time) {
        return;
    }

    float a = min(1.f, (_spawn_time + _info.fuse_time - get_world()->frametime()) / _info.fade_time);
    vec2 p1 = get_position() - get_linear_velocity()
        * (std::min(FRAMETIME, time - _spawn_time) / time_delta::from_seconds(1));
    vec2 p2 = get_position();

    if (_info.flight_effect != effect_type::none) {
        get_world()->add_trail_effect(
            _info.flight_effect,
            p2,
            p1,
            get_linear_velocity() * -0.5f,
            4.f * a );
    }
}

//------------------------------------------------------------------------------
void projectile::update_sound()
{
    if (_info.flight_sound != sound::asset::invalid) {
        if (_channel) {
            if (!_channel->playing()) {
                _channel->loop(_info.flight_sound);
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
    if (other && !other->is_type<projectile>() && !other->touch(this, collision)) {
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
        _impact_time = get_world()->frametime() + time_delta::from_seconds(1) * delta_time;
    }

    float factor = (other && other->is_type<shield>()) ? .5f : 1.f;

    if (collision) {
        get_world()->add_sound(_info.impact_sound, collision->point, factor * _info.damage);
        get_world()->add_effect(_impact_time, _info.impact_effect, collision->point, -collision->normal, .5f * factor * _info.damage);
    } else {
        get_world()->add_sound(_info.impact_sound, get_position(), factor * _info.damage);
        get_world()->add_effect(_impact_time, _info.impact_effect, get_position(), vec2_zero, .5f * factor * _info.damage);
    }

    if (other && other->is_type<ship>()) {
        static_cast<ship*>(other)->damage(this, collision ? collision->point : get_position(), _info.damage);
    }

    get_world()->remove(this);
    return true;
}

//------------------------------------------------------------------------------
void projectile::draw(render::system* renderer, time_value time) const
{
    constexpr time_delta tail_time = time_delta::from_seconds(.04f);

    if (time - _info.tail_time > _impact_time) {
        return;
    }

    float a = min(1.f, (_spawn_time + _info.fuse_time - time) / _info.fade_time);
    vec2 p1 = get_position(std::max(_spawn_time, time - _info.tail_time));
    vec2 p2 = get_position(std::min(_impact_time, time));

    renderer->draw_line(1.f, p2, p1,
        _info.color * color4(1,1,1,a),
        _info.color * color4(1,1,1,0),
        _info.color * color4(1,1,1,a),
        _info.color * color4(1,1,1,0));
}

//------------------------------------------------------------------------------
void projectile::read_snapshot(network::delta_message const& message)
{
    _old_position = get_position();

    _owner = get_world()->find<object>(message.read_long());
    set_position(message.read_vector());
    set_linear_velocity(message.read_vector());

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void projectile::write_snapshot(network::delta_message& message) const
{
    message.write_long(narrow_cast<int>(_owner->get_sequence() & 0xffffffff));
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
}

} // namespace game

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
projectile::projectile(object* owner, projectile_info info, handle<game::object> target)
    : object(owner)
    , _info(info)
    , _target(target)
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
    time_delta delta_time = get_world()->frametime() - _spawn_time;

    if (delta_time > _info.fuse_time) {
        get_world()->remove(this);
        return;
    }

    if (_info.homing && delta_time > _info.delay_time) {
        update_homing();
    }

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void projectile::update_homing()
{
    if (_target) {
        vec2 relative_position = _target->get_position() - get_position();
        vec2 relative_velocity = _target->get_linear_velocity() - get_linear_velocity();

        // no homing if we missed the target
        if (dot(relative_position, relative_velocity) > 0.f) {
            return;
        }

        float delta_time = -dot(relative_position, relative_velocity) / dot(relative_velocity, relative_velocity);
        vec2 target_pos = _target->get_position() + _target->get_linear_velocity() * delta_time;
        vec2 target_dir = normalize(target_pos - get_position());

        // Project linear velocity onto target direction to determine course correction.
        vec2 correction = relative_velocity - target_dir * dot(relative_velocity, target_dir);
        float deltav = _info.acceleration * FRAMETIME.to_seconds();
        if (dot(correction, correction) < square(deltav)) {
            // If correction is less than max acceleration use the remaining
            // delta-v to accelerate towards the intercept point. This will
            // slightly invalidate the intercept point, which will be corrected
            // in the next update.
            float accel = sqrt(square(deltav) - dot(correction, correction));
            set_linear_velocity(get_linear_velocity() + correction + target_dir * accel);
        } else {
            // Otherwise apply maximum delta-v towards the course correction.
            set_linear_velocity(get_linear_velocity() + normalize(correction) * deltav);
        }
    }
}

//------------------------------------------------------------------------------
void projectile::update_effects()
{
    if (get_world()->frametime() > _impact_time) {
        return;
    }

    time_delta delta_time = get_world()->frametime() - _spawn_time;
    float a = min(1.f, (_info.fuse_time - delta_time) / _info.fade_time);
    vec2 p1 = get_position() - get_linear_velocity()
        * (std::min(FRAMETIME, delta_time) / time_delta::from_seconds(1));
    vec2 p2 = get_position();

    if (_info.flight_effect != effect_type::none && delta_time > _info.delay_time) {
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
    bool is_target = other && (other == _target || (other->owner() && other->owner() == _target));
    if (!is_target) {
        return false;
    }

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
void projectile::read_snapshot(network::message const& message)
{
    _old_position = get_position();

    _owner = get_world()->find<object>(message.read_long());
    set_position(message.read_vector());
    set_linear_velocity(message.read_vector());

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void projectile::write_snapshot(network::message& message) const
{
    message.write_long(narrow_cast<int>(_owner->get_sequence() & 0xffffffff));
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
}

} // namespace game

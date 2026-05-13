// g_projectile.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ballistics.h"
#include "g_projectile.h"
#include "g_ship.h"
#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type projectile::_type(object::_type);
physics::circle_shape projectile::_shape(1.0f);
physics::material projectile::_material(0.5f, 1.0f);

//------------------------------------------------------------------------------
projectile::projectile(object* owner, projectile_info info, vec3 position, vec3 velocity)
    : object(owner)
    , _info(info)
    , _position(position)
    , _velocity(velocity)
    , _impact_time(time_value::max)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1e-3);
    set_position(position.to_vec2(), true);
    set_linear_velocity(velocity.to_vec2());
    vec2 direction = normalize(velocity.to_vec2());
    set_rotation(rot2(direction.x, direction.y), true);
    _channel = pSound->allocate_channel();

    vec2 p[5] = {
        vec2( _info.diameter, 0),
        vec2( 0, 0.5 * _info.diameter),
        vec2(-2.0 * _info.diameter, 0.5 * _info.diameter),
        vec2(-2.0 * _info.diameter,-0.5 * _info.diameter),
        vec2( 0,-0.5 * _info.diameter),
    };

    _outline = render::outline(p, countof(p));
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
void projectile::spawn()
{
    object::spawn();
}

//------------------------------------------------------------------------------
void projectile::think()
{
    vec3 new_position = _position;
    vec3 new_velocity = _velocity;

    ballistics::step(new_position, new_velocity, ballistics::curve::G1, _info.ballistic_coefficient, FRAMETIME);

    // Assume projectiles never hit anything on their way up
    if (new_velocity.z < 0.0 && new_position.z < 12.0) {
        physics::contact c;
        game::object* obj = get_world()->trace(c, _position.to_vec2(), new_position.to_vec2());

        if (obj) {
            physics::collision collision(c);
            touch(obj, &collision);
        }
    }

    // rigid body position and velocity is used for rendering
    set_position(new_position.to_vec2());
    set_linear_velocity((new_position - _position).to_vec2() / FRAMETIME.to_seconds());

    _position = new_position;
    _velocity = new_velocity;

    // Use impact time as a proxy for whether we've already hit something this frame
    if (_position.z < 0.0 && _impact_time == time_value::max) {
        // intersect with z=0 plane
        double t = _position.z / _velocity.z;
        vec3 p = _position - _velocity * t;

        _impact_time = get_world()->frametime() + (FRAMETIME - time_delta::from_seconds(t));

        get_world()->add_effect(_impact_time, effect_type::splash, globe::planar_to_surface(p.to_vec2()), vec3_zero, std::cbrt(_info.damage));
        get_world()->remove(this);
    }
}

//------------------------------------------------------------------------------
bool projectile::touch(object *other, physics::collision const* collision)
{
    if (other && !other->is_type<projectile>() && !other->touch(this, collision)) {
        return false;
    }

    // TODO: need to intersect with ship components in 3d space
    // ...

    // calculate impact time
    {
        vec2 displacement = (collision->point - get_position());
        vec2 relative_velocity = get_linear_velocity();
        if (other) {
            relative_velocity -= other->get_linear_velocity();
        }
        double delta_time = displacement.dot(relative_velocity) / relative_velocity.length_sqr();
        _impact_time = get_world()->frametime() + time_delta::from_seconds(1) * delta_time;
    }

    if (collision) {
        vec3 collision_point = globe::planar_to_surface(collision->point);
        get_world()->add_sound(_info.impact_sound, collision->point, _info.damage);
        get_world()->add_effect(
            _impact_time,
            _info.impact_effect,
            collision_point,
            vec3(collision->normal) * globe::surface_projection(collision_point).submatrix<3,3>(),
            std::cbrt(_info.damage));
    } else {
        get_world()->add_sound(_info.impact_sound, get_position(), _info.damage);
        get_world()->add_effect(_impact_time, _info.impact_effect, globe::planar_to_surface(get_position()), vec3_zero, std::cbrt(_info.damage));
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
    constexpr color4 color(1,1,1,1);

    if (time > _impact_time) {
        return;
    }

    mat3 tx = get_transform(std::min(_impact_time, time));

    vec3 origin = globe::planar_to_surface(get_position(time));
    mat4 proj = globe::surface_projection(origin);

    mat4 tx4 = mat4(tx[0][0], tx[0][1], 0, 0,
                    tx[1][0], tx[1][1], 0, 0,
                    0, 0, 1, 0,
                    0, 0, 0, 1) * proj;

    renderer->draw_outline(_outline, tx4, color);
}

//------------------------------------------------------------------------------
void projectile::read_snapshot(network::message const& message)
{
    _old_position = get_position();

    _owner = get_world()->find<object>(message.read_long());
    set_position(message.read_vector());
    set_linear_velocity(message.read_vector());
}

//------------------------------------------------------------------------------
void projectile::write_snapshot(network::message& message) const
{
    message.write_long(narrow_cast<int>(_owner->get_sequence() & 0xffffffff));
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
}

} // namespace game

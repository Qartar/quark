// g_tank.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "p_collide.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

physics::material tank::_material(0.5f, 1.0f, 5.0f);
physics::box_shape tank::_shape(vec2(24, 16));

#define DAMAGE_FRONT    0.334f
#define DAMAGE_SIDE     0.5f
#define DAMAGE_REAR     1.0f
#define DAMAGE_FULL     1.0f

//------------------------------------------------------------------------------
tank::tank()
    : object(object_type::tank)
    , _turret_model(nullptr)
    , _turret_rotation(0)
    , _turret_velocity(0)
    , _old_turret_rotation(0)
    , _track_speed(0)
    , _damage(0)
    , _player_index(0)
    , _dead_time(0)
    , _fire_time(0)
    , _usercmd{}
    , _channels{0}
    , _client(nullptr)
    , _weapon(weapon_type::cannon)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.0f);

    _model = &tank_body_model;
    _turret_model = &tank_turret_model;

    for (auto& chan : _channels) {
        chan = pSound->allocate_channel();
    }

    _sound_idle = pSound->load_sound("assets/sound/tank_idle.wav");
    _sound_move = pSound->load_sound("assets/sound/tank_move.wav");
    _sound_turret_move = pSound->load_sound("assets/sound/turret_move.wav");
    _sound_explode = pSound->load_sound("assets/sound/tank_explode.wav");
    _sound_blaster_fire = pSound->load_sound("assets/sound/blaster_fire.wav");
    _sound_cannon_fire = pSound->load_sound("assets/sound/cannon_fire.wav");
}

//------------------------------------------------------------------------------
tank::~tank()
{
    for (auto& chan : _channels) {
        if (chan) {
            chan->stop();
            pSound->free_channel(chan);
        }
    }
}

//------------------------------------------------------------------------------
void tank::draw(render::system* renderer, float time) const
{
    float lerp = (time - _world->framenum() * FRAMETIME) / FRAMETIME;

    float denominator = _weapon == weapon_type::cannon ? cannon_reload :
                        _weapon == weapon_type::missile ? missile_reload :
                        _weapon == weapon_type::blaster ? blaster_reload : 1.0f;

    float reload = 20.0f * clamp((time - _fire_time) * _client->refire_mod / denominator, 0.0f, 1.0f);
    float health = 20.0f * clamp(1.0f - _damage, 0.0f, 1.0f);

    if ( lerp > 1.0f ) {
        lerp = 1.0f;
    } else if ( lerp < 0.0f ) {
        lerp = 0.0f;
    }

    vec2    pos = get_position( lerp );
    float   angle = get_rotation( lerp );
    float   tangle = get_turret_rotation( lerp );

    color4 color_health;
    color4 color_reload;

    color_health.r = ( health < 10.0f ? 1.0f : (10.0f - (health - 10.0f)) / 10.0f );
    color_health.g = ( health > 10.0f ? 1.0f : (health / 10.0f) );
    color_health.b = 0.0f;
    color_health.a = 1.0f;

    color_reload.r = 1.0f;
    color_reload.g = reload/20.0f;
    color_reload.b = ( reload == 20.0f ? 1.0f : 0.0f );
    color_reload.a = 1.0f;

    if (_damage >= 1.0f)
    {
        _model->draw(pos, angle, color4(0.3f,0.3f,0.3f,1));
        _turret_model->draw(pos, tangle, color4(0.3f,0.3f,0.3f,1));
        return;
    }

    // status bars

    renderer->draw_box(vec2(20,2), pos + vec2(0,25), color4(0.5,0.5,0.5,1));
    renderer->draw_box(vec2(reload,2), pos + vec2(0,25), color_reload);
    renderer->draw_box(vec2(20,2), pos + vec2(0,22), color4(0.5,0.5,0.5,1));
    renderer->draw_box(vec2(health,2), pos + vec2(0,22), color_health);

    // actual body

    _model->draw(pos, angle, _color);
    _turret_model->draw(pos, tangle, _color);
}

//------------------------------------------------------------------------------
void tank::touch(object *other, physics::contact const* contact)
{
    if (!other || !contact) {
        return;
    }

    float impulse = contact->impulse.length();
    float strength = clamp((impulse - 5.0f) / 5.0f, 0.0f, 1.0f);
    _world->add_effect(effect_type::sparks, contact->point, -contact->normal, strength);

    if (other->_type != object_type::tank) {
        return;
    }

    tank* other_tank = static_cast<tank*>(other);

    this->collide(other_tank, contact);
    other_tank->collide(this, contact);
}

//------------------------------------------------------------------------------
void tank::collide(tank* other, physics::contact const* contact)
{
    if (other->_damage >= 1.0f) {
        return;
    }

    float base_damage = std::max<float>(0, contact->impulse.dot(contact->normal) - 10.0f) / 2.0f * FRAMETIME;

    vec2 direction = (contact->point - other->get_position()).normalize();
    vec2 forward = rotate(vec2(1,0), other->get_rotation());
    float impact_angle = direction.dot(forward);

    if (impact_angle > M_SQRT1_2) {
        base_damage *= DAMAGE_FRONT;
    } else if (impact_angle > -M_SQRT1_2) {
        base_damage *= DAMAGE_SIDE;
    } else {
        base_damage *= DAMAGE_REAR;
    }

    other->_damage += base_damage / other->_client->armor_mod;

    if (other->_damage >= 1.0f)
    {
        g_Game->add_score(_player_index, 1);
        other->_dead_time = _world->framenum() * FRAMETIME;

        g_Game->write_message( va("%s got a little too cozy with %s.", other->player_name(), player_name() ) );
    }
}

#define HACK_TIME       1.0f

//------------------------------------------------------------------------------
void tank::respawn()
{
    vec2 spawn_size = _world->maxs() - _world->mins() - vec2(SPAWN_BUFFER) * 2.0f;
    vec2 spawn_pos = _world->mins() + vec2(frand(), frand()) * spawn_size + vec2(SPAWN_BUFFER);

    _dead_time = 0.0f;

    set_position(spawn_pos, true);
    set_rotation(frand()*2.0f*M_PI, true);
    set_turret_rotation(get_rotation(), true);

    set_linear_velocity(vec2_zero);
    set_angular_velocity(0.0f);
    _turret_velocity = 0.0f;
    _track_speed = 0.0f;

    _damage = 0.0f;
}

//------------------------------------------------------------------------------
void tank::think()
{
    float const time = _world->framenum() * FRAMETIME;

    _old_turret_rotation = _turret_rotation;

    if ((_damage >= 1.0f) && (_dead_time+RESTART_TIME+HACK_TIME <= time)) {
        respawn();
    }

    vec2 forward = rotate(vec2(1,0), get_rotation());
    float speed = forward.dot(get_linear_velocity());

    vec2 track_velocity = rotate(vec2(_track_speed,0), get_rotation());
    vec2 delta_velocity = track_velocity - get_linear_velocity();
    vec2 friction_impulse = delta_velocity * _rigid_body.get_mass() * _rigid_body.get_material()->sliding_friction() * FRAMETIME;

    _rigid_body.apply_impulse(friction_impulse);

    if (_damage >= 1.0f)
    {
        vec2 vVel = vec2(speed * 0.98 * (1-FRAMETIME),0);

        set_linear_velocity(rotate(vVel,get_rotation()));
        set_angular_velocity(get_angular_velocity() * 0.9f);
        _turret_velocity *= 0.9f;

        // extra explosion
        if (_dead_time && (time - _dead_time > 0.65f) && (time - _dead_time < 0.65f+HACK_TIME/2))
        {
            _world->add_sound(_sound_explode, get_position());
            _world->add_effect(effect_type::explosion, get_position());
            _dead_time -= HACK_TIME;    // dont do it again
        }
    }
    else
    {
        float new_speed = _track_speed * 0.9 * (1 - FRAMETIME) + _usercmd.move[0] * 192 * _client->speed_mod * FRAMETIME;
        new_speed = clamp(new_speed, -32 * _client->speed_mod, 48 * _client->speed_mod);

        set_linear_velocity(get_linear_velocity() + forward * (new_speed - _track_speed));
        _track_speed = new_speed;

        set_angular_velocity(deg2rad(-_usercmd.move[1] * 90));
        _turret_velocity = deg2rad(-_usercmd.look[0] * 90);
    }

    // update position here because Move doesn't
    _turret_rotation += _turret_velocity * FRAMETIME;

    if (_usercmd.action == usercmd::action::attack && _damage < 1.0f) {
        launch_projectile();
    }

    update_effects();
    update_sound();
}

//------------------------------------------------------------------------------
void tank::launch_projectile()
{
    constexpr vec2 effect_origin(21,0);

    float const time = _world->framenum() * FRAMETIME;

    switch (_weapon) {
        case weapon_type::cannon: {
            if ((_fire_time + cannon_reload / _client->refire_mod) < time) {
                _fire_time = time;

                projectile* proj = _world->spawn<projectile>(this, _client->damage_mod, weapon_type::cannon);

                float launch_rotation = _turret_rotation + crand() * M_PI / 180.f;
                vec2 launch_direction = rotate(vec2(1, 0), launch_rotation);
                vec2 launch_position = get_position() + rotate(effect_origin, _turret_rotation);

                proj->set_position(launch_position, true);
                proj->set_linear_velocity(launch_direction * cannon_speed);

                _world->add_sound(_sound_cannon_fire, launch_position);
                _world->add_effect(
                    effect_type::smoke,
                    launch_position,
                    launch_direction * 21 * 16,
                    64 );

            }
            break;
        }

        case weapon_type::missile: {
            if ((_fire_time + missile_reload / _client->refire_mod) < time) {
                _fire_time = time;

                projectile* proj = _world->spawn<projectile>(this, _client->damage_mod, weapon_type::missile);

                vec2 launch_direction = rotate(vec2(1, 0), _turret_rotation);
                vec2 launch_position = get_position() + rotate(effect_origin, _turret_rotation);

                proj->set_position(launch_position, true);
                proj->set_linear_velocity(launch_direction * missile_speed);

                _world->add_sound(_sound_cannon_fire, launch_position);
            }
            break;
        }

        case weapon_type::blaster: {
            if ((_fire_time + blaster_reload / _client->refire_mod) < time) {
                _fire_time = time;

                projectile* proj = _world->spawn<projectile>(this, _client->damage_mod * 0.1f, weapon_type::blaster);

                float launch_rotation = _turret_rotation + crand() * M_PI / 180.f;
                vec2 launch_direction = rotate(vec2(1, 0), launch_rotation);
                vec2 launch_position = get_position() + rotate(effect_origin, _turret_rotation);

                proj->set_position(launch_position, true);
                proj->set_linear_velocity(launch_direction * blaster_speed);

                _world->add_sound(_sound_blaster_fire, launch_position);

                _world->add_effect(
                    effect_type::blaster,
                    launch_position,
                    launch_direction * 2);
            }
            break;
        }

        default:
            break;
    }
}

//------------------------------------------------------------------------------
void tank::update_effects()
{
    constexpr vec2 effect_origin(21,0);
    float const time = _world->framenum() * FRAMETIME;

    switch (_weapon) {
        case weapon_type::cannon: {
            if ((_fire_time + 2.5f/_client->refire_mod) > time) {
                float power;

                power = 1.5f - (time - _fire_time);
                power = clamp(power, 0.5f, 1.5f);

                _world->add_effect(
                    effect_type::smoke,
                    get_position() + rotate(effect_origin, _turret_rotation),
                    rotate(effect_origin, _turret_rotation) * power * power * power * 2,
                    power * power * 4 );
            }
            break;
        }

        case weapon_type::missile: {
            if ((_fire_time + 2.5f/_client->refire_mod) > time) {
                float power;

                power = 1.5f - (time - _fire_time);
                power = clamp(power, 0.5f, 1.5f);

                _world->add_effect(
                    effect_type::smoke,
                    get_position() + rotate(effect_origin, _turret_rotation),
                    rotate(effect_origin, _turret_rotation) * power * power * power * 2,
                    power * power * 2 );
            }
            break;
        }

        case weapon_type::blaster:
        default:
            break;
    }
}

//------------------------------------------------------------------------------
void tank::read_snapshot(network::delta_message const& message)
{
    _old_position = get_position();
    _old_rotation = get_rotation();
    _old_turret_rotation = get_turret_rotation();

    _player_index = message.read_byte();
    _client = g_Game->_clients + _player_index;
    _world->_players[_player_index] = this;
    _color.r = message.read_float();
    _color.g = message.read_float();
    _color.b = message.read_float();
    set_position(message.read_vector());
    set_linear_velocity(message.read_vector());
    set_rotation(message.read_float());
    set_angular_velocity(message.read_float());
    _turret_rotation = message.read_float();
    _turret_velocity = message.read_float();
    _damage = message.read_float();
    _fire_time = message.read_float();

    update_sound();
}

//------------------------------------------------------------------------------
void tank::write_snapshot(network::delta_message& message) const
{
    message.write_byte(_player_index);
    message.write_float(_color.r);
    message.write_float(_color.g);
    message.write_float(_color.b);
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
    message.write_float(get_rotation());
    message.write_float(get_angular_velocity());
    message.write_float(_turret_rotation);
    message.write_float(_turret_velocity);
    message.write_float(_damage);
    message.write_float(_fire_time);
}

//------------------------------------------------------------------------------
float tank::get_turret_rotation(float lerp) const
{
    return _old_turret_rotation + (_turret_rotation - _old_turret_rotation) * lerp;
}

//------------------------------------------------------------------------------
void tank::set_turret_rotation(float rotation, bool teleport/* = false*/)
{
    _turret_rotation = rotation;
    if (teleport) {
        _old_turret_rotation = rotation;
    }
}

//------------------------------------------------------------------------------
void tank::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

//------------------------------------------------------------------------------
void tank::update_sound()
{
    // engine noise
    if (_channels[0]) {
        if (_damage < 1.0f) {
            if (!_channels[0]->playing()) {
                _channels[0]->loop(_sound_idle);
            }
            _channels[0]->set_origin(vec3(get_position()));
            _channels[0]->set_volume(0.5f);
            _channels[0]->set_attenuation(1.0f);
            _channels[0]->set_frequency(1.0f);
        }
        else if (_channels[0]->playing()) {
            _channels[0]->stop();
        }
    }

    // tread noise
    if (_channels[1]) {
        if (get_linear_velocity().length_sqr() > 1.0f || fabs(get_angular_velocity()) > deg2rad(1.0f)) {
            if (!_channels[1]->playing()) {
                _channels[1]->loop(_sound_move);
            }
            _channels[1]->set_origin(vec3(get_position()));
            _channels[1]->set_volume(0.5f);
            _channels[1]->set_attenuation(1.0f);
        }
        else if (_channels[1]->playing()) {
            _channels[1]->stop();
        }
    }

    // turret noise
    if (_channels[2]) {
        if (fabs(get_angular_velocity() - _turret_velocity) > deg2rad(1.0f)) {
            if (!_channels[2]->playing()) {
                _channels[2]->loop(_sound_turret_move);
            }
            _channels[2]->set_origin(vec3(get_position()));
            _channels[2]->set_volume(0.3f);
            _channels[2]->set_attenuation(1.0f);
        }
        else if (_channels[2]->playing()) {
            _channels[2]->stop( );
        }
    }
}

//------------------------------------------------------------------------------
char const* tank::player_name() const
{
    return va("\\c%02x%02x%02x%s\\cx",
              int(_color.r * 255),
              int(_color.g * 255),
              int(_color.b * 255),
              g_Game->svs.clients[_player_index].info.name.data());
}

////////////////////////////////////////////////////////////////////////////////
physics::circle_shape projectile::_shape(1.0f);
physics::material projectile::_material(0.5f, 1.0f);

//------------------------------------------------------------------------------
projectile::projectile(tank* owner, float damage, weapon_type type)
    : object(object_type::projectile, owner)
    , _damage(damage)
    , _type(type)
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 1.0f);
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
    float bestValue = float(M_SQRT1_2);
    game::object* bestTarget = nullptr;

    for (auto& obj : _world->objects()) {
        if (obj->_type != object_type::tank) {
            continue;
        }
        if (static_cast<game::tank*>(obj.get())->_damage >= 1.0f) {
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
        vec2 delta = displacement * speed * 0.5f * FRAMETIME;
        vec2 new_velocity = (get_linear_velocity() + delta).normalize() * speed;
        set_linear_velocity(new_velocity);
    }
}

//------------------------------------------------------------------------------
void projectile::update_effects()
{
    if (_type == weapon_type::missile) {
        _world->add_trail_effect(
            effect_type::missile_trail,
            get_position(),
            _old_position,
            get_linear_velocity() * -0.5f,
            4 );
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
            _channel->set_volume(0.2f);
            _channel->set_attenuation(0.0f);
            _channel->set_origin(vec3(get_position()));
        }
    } else if (_channel && _channel->playing()) {
        _channel->stop();
    }
}

//------------------------------------------------------------------------------
void projectile::touch(object *other, physics::contact const* contact)
{
    auto sound = _type == weapon_type::cannon ? _sound_cannon_impact :
                 _type == weapon_type::missile ? _sound_cannon_impact :
                 _type == weapon_type::blaster ? _sound_blaster_impact : sound::asset::invalid;

    auto effect = _type == weapon_type::cannon ? effect_type::cannon_impact :
                  _type == weapon_type::missile ? effect_type::missile_impact :
                  _type == weapon_type::blaster ? effect_type::blaster_impact : effect_type::smoke;

    if (contact) {
        _world->add_sound(sound, contact->point);
        _world->add_effect(effect, contact->point, -contact->normal, 0.5f * _damage);
    } else {
        _world->add_sound(sound, get_position());
        _world->add_effect(effect, get_position(), vec2_zero, 0.5f * _damage);
    }

    _world->remove(this);

    if (other && other->_type == object_type::tank) {
        tank* owner_tank = static_cast<tank*>(_owner);
        tank* other_tank = static_cast<tank*>(other);

        if (other_tank->_damage >= 1.0f) {
            return; // dont add damage or score
        }

        vec2 direction = get_linear_velocity().normalize();
        vec2 impact_normal = contact ? -contact->normal : -direction;
        vec2 forward = rotate(vec2(1,0), other->get_rotation());
        float impact_angle = impact_normal.dot(forward);
        float damage = _damage / other_tank->_client->armor_mod;

        if (_type == weapon_type::cannon) {
            float surface_angle = impact_normal.dot(-direction);
            damage = 1.2f * damage * surface_angle;
            if (impact_angle > M_SQRT1_2) {
                other_tank->_damage += damage * DAMAGE_FRONT;
            } else if (impact_angle > -M_SQRT1_2) {
                other_tank->_damage += damage * DAMAGE_SIDE;
            } else {
                other_tank->_damage += damage * DAMAGE_REAR;
            }
        } else if (_type == weapon_type::missile) {
            other_tank->_damage += damage * DAMAGE_SIDE;
        } else if (_type == weapon_type::blaster) {
            if (impact_angle > M_SQRT1_2) {
                other_tank->_damage += damage * DAMAGE_FRONT / 1.25f;
            } else if (impact_angle > -M_SQRT1_2) {
                other_tank->_damage += damage * DAMAGE_SIDE * 1.25f;
            } else {
                other_tank->_damage += damage * DAMAGE_REAR * 1.5f;
            }
        }

        if (other_tank->_damage >= 1.0f) {
            g_Game->add_score( owner_tank->_player_index, 1 );
            other_tank->_dead_time = _world->framenum() * FRAMETIME;

            char const* fmt = "";

            switch (_type) {
                case weapon_type::cannon:
                    switch (rand()%3) {
                        case 0: fmt = "%s couldn't take %s's HEAT."; break;
                        case 1: fmt = "%s was on the wrong end of %s's cannon."; break;
                        case 2: fmt = "%s ate all 125mm of %s's boom stick."; break;
                    }
                    break;

                case weapon_type::missile:
                    switch (rand()%2) {
                        case 0: fmt = "%s couldn't hide from %s's heat seeker."; break;
                        case 1: fmt = "%s ate %s's rocket."; break;
                    }
                    break;

                case weapon_type::blaster:
                    switch (rand()%2) {
                        case 0: fmt = "%s was melted by %s's blaster."; break;
                        case 1: fmt = "%s was blasted by %s."; break;
                    }
                    break;
            }

            g_Game->write_message(va(fmt, other_tank->player_name(), owner_tank->player_name()));
        }
    }
}

//------------------------------------------------------------------------------
void projectile::draw(render::system* renderer, float time) const
{
    float lerp = (time - _world->framenum() * FRAMETIME) / FRAMETIME;

    vec2 p1 = get_position(lerp);
    vec2 p2 = get_position(lerp + 0.4f);

    switch (_type) {
        case weapon_type::cannon:
            renderer->draw_line(p2, p1, color4(1,0.5,0,1), color4(1,0.5,0,0));
            break;

        case weapon_type::missile:
            renderer->draw_line(p2, p1, color4(1,1,1,1), color4(0,0,0,0));
            break;

        case weapon_type::blaster:
            renderer->draw_line(p2, p1, color4(1,0.1f,0,1), color4(1,0.7f,0,0));
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------
void projectile::read_snapshot(network::delta_message const& message)
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
void projectile::write_snapshot(network::delta_message& message) const
{
    message.write_long(_owner->spawn_id());
    message.write_float(_damage);
    message.write_byte(static_cast<int>(_type));
    message.write_vector(get_position());
    message.write_vector(get_linear_velocity());
}

} // namespace game

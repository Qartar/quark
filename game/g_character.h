// g_character.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

class ship;
class subsystem;
class ship_layout;

//------------------------------------------------------------------------------
class character : public object
{
public:
    enum class action_type {
        idle,
        move,
        operate,
        repair,
    };

    static const object_type _type;

public:
    character();
    virtual ~character();

    void spawn();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

    string::view name() const { return _name; }
    void damage(object* inflictor, float amount);
    float health() const { return _health; }
    float radius() const { return _radius; }

    using object::set_position;
    void set_position(handle<ship> ship, vec2 position, bool teleport = false);

    //! return the compartment containing the character
    uint16_t compartment() const;

    //! return the current action type
    action_type action() const { return _action; }

    //! return the subsystem this character is repairing (or moving to repair)
    handle<game::subsystem> repair_target() const;

    //! return the compartment this character is moving towards
    uint16_t move_target() const;

    //! return true if the character is not performing any action
    bool is_idle() const;

    //! return true if the character is moving regardless of current action type
    bool is_moving() const;

    // return true if the character is currently repairing the given subsystem
    bool is_repairing(handle<game::subsystem> subsystem) const;

    //! repair the given subsystem, moving if necessary
    bool repair(handle<game::subsystem> subsystem);

    //! repair the given ship compartment, moving if necessary
    bool repair(uint16_t compartment);

    //! operate the given subsystem, moving if necessary
    bool operate(handle<game::subsystem> subsystem);

    //! move to a random point in the compartment containing the given subsystem
    bool move(handle<game::subsystem> subsystem);

    //! move to specific point in ship-local space
    bool move(vec2 goal);

    //! move to a random point in the given compartment
    bool move(uint16_t compartment);

    vec2 get_path(int index) const { return _path[_path_start + index]; }
    std::size_t get_path_length() const { return _path_end - _path_start; }

protected:
    string::buffer _name;
    float _health;

    action_type _action;
    handle<game::subsystem> _target_subsystem;
    uint16_t _target_compartment;

    static constexpr int path_size = 32;
    vec2 _path[path_size];
    std::size_t _path_start;
    std::size_t _path_end;

    handle<ship> _ship;

    static constexpr float _radius = .3f;

    static constexpr float repair_rate = 1.f / 15.f; //!< damage per second
};

} // namespace game

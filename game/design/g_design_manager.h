// g_design_manager.h
//

#pragma once

#include "cm_string.h"

#include <memory>
#include <map>

////////////////////////////////////////////////////////////////////////////////
class lexer;

namespace game {

struct gun_design;
struct ship_design;
struct turret_design;

//------------------------------------------------------------------------------
class design_manager
{
public:
    design_manager();
    ~design_manager();

    void clear();
    void load_historical();

    gun_design* find_gun(string::view id) const;
    ship_design* find_ship(string::view id) const;
    turret_design* find_turret(string::view id) const;

protected:
    std::map<string::view, std::unique_ptr<gun_design>> _guns;
    std::map<string::view, std::unique_ptr<ship_design>> _ships;
    std::map<string::view, std::unique_ptr<turret_design>> _turrets;

protected:
    bool load_gun(string::view filename, gun_design& gun);
    bool load_ship(string::view filename, ship_design& ship);
    bool load_turret(string::view filename, turret_design& turret);
};

} // namespace game

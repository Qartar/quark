// g_design_manager.h
//

#pragma once

#include "cm_string.h"

#include <memory>
#include <map>

////////////////////////////////////////////////////////////////////////////////
class lexer;

namespace game {

struct funnel_design;
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

    funnel_design* find_funnel(string::view id) const;
    gun_design* find_gun(string::view id) const;
    ship_design* find_ship(string::view id) const;
    turret_design* find_turret(string::view id) const;

protected:
    std::map<string::view, std::unique_ptr<funnel_design>> _funnels;
    std::map<string::view, std::unique_ptr<gun_design>> _guns;
    std::map<string::view, std::unique_ptr<ship_design>> _ships;
    std::map<string::view, std::unique_ptr<turret_design>> _turrets;
};

} // namespace game

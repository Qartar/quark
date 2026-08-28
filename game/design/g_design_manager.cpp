// g_design_manager.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_design_manager.h"

#include "cm_filesystem.h"
#include "cm_lexer.h"
#include "g_funnel_design.h"
#include "g_gun_design.h"
#include "g_ship_design.h"
#include "g_turret_design.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
design_manager::design_manager()
{
}

//------------------------------------------------------------------------------
design_manager::~design_manager()
{
}

//------------------------------------------------------------------------------
void design_manager::clear()
{
    _guns.clear();
    _ships.clear();
    _turrets.clear();
}

//------------------------------------------------------------------------------
void design_manager::load_historical()
{
    log::message("loading historical designs...\n");
    time_value t0 = time_value::current();

    for (file::find f("assets/design/historical/funnel/", "*.design"); f; ++f) {
        funnel_design funnel{};
        if (load_funnel(f.fullname(), funnel)) {
            auto funnel_ptr = std::make_unique<funnel_design>(std::move(funnel));
            _funnels[funnel_ptr->id] = std::move(funnel_ptr);
        }
    }

    for (file::find f("assets/design/historical/gun/", "*.design"); f; ++f) {
        gun_design gun{};
        if (load_gun(f.fullname(), gun)) {
            auto gun_ptr = std::make_unique<gun_design>(std::move(gun));
            _guns[gun_ptr->id] = std::move(gun_ptr);
        }
    }

    for (file::find f("assets/design/historical/turret/", "*.design"); f; ++f) {
        turret_design turret{};
        if (load_turret(f.fullname(), turret)) {
            auto turret_ptr = std::make_unique<turret_design>(std::move(turret));
            _turrets[turret_ptr->id] = std::move(turret_ptr);
        }
    }

    for (file::find f("assets/design/historical/ship/", "*.design"); f; ++f) {
        vec2 verts[] = {vec2(0,0),vec2(1,0),vec2(0,1)};
        // No default constructor for ship_design because hull_shape is not default constructible
        ship_design d{string::buffer(),string::buffer(),0,0,0,0,0,0,0,0,0,0,{},{},{},
            {{{std::make_unique<physics::convex_shape>(verts)}}}
        };
        if (load_ship(f.fullname(), d)) {
            auto dptr = std::make_unique<ship_design>(std::move(d));
            _ships[dptr->id] = std::move(dptr);
        }
    }

    time_delta dt = time_value::current() - t0;
    log::message("...%zu designs in %lg ms\n",
        _funnels.size() + _guns.size() + _ships.size() + _turrets.size(),
        dt.to_seconds() * 1e3);
}

//------------------------------------------------------------------------------
funnel_design* design_manager::find_funnel(string::view id) const
{
    auto kv = _funnels.find(id);
    return kv == _funnels.end() ? nullptr : kv->second.get();
}

//------------------------------------------------------------------------------
gun_design* design_manager::find_gun(string::view id) const
{
    auto kv = _guns.find(id);
    return kv == _guns.end() ? nullptr : kv->second.get();
}

//------------------------------------------------------------------------------
ship_design* design_manager::find_ship(string::view id) const
{
    auto kv = _ships.find(id);
    return kv == _ships.end() ? nullptr : kv->second.get();
}

//------------------------------------------------------------------------------
turret_design* design_manager::find_turret(string::view id) const
{
    auto kv = _turrets.find(id);
    return kv == _turrets.end() ? nullptr : kv->second.get();
}

//------------------------------------------------------------------------------
bool design_manager::load_funnel(string::view filename, funnel_design& funnel)
{
    file::buffer b = file::read(filename);
    lexer lex(string::view{(char const*)b.data(), (char const*)b.data() + b.size()}, filename);
    lexer::token t;
    if (lex.expect_token_type(t, lexer::token_type::name)
        && lex.expect_token("=")
        && funnel_design::parse(lex, *this, funnel)
        && lex.expect_token(";")) {
        funnel.id = string::buffer(t);
        return true;
    } else {
        log::error("%s\n", lex.last_error().message.c_str());
        return false;
    }
}

//------------------------------------------------------------------------------
bool design_manager::load_gun(string::view filename, gun_design& gun)
{
    file::buffer b = file::read(filename);
    lexer lex(string::view{(char const*)b.data(), (char const*)b.data() + b.size()}, filename);
    lexer::token t;
    if (lex.expect_token_type(t, lexer::token_type::name)
        && lex.expect_token("=")
        && gun_design::parse(lex, *this, gun)
        && lex.expect_token(";")) {
        gun.id = string::buffer(t);
        return true;
    } else {
        log::error("%s\n", lex.last_error().message.c_str());
        return false;
    }
}

//------------------------------------------------------------------------------
bool design_manager::load_ship(string::view filename, ship_design& ship)
{
    file::buffer b = file::read(filename);
    lexer lex(string::view{(char const*)b.data(), (char const*)b.data() + b.size()}, filename);
    lexer::token t;
    if (lex.expect_token_type(t, lexer::token_type::name)
        && lex.expect_token("=")
        && ship_design::parse(lex, *this, ship)
        && lex.expect_token(";")) {
        ship.id = string::buffer(t);
        return true;
    } else {
        log::error("%s\n", lex.last_error().message.c_str());
        return false;
    }
}

//------------------------------------------------------------------------------
bool design_manager::load_turret(string::view filename, turret_design& turret)
{
    file::buffer b = file::read(filename);
    lexer lex(string::view{(char const*)b.data(), (char const*)b.data() + b.size()}, filename);
    lexer::token t;
    if (lex.expect_token_type(t, lexer::token_type::name)
        && lex.expect_token("=")
        && turret_design::parse(lex, *this, turret)
        && lex.expect_token(";")) {
        turret.id = string::buffer(t);
        return true;
    } else {
        log::error("%s\n", lex.last_error().message.c_str());
        return false;
    }
}

} // namespace game

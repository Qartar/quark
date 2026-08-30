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

namespace {

//------------------------------------------------------------------------------
template<typename T> bool load_design(design_manager& manager, string::view filename, T& design)
{
    file::buffer b = file::read(filename);
    lexer lex(string::view{(char const*)b.data(), (char const*)b.data() + b.size()}, filename);
    lexer::token t;
    if (lex.expect_token_type(t, lexer::token_type::name)
        && lex.expect_token("=")
        && T::parse(lex, manager, design)
        && lex.expect_token(";")) {
        design.id = string::buffer(t);
        return true;
    } else {
        log::error("%s\n", lex.last_error().message.c_str());
        return false;
    }
}

//------------------------------------------------------------------------------
template<typename T> void load_designs(design_manager& manager, string::literal path, std::map<string::view, std::unique_ptr<T>>& map)
{
    for (file::find f(path, "*.design"); f; ++f) {
        T design{};
        if (!load_design(manager, f.fullname(), design)) {
            continue;
        }
        if (map.find(design.id) != map.end()) {
            // Would be nice to have the filename of the original definition as well
            log::warning("ignoring duplicate definition for '%.*s' in file '%.*s'\n",
                int(design.id.length()), design.id.begin(),
                int(f.fullname().length()), f.fullname().begin());
            continue;
        }
        auto design_ptr = std::make_unique<T>(std::move(design));
        map[design_ptr->id] = std::move(design_ptr);
    }
}

} // anonymous namespace

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

    load_designs<funnel_design>(*this, "assets/design/historical/funnel/", _funnels);
    load_designs<gun_design>(*this, "assets/design/historical/gun/", _guns);
    load_designs<turret_design>(*this, "assets/design/historical/turret/", _turrets);
    load_designs<ship_design>(*this, "assets/design/historical/ship/", _ships);

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

} // namespace game

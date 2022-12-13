// ed_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "ed_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

#include <algorithm>
#include <iterator>

////////////////////////////////////////////////////////////////////////////////
namespace editor {

namespace {

std::vector<std::size_t> sorted_union(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(out));

    return out;
}

std::vector<std::size_t> sorted_difference(std::vector<std::size_t> const& a, std::vector<std::size_t> const& b)
{
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());
    std::set_difference(a.begin(), a.end(),
                        b.begin(), b.end(),
                        std::back_inserter(out));

    return out;
}

} // anonymous namespace

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
{
    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.f, 64.f * float(_view.viewport.maxs().y) / float(_view.viewport.maxs().x)};

}

//------------------------------------------------------------------------------
ship_editor::~ship_editor()
{
}

//------------------------------------------------------------------------------
vec2 ship_editor::cursor_to_world() const
{
    //return _view.origin + _view.size * (_cursor / 
    //    vec2(_view.viewport.maxs() - _view.viewport.mins()) - vec2(.5f,.5f));
    return _view.origin + _view.size * _cursor;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value time) const
{
    (void)time;
    renderer->set_view(_view);
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
    if (!down) {
        switch (key) {
            case K_MOUSE1:
                on_click();
                break;

            case K_CTRL:
                break;

            case K_SHIFT:
                break;

            default:
                break;
        }

        return false;
    }

    switch (key) {
        case K_MOUSE1:
            break;

        case K_MOUSE2:
            break;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    _cursor = position;
}

//------------------------------------------------------------------------------
void ship_editor::on_click()
{
}

} // namespace editor

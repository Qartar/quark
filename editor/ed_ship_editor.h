// ed_ship_editor.h
//

#pragma once

#include "r_main.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
class image;
} // namespace render

namespace editor {

//------------------------------------------------------------------------------
class ship_editor
{
public:
    ship_editor();
    virtual ~ship_editor();

    void draw(render::system* renderer, time_value time) const;

    bool key_event(int key, bool down);
    void cursor_event(vec2 position);

protected:
    render::view _view;
    vec2 _cursor;

protected:
    vec2 cursor_to_world() const;

    void on_click();
};

} // namespace editor

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
struct shape {
    using index = std::size_t;
    static constexpr index invalid_index = SIZE_MAX;
    std::vector<vec2> points;
};

//------------------------------------------------------------------------------
struct palette {
    std::vector<shape> shapes;
};

//------------------------------------------------------------------------------
struct shape_instance {
    shape::index index;
    vec2 position;
    float rotation;
};

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

    palette _palette;

    shape::index _shape_index;
    std::vector<shape_instance> _shape_instances;

    bool _snap_to_grid;
    bool _snap_to_edge;
    bool _snap_to_vertex;
    float _grid_size;

protected:
    vec2 cursor_to_world() const;
    vec2 snap_position(vec2 p) const;
    bool snap_to_grid(vec2& p) const;
    bool snap_to_edge(vec2& p) const;
    bool snap_to_vertex(vec2& p) const;

    void on_click();

    void draw_palette(render::system* renderer, time_value time) const;
};

} // namespace editor

// g_ship_editor.h
//

#pragma once

#include "r_main.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

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
    std::vector<vec2> _render_verts;
    std::vector<int> _render_tris;

    render::view _view;
    vec2 _cursor;

    enum class editor_mode {
        none,
        vertices,
        triangles,
    };

    editor_mode _mode;

    float _snap_distance;
    bool _snap_to_grid;
    bool _snap_to_edge;
    bool _snap_to_mirror;
    bool _mirror;

protected:
    vec2 cursor_to_world() const;
    vec2 snap_vertex(vec2 pos) const;
};

} // namespace game

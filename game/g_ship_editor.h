// g_ship_editor.h
//

#pragma once

#include "r_main.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
class image;
} // namespace render

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
    std::vector<vec2> _guide_vertices;
    std::vector<std::size_t> _guide_triangles;

    std::vector<vec2> _render_vertices;
    std::vector<std::size_t> _render_triangles;

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

    render::image const* _image;
    vec2 _image_offset;
    vec2 _image_scale;
    float _image_rotation;

    std::size_t _triangle[2];
    std::size_t _triangle_mirror[2];
    std::size_t _triangle_size;

protected:
    vec2 cursor_to_world() const;
    vec2 snap_vertex(vec2 pos) const;

    std::size_t insert_vertex(vec2 pos);
};

} // namespace game
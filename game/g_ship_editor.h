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
    enum segment_type {
        line,
        quad,
        cube,
    };
    std::vector<vec2> _deck_vertices;
    std::vector<segment_type> _deck_segments;
    std::vector<vec2> _deck_linearized;

    render::view _view;
    vec2 _cursor;

    float _snap_distance;
    bool _snap_to_grid;
    bool _snap_to_edge;
    bool _draw_grid;
    bool _draw_linearized;

    bool _is_panning;
    bool _is_panning_image;
    bool _control;

    bool _is_dragging;
    std::size_t _drag_index;

    render::image const* _image;
    vec2 _image_offset;
    config::scalar _image_scale;

    //! minimum distance between vertices squared
    static constexpr float minimum_vertex_dsqr = 1.f;

protected:
    vec2 cursor_to_world() const;
    vec2 snap_vertex(vec2 pos) const;

    void draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const;
    void draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const;

    //! Return the closest point on the given curve segments to the given point
    vec2 closest_point(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments, vec2 v) const;

    bool insert_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);
    bool remove_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);

    void save(string::view filename) const;
    bool load(string::view filename);
    void export_verts(string::view filename) const;

    //! Convert the given curve segments into a loop of vertices approximating the curve
    static std::vector<vec2> linearize(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments);

    static std::vector<vec2> subdivide(std::function<vec2(float)> fn, float error);
};

} // namespace game

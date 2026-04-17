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

    struct turret {
        float radius;
        std::vector<vec2> vertices;
        std::vector<segment_type> segments;
        std::vector<vec2> linearized;
    };

    struct turret_instance {
        mat3 transform;
        std::size_t index;
    };

    std::vector<turret> _turrets;
    std::vector<turret_instance> _turret_instances;

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

    enum class feature {
        none,
        vertex,
        vertex_mirror,
        turret,
        turret_radius,
        turret_rotation,
    };

    feature _drag_feature;
    std::size_t _drag_index;

    feature _highlight_feature;
    std::size_t _highlight_index;

    string::buffer _filename;

    render::image const* _image;
    vec2 _image_offset;
    config::scalar _image_scale;

    enum class editor_mode {
        deck,
        turret,
    };

    editor_mode _mode;
    std::size_t _turret_instance;

    //! minimum distance between vertices squared
    static constexpr float minimum_vertex_dsqr = 1.f;

protected:
    vec2 cursor_to_world() const;
    float snap_radius(float r) const;
    vec2 snap_vertex(vec2 pos) const;

    float render_vertex_size() const { return _view.size.y * (1.f / 384.f); }

    void update_highlight();

    void draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const;
    void draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const;

    void draw_transformed(render::system* renderer, mat3 transform, std::vector<vec2> const& linearized) const;
    void draw_transformed(render::system* renderer, mat3 transform, std::vector<vec2> const& vertices, std::vector<segment_type> const& segments) const;

    //! Return the index of the closest vertex to the given point
    std::size_t closest_vertex(std::vector<vec2> const& vertices, vec2 v) const;
    //! Return the index of the closest segment to the given point
    std::size_t closest_segment(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments, vec2 v) const;
    //! Return the closest point on the given curve segments to the given point
    vec2 closest_point(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments, vec2 v) const;

    bool insert_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);
    bool remove_vertex(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);

    bool upconvert_segment(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);
    bool downconvert_segment(std::vector<vec2>& vertices, std::vector<segment_type>& segments, vec2 v);

    bool insert_turret(vec2 v);
    bool remove_turret(vec2 v);

    bool get_save_filename(string::buffer& filename) const;
    bool get_load_filename(string::buffer& filename) const;
    bool get_image_filename(string::buffer& filename) const;

    void clear();
    bool save(string::view filename) const;
    bool load(string::view filename);
    void export_verts(string::view filename) const;

    //! Convert the given curve segments into a loop of vertices approximating the curve
    static std::vector<vec2> linearize(std::vector<vec2> const& vertices, std::vector<segment_type> const& segments);

    static std::vector<vec2> subdivide(std::function<vec2(float)> fn, float error);
};

} // namespace game

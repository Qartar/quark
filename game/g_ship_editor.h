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
class ship_outline
{
public:
    enum segment_type {
        line,
        quad,
        cube,
    };

public:
    ship_outline(std::vector<vec2>&& vertices, std::vector<segment_type>&& segments);

    void draw(render::system* renderer, mat3 transform, vec2 vertex_size) const;
    void draw_linearized(render::system* renderer, mat3 transform, vec2 vertex_size) const;

    std::vector<vec2>& vertices() { return _vertices; }

    std::vector<vec2> const& vertices() const { return _vertices; }
    std::vector<segment_type> const& segments() const { return _segments; }
    std::vector<vec2> const& linearized() const { return _linearized; }

    //! Return the index of the closest vertex to the given point
    std::size_t closest_vertex(vec2 v) const;
    //! Return the index of the closest segment to the given point
    std::size_t closest_segment(vec2 v) const;
    //! Return the closest point on the given curve segments to the given point
    vec2 closest_point(vec2 v) const;

    bool insert_vertex(vec2 v, double minimum_vertex_dsqr);
    bool remove_vertex(vec2 v, double minimum_vertex_dsqr);

    bool upconvert_segment(vec2 v);
    bool downconvert_segment(vec2 v);

    //! Convert the given curve segments into a loop of vertices approximating the curve
    void linearize();

protected:
    std::vector<vec2> _vertices;
    std::vector<segment_type> _segments;
    std::vector<vec2> _linearized;

protected:
    void draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const;
    void draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const;

    static std::vector<vec2> subdivide(std::function<vec2(double)> fn, float error);
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
    struct turret {
        double radius;
        std::size_t index; //!< outline index
    };

    struct turret_instance {
        mat3 transform;
        std::size_t index; //!< turret index
    };

    std::vector<ship_outline> _outlines;

    std::vector<turret> _turrets;
    std::vector<turret_instance> _turret_instances;

    render::view _view;
    vec2 _cursor;

    double _snap_distance;
    bool _snap_to_grid;
    bool _snap_to_edge;
    bool _draw_grid;
    bool _draw_linearized;

    bool _is_panning;
    bool _is_panning_image;
    bool _control;
    bool _is_dragging;

    enum class feature {
        none,
        vertex,
        vertex_mirror,
        turret,
        turret_radius,
        turret_rotation,
    };

    feature _feature;
    std::size_t _feature_index;
    std::size_t _feature_outline;
    mat3 _feature_transform;

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
    static constexpr double minimum_vertex_dsqr = 1.0;

protected:
    vec2 cursor_to_world() const;
    double snap_radius(double r) const;
    vec2 snap_vertex(vec2 pos) const;

    double render_vertex_size() const { return _view.size.y * (1.0 / 384.0); }

    void update_highlight();

    bool insert_turret(vec2 v);
    bool remove_turret(vec2 v);

    bool get_save_filename(string::buffer& filename) const;
    bool get_load_filename(string::buffer& filename) const;
    bool get_image_filename(string::buffer& filename) const;

    void clear();
    bool save(string::view filename) const;
    bool load(string::view filename);
    void export_verts(string::view filename) const;
};

} // namespace game

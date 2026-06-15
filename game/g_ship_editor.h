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
    ship_outline(std::vector<vec3>&& vertices, std::vector<segment_type>&& segments);

    void draw(render::system* renderer, mat4 transform, vec2 vertex_size) const;
    void draw_linearized(render::system* renderer, mat4 transform, vec2 vertex_size) const;

    std::vector<vec3>& vertices() { return _vertices; }

    std::vector<vec3> const& vertices() const { return _vertices; }
    std::vector<segment_type> const& segments() const { return _segments; }
    std::vector<vec3> const& linearized() const { return _linearized; }

    //! Return the index of the closest vertex to the given point
    std::size_t closest_vertex(vec3 v, mat4 projection) const;
    //! Return the index of the closest segment to the given point
    std::size_t closest_segment(vec3 v, mat4 projection) const;
    //! Return the closest point on the given curve segments to the given point
    vec2 closest_point(vec3 v, mat4 projection) const;

    bool insert_vertex(vec3 v, mat4 projection, double minimum_vertex_dsqr);
    bool remove_vertex(vec3 v, mat4 projection, double minimum_vertex_dsqr);

    bool upconvert_segment(vec3 v, mat4 projection);
    bool downconvert_segment(vec3 v, mat4 projection);

    //! Convert the given curve segments into a loop of vertices approximating the curve
    void linearize();

protected:
    std::vector<vec3> _vertices;
    std::vector<segment_type> _segments;
    std::vector<vec3> _linearized;

protected:
    void draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const;
    void draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const;

    static std::vector<vec3> subdivide(std::function<vec3(double)> fn, float error);
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
        mat4 transform;
        mat4 inverse_transform;
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
    mat4 _feature_transform;
    mat4 _feature_inverse_transform;

    enum class viewport {
        plan, //!< XY projection
        profile, //!< XZ projection
    };

    viewport _viewport;
    mat4 _viewport_projection; //!< Projection matrix from 3D to 2D

    string::buffer _filename;

    render::image const* _plan_image;
    vec2 _plan_image_offset;
    config::scalar _plan_image_scale;
    config::scalar _plan_image_rotation;

    render::image const* _profile_image;
    vec2 _profile_image_offset;
    config::scalar _profile_image_scale;
    config::scalar _profile_image_rotation;

    enum class editor_mode {
        deck,
        turret,
    };

    editor_mode _mode;
    std::size_t _turret_instance;

    //! minimum distance between vertices squared
    static constexpr double minimum_vertex_dsqr = 1.0;

protected:
    vec3 cursor_to_world() const;
    vec3 screen_to_world(vec2 p) const;
    double snap_radius(double r) const;
    vec3 snap_vertex(vec3 pos) const;
    vec2 snap_vertex(vec2 pos) const;

    void draw_view(render::system* renderer, render::view const& view, render::image const* image, vec2 image_offset, double image_rotation, double image_scale) const;

    double render_vertex_size() const { return _view.size.y * (1.0 / 384.0); }

    void update_highlight();

    bool insert_turret(vec3 v, mat4 projection);
    bool remove_turret(vec3 v, mat4 projection);

    bool get_save_filename(string::buffer& filename) const;
    bool get_load_filename(string::buffer& filename) const;
    bool get_image_filename(string::buffer& filename) const;

    void clear();
    bool save(string::view filename) const;
    bool load(string::view filename);
    void export_verts(string::view filename) const;

    static constexpr mat4 plan_projection = mat4(1,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,1);
    static constexpr mat4 profile_projection = mat4(1,0,0,0, 0,0,0,0, 0,1,0,0, 0,0,0,1);
};

} // namespace game

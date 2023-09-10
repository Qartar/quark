// ed_ship_editor.h
//

#pragma once

#include "r_main.h"
#include "cm_curve.h"
#include "cm_clothoid.h"
#include "cm_clothoid_network.h"

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
    mutable float _anim;
    mutable time_value _last_time;

    mutable std::vector<float> _forward_acceleration;
    mutable std::vector<float> _lateral_acceleration;
    mutable std::vector<float> _speed;
    mutable std::vector<float> _max_speed;
    mutable std::vector<float> _length;
    mutable std::size_t _graph_index;

    struct qcurve {
        vec2 a, b, c;
    };

    using ccurve_coeff = curve::cubic;

    struct ccurve {
        vec2 a, b, c, d;
        ccurve_coeff to_coeff() const {
            return ccurve_coeff::from_bezier(a, b, c, d);
        }
    };

    enum class state {
        ready,
        started,
        beta,
        arc_start,
    } _state;

    vec2 _start;
    vec2 _stop;

    float _curvature;

    vec2 _start_tangent;
    float _start_curvature;
    std::size_t _start_section;

    float _beta1;
    float _beta2;

    std::vector<qcurve> _qspline;
    std::vector<ccurve> _cspline;
    std::vector<float> _cspline_length;

    std::vector<vec2> _cspline_max_speed;

    clothoid::network _network;

    mutable clothoid::segment _section;
    mutable bool _has_section = false;

protected:
    vec2 cursor_to_world() const;

    void draw_qspline(render::system* renderer, qcurve const& q) const;
    void draw_cspline(render::system* renderer, ccurve const& c) const;

    void draw_box(render::system* renderer, ccurve const& c, float t, vec2 size, color4 color) const;
    void draw_graph(render::system* renderer, std::vector<float> const& data, color4 color) const;
    void draw_graph(render::system* renderer, std::vector<vec2> const& data, color4 color) const;

    void draw_section(render::system* renderer, clothoid::segment const& s) const;

    void on_click();
    void on_rclick();

    void calculate_qspline_points(qcurve& q) const;
    void calculate_cspline_points(ccurve& c) const;

    void calculate_section(clothoid::segment& s) const;
    void calculate_section_offset(clothoid::segment const& s, clothoid::segment& o, float offset) const;
    std::vector<clothoid::segment> connect_sections(clothoid::segment const& s1, clothoid::segment const& s2) const;

    void linearize_max_speed(ccurve const& c, std::vector<vec2>& max_speed) const;
    void linearize_max_speed_r(ccurve_coeff const& cc, std::vector<vec2>& max_speed, float t0, float t1, float s0, float s1, float k0, float k1) const;

    float calculate_max_speed(float s) const;

protected:
    static constexpr float kMaxSpeed = 10.f;
    static constexpr float kMaxLateral = 10.f;
    static constexpr float kMaxAccel = 1.f;
    static constexpr float kMaxDecel = 1.f;
};

} // namespace editor

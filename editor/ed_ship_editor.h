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
    mutable float _anim;
    mutable time_value _last_time;

    struct qcurve {
        vec2 a, b, c;
    };

    struct ccurve_coeff {
        vec2 a; //   P0
        vec2 b; // -3P0 + 3P1
        vec2 c; //  3P0 - 6P1 + 3P2
        vec2 d; //  -P0 + 3P1 - 3P2 + P3

        static ccurve_coeff from_bezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3) {
            return {
                p0,
                -3 * p0 + 3 * p1,
                3 * p0 - 6 * p1 + 3 * p2,
                -p0 + 3 * p1 - 3 * p2 + p3,
            };
        }

        vec2 evaluate(float t) const {
            // P(t) = a + bt + ct^2 + dt^3
            return ((d * t + c) * t + b) * t + a;
        }

        vec2 evaluate_tangent(float t) const {
            // P'(t) = b + 2ct + 3dt^2
            return (3 * d * t + 2 * c) * t + b;
        }

        float evaluate_curvature(float t) const {
            // P"(t) = 2c + 6dt
            vec2 uu = 6 * d * t + 2 * c;
            vec2 u = evaluate_tangent(t);
            return cross(u, uu) / (length(u) * length_sqr(u));
        }
    };

    struct ccurve {
        vec2 a, b, c, d;
        ccurve_coeff to_coeff() const {
            return ccurve_coeff::from_bezier(a, b, c, d);
        }
    };

    enum class state {
        ready,
        started,
    } _state;

    vec2 _start;

    std::vector<qcurve> _qspline;
    std::vector<ccurve> _cspline;

protected:
    vec2 cursor_to_world() const;

    void draw_qspline(render::system* renderer, qcurve const& q) const;
    void draw_cspline(render::system* renderer, ccurve const& c) const;

    void on_click();

    void calculate_qspline_points(qcurve& q) const;
    void calculate_cspline_points(ccurve& c) const;
};

} // namespace editor

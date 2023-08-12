// ed_ship_editor.h
//

#pragma once

#include "r_main.h"
#include "cm_curve.h"

////////////////////////////////////////////////////////////////////////////////
namespace render {
class image;
} // namespace render

namespace {

/*************************************************************************
Fresnel integral

Evaluates the Fresnel integrals

          x
          -
         | |
C(x) =   |   cos(pi/2 t**2) dt,
       | |
        -
         0

          x
          -
         | |
S(x) =   |   sin(pi/2 t**2) dt.
       | |
        -
         0


The integrals are evaluated by a power series for x < 1.
For x >= 1 auxiliary functions f(x) and g(x) are employed
such that

C(x) = 0.5 + f(x) sin( pi/2 x**2 ) - g(x) cos( pi/2 x**2 )
S(x) = 0.5 - f(x) cos( pi/2 x**2 ) - g(x) sin( pi/2 x**2 )



ACCURACY:

 Relative error.

Arithmetic  function   domain     # trials      peak         rms
  IEEE       S(x)      0, 10       10000       2.0e-15     3.2e-16
  IEEE       C(x)      0, 10       10000       1.8e-15     3.3e-16

Cephes Math Library Release 2.8:  June, 2000
Copyright 1984, 1987, 1989, 2000 by Stephen L. Moshier
*************************************************************************/
void fresnelintegral(double x, double* c, double* s/*, ae_state* _state*/)
{
#define ae_fabs(a, b) fabs(a)
#define ae_fp_less(a, b) (a < b)
#define ae_fp_greater(a, b) (a > b)
#define ae_sin(a, b) sin(a)
#define ae_cos(a, b) cos(a)
#define ae_sign(a, b) (a < 0. ? -1. : 1.)
    double xxa;
    double f;
    double g;
    double cc;
    double ss;
    double t;
    double u;
    double x2;
    double sn;
    double sd;
    double cn;
    double cd;
    double fn;
    double fd;
    double gn;
    double gd;
    double mpi;
    double mpio2;


    mpi = 3.14159265358979323846;
    mpio2 = 1.57079632679489661923;
    xxa = x;
    x = ae_fabs(xxa, _state);
    x2 = x * x;
    if (ae_fp_less(x2, 2.5625))
    {
        t = x2 * x2;
        sn = -2.99181919401019853726E3;
        sn = sn * t + 7.08840045257738576863E5;
        sn = sn * t - 6.29741486205862506537E7;
        sn = sn * t + 2.54890880573376359104E9;
        sn = sn * t - 4.42979518059697779103E10;
        sn = sn * t + 3.18016297876567817986E11;
        sd = 1.00000000000000000000E0;
        sd = sd * t + 2.81376268889994315696E2;
        sd = sd * t + 4.55847810806532581675E4;
        sd = sd * t + 5.17343888770096400730E6;
        sd = sd * t + 4.19320245898111231129E8;
        sd = sd * t + 2.24411795645340920940E10;
        sd = sd * t + 6.07366389490084639049E11;
        cn = -4.98843114573573548651E-8;
        cn = cn * t + 9.50428062829859605134E-6;
        cn = cn * t - 6.45191435683965050962E-4;
        cn = cn * t + 1.88843319396703850064E-2;
        cn = cn * t - 2.05525900955013891793E-1;
        cn = cn * t + 9.99999999999999998822E-1;
        cd = 3.99982968972495980367E-12;
        cd = cd * t + 9.15439215774657478799E-10;
        cd = cd * t + 1.25001862479598821474E-7;
        cd = cd * t + 1.22262789024179030997E-5;
        cd = cd * t + 8.68029542941784300606E-4;
        cd = cd * t + 4.12142090722199792936E-2;
        cd = cd * t + 1.00000000000000000118E0;
        *s = (double)ae_sign(xxa, _state) * x * x2 * sn / sd;
        *c = (double)ae_sign(xxa, _state) * x * cn / cd;
        return;
    }
    if (ae_fp_greater(x, 36974.0))
    {
        *c = (double)ae_sign(xxa, _state) * 0.5;
        *s = (double)ae_sign(xxa, _state) * 0.5;
        return;
    }
    x2 = x * x;
    t = mpi * x2;
    u = (double)1 / (t * t);
    t = (double)1 / t;
    fn = 4.21543555043677546506E-1;
    fn = fn * u + 1.43407919780758885261E-1;
    fn = fn * u + 1.15220955073585758835E-2;
    fn = fn * u + 3.45017939782574027900E-4;
    fn = fn * u + 4.63613749287867322088E-6;
    fn = fn * u + 3.05568983790257605827E-8;
    fn = fn * u + 1.02304514164907233465E-10;
    fn = fn * u + 1.72010743268161828879E-13;
    fn = fn * u + 1.34283276233062758925E-16;
    fn = fn * u + 3.76329711269987889006E-20;
    fd = 1.00000000000000000000E0;
    fd = fd * u + 7.51586398353378947175E-1;
    fd = fd * u + 1.16888925859191382142E-1;
    fd = fd * u + 6.44051526508858611005E-3;
    fd = fd * u + 1.55934409164153020873E-4;
    fd = fd * u + 1.84627567348930545870E-6;
    fd = fd * u + 1.12699224763999035261E-8;
    fd = fd * u + 3.60140029589371370404E-11;
    fd = fd * u + 5.88754533621578410010E-14;
    fd = fd * u + 4.52001434074129701496E-17;
    fd = fd * u + 1.25443237090011264384E-20;
    gn = 5.04442073643383265887E-1;
    gn = gn * u + 1.97102833525523411709E-1;
    gn = gn * u + 1.87648584092575249293E-2;
    gn = gn * u + 6.84079380915393090172E-4;
    gn = gn * u + 1.15138826111884280931E-5;
    gn = gn * u + 9.82852443688422223854E-8;
    gn = gn * u + 4.45344415861750144738E-10;
    gn = gn * u + 1.08268041139020870318E-12;
    gn = gn * u + 1.37555460633261799868E-15;
    gn = gn * u + 8.36354435630677421531E-19;
    gn = gn * u + 1.86958710162783235106E-22;
    gd = 1.00000000000000000000E0;
    gd = gd * u + 1.47495759925128324529E0;
    gd = gd * u + 3.37748989120019970451E-1;
    gd = gd * u + 2.53603741420338795122E-2;
    gd = gd * u + 8.14679107184306179049E-4;
    gd = gd * u + 1.27545075667729118702E-5;
    gd = gd * u + 1.04314589657571990585E-7;
    gd = gd * u + 4.60680728146520428211E-10;
    gd = gd * u + 1.10273215066240270757E-12;
    gd = gd * u + 1.38796531259578871258E-15;
    gd = gd * u + 8.39158816283118707363E-19;
    gd = gd * u + 1.86958710162783236342E-22;
    f = (double)1 - u * fn / fd;
    g = t * gn / gd;
    t = mpio2 * x2;
    cc = ae_cos(t, _state);
    ss = ae_sin(t, _state);
    t = mpi * x;
    *c = 0.5 + (f * ss - g * cc) / t;
    *s = 0.5 - (f * cc + g * ss) / t;
    *c = *c * (double)ae_sign(xxa, _state);
    *s = *s * (double)ae_sign(xxa, _state);
}
}

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

    float _beta1;
    float _beta2;

    std::vector<qcurve> _qspline;
    std::vector<ccurve> _cspline;
    std::vector<float> _cspline_length;

    std::vector<vec2> _cspline_max_speed;

    enum class section_type {
        line,
        arc,
        clothoid,
    };
    struct section {
        section_type type;
        vec2 p0; //!< initial position
        vec2 t0; //!< initial tangent
        float L; //!< length
        // arc
        float k0; //!< initial curvature
        // clothoid
        float k1; //!< final curvature

        vec2 evaluate(float s) const {
            switch (type) {
                case section_type::line:
                    return p0 + s * t0;

                case section_type::arc: {
                    vec2 n0 = t0.cross(1);
                    return p0 + (n0 * (1.f - cos(s * k0)) + t0 * sin(s * k0)) / k0;
                };

                case section_type::clothoid: {
                    float B = sqrt(L / (math::pi<float> * abs(k1 - k0)));
                    float t = B * ((k1 - k0) * s / L + k0);
                    float tk0 = B * k0;
                    vec2 t00 = t0 * copysign(1.f, k1 - k0);
                    vec2 n0 = t0.cross(-1.f);
                    vec2 ti, ni, pi;
                    if (k0) {
                        float c0, s0;
                        c0 = cos(.5f * math::pi<float> * tk0 * tk0);
                        s0 = sin(.5f * math::pi<float> * tk0 * tk0);
                        ti = t00 * c0 - n0 * s0;
                        ni = n0 * c0 + t00 * s0;
                        fresnel_integral(tk0, c0, s0);
                        pi = p0 - math::pi<float> * B * (ti * c0 + ni * s0);
                    } else {
                        ti = t00;
                        ni = n0;
                        pi = p0;
                    }
                    float ci, si;
                    fresnel_integral(t, ci, si);
                    return pi + math::pi<float> * B * (ti * ci + ni * si);
                }
            }
            __assume(false);
        }

        vec2 evaluate_tangent(float s) const {
            switch (type) {
                case section_type::line:
                    return t0;

                case section_type::arc: {
                    vec2 n0 = t0.cross(1);
                    return t0 * cos(s * k0) + n0 * sin(s * k0);
                }

                case section_type::clothoid: {
                    float B = sqrt(L / (math::pi<float> * abs(k1 - k0)));
                    float t = B * ((k1 - k0) * s / L + k0);
                    float tk0 = B * k0;
                    vec2 t00 = t0 * copysign(1.f, k1 - k0);
                    vec2 n0 = t00.cross(-1.f);
                    vec2 ti, ni;
                    if (k0) {
                        float c0, s0;
                        c0 = cos(.5f * math::pi<float> * tk0 * tk0);
                        s0 = sin(.5f * math::pi<float> * tk0 * tk0);
                        ti = t0 * c0 - n0 * s0;
                        ni = n0 * c0 + t0 * s0;
                    } else {
                        ti = t0;
                        ni = n0;
                    }
                    return ti * cos(.5f * math::pi<float> * t * t) + ni * sin(.5f * math::pi<float> * t * t);
                }
            }
            __assume(false);
        }

        void fresnel_integral(float t, float&c, float& s) const {
            double C, S;
            fresnelintegral(t, &C, &S);
            c = float(C);
            s = float(S);
        }
    };

protected:
    vec2 cursor_to_world() const;

    void draw_qspline(render::system* renderer, qcurve const& q) const;
    void draw_cspline(render::system* renderer, ccurve const& c) const;

    void draw_box(render::system* renderer, ccurve const& c, float t, vec2 size, color4 color) const;
    void draw_graph(render::system* renderer, std::vector<float> const& data, color4 color) const;
    void draw_graph(render::system* renderer, std::vector<vec2> const& data, color4 color) const;

    void draw_section(render::system* renderer, section const& s) const;

    void on_click();
    void on_rclick();

    void calculate_qspline_points(qcurve& q) const;
    void calculate_cspline_points(ccurve& c) const;

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

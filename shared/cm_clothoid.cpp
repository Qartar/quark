// cm_clothoid.cpp
//

#include "cm_clothoid.h"
#include "cm_shared.h"

#include <immintrin.h>

////////////////////////////////////////////////////////////////////////////////
namespace clothoid {

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
void segment::fresnel_integral(double x, double* c, double* s)
{
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
    x = fabs(xxa);
    x2 = x * x;
    if (x2 < 2.5625)
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
        *s = copysign(x * x2 * sn / sd, xxa);
        *c = copysign(x * cn / cd, xxa);
        return;
    }
    if (x > 36974.0)
    {
        *c = copysign(0.5, xxa);
        *s = copysign(0.5, xxa);
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
    cc = cos(t);
    ss = sin(t);
    t = mpi * x;
    *c = copysign(0.5 + (f * ss - g * cc) / t, xxa);
    *s = copysign(0.5 - (f * cc + g * ss) / t, xxa);
}

//------------------------------------------------------------------------------
void segment::fresnel_integral_simd(double x, double* c, double* s)
{
    double x2 = x * x;
    if (x > 36974.0) {
        *c = copysign(0.5, x);
        *s = copysign(0.5, x);
    } else if (x2 < 2.5625f) {
        __m256d t = _mm256_set1_pd(x2 * x2);

        // r = [sn, cn, sd, cd]
        __m256d r = _mm256_set_pd(0, 0, 1.00000000000000000000E0, 3.99982968972495980367E-12);
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd(-2.99181919401019853726E3,  -4.98843114573573548651E-8, 2.81376268889994315696E2,  9.15439215774657478799E-10));
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd( 7.08840045257738576863E5,   9.50428062829859605134E-6, 4.55847810806532581675E4,  1.25001862479598821474E-7));
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd(-6.29741486205862506537E7,  -6.45191435683965050962E-4, 5.17343888770096400730E6,  1.22262789024179030997E-5));
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd( 2.54890880573376359104E9,   1.88843319396703850064E-2, 4.19320245898111231129E8,  8.68029542941784300606E-4));
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd(-4.42979518059697779103E10, -2.05525900955013891793E-1, 2.24411795645340920940E10, 4.12142090722199792936E-2));
        r = _mm256_fmadd_pd(r, t, _mm256_set_pd( 3.18016297876567817986E11,  9.99999999999999998822E-1, 6.07366389490084639049E11, 1.00000000000000000118E0));

        // q = [sn, cn]
        __m128d q = _mm256_extractf128_pd(r, 1);
        // q = [sn / sd, cn / cd]
        q = _mm_div_pd(q, _mm256_castpd256_pd128(r));
        q = _mm_mul_pd(q, _mm_set1_pd(x));

        // copysign
        const __m128d signbit = _mm_set1_pd(-0.0);
        q = _mm_or_pd(_mm_andnot_pd(signbit, q), _mm_and_pd(signbit, _mm_set1_pd(x)));

        _mm_storel_pd(c, q);
        q = _mm_mul_pd(q, _mm_set1_pd(x2));
        _mm_storeh_pd(s, q);
    } else {
        __m256d t = _mm256_div_pd(_mm256_set1_pd(1.0 / 3.14159265358979323846), _mm256_set1_pd(x2));
        __m256d u = _mm256_mul_pd(t, t);

        // r = [gn, fn, gd, fd]
        __m256d r = _mm256_set_pd(0, 0, 1.00000000000000000000E0, 0);
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(5.04442073643383265887E-1,  0,                           1.47495759925128324529E0,   1.00000000000000000000E0));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.97102833525523411709E-1,  4.21543555043677546506E-1,   3.37748989120019970451E-1,  7.51586398353378947175E-1));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.87648584092575249293E-2,  1.43407919780758885261E-1,   2.53603741420338795122E-2,  1.16888925859191382142E-1));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(6.84079380915393090172E-4,  1.15220955073585758835E-2,   8.14679107184306179049E-4,  6.44051526508858611005E-3));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.15138826111884280931E-5,  3.45017939782574027900E-4,   1.27545075667729118702E-5,  1.55934409164153020873E-4));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(9.82852443688422223854E-8,  4.63613749287867322088E-6,   1.04314589657571990585E-7,  1.84627567348930545870E-6));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(4.45344415861750144738E-10, 3.05568983790257605827E-8,   4.60680728146520428211E-10, 1.12699224763999035261E-8));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.08268041139020870318E-12, 1.02304514164907233465E-10,  1.10273215066240270757E-12, 3.60140029589371370404E-11));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.37555460633261799868E-15, 1.72010743268161828879E-13,  1.38796531259578871258E-15, 5.88754533621578410010E-14));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(8.36354435630677421531E-19, 1.34283276233062758925E-16,  8.39158816283118707363E-19, 4.52001434074129701496E-17));
        r = _mm256_fmadd_pd(r, u, _mm256_set_pd(1.86958710162783235106E-22, 3.76329711269987889006E-20,  1.86958710162783236342E-22, 1.25443237090011264384E-20));

        // q = [gn, fn]
        __m128d q = _mm256_extractf128_pd(r, 1);
        // q = [gn / gd, fn / fd]
        q = _mm_div_pd(q, _mm256_castpd256_pd128(r));

        // q = [gn / gd, t fn / fd]
        q = _mm_mul_sd(q, _mm256_castpd256_pd128(t));
        // q = [t gn / gd, u fn / fd]
        q = _mm_mul_pd(q, _mm256_castpd256_pd128(t));
        // q = [t gn / gd, -u fn / fd]
        q = _mm_xor_pd(q, _mm_set_pd(0.0, -0.0));
        // q = [t gn / gd, 1 - u fn / fd]
        q = _mm_add_pd(q, _mm_set_pd(0.0, 1.0));

        __m128d t2 = _mm_mul_pd(_mm_set1_pd(1.57079632679489661923), _mm_set1_pd(x2));
        __m128d cc, ss = _mm_sincos_pd(&cc, t2);

        // xx = [-g * ss, f * ss]
        __m128d xx = _mm_xor_pd(_mm_mul_pd(q, ss), _mm_set_pd(-0.0, 0.0));
        // yy = [-g * cc, -f * cc]
        __m128d yy = _mm_xor_pd(_mm_mul_pd(q, cc), _mm_set_pd(-0.0, -0.0));
        // q = [ -f * cc - g * ss, f * ss - g * cc]
        q = _mm_add_pd(xx, _mm_shuffle_pd(yy, yy, 1));

        const __m128d signbit = _mm_set1_pd(-0.0);
        __m128d t3 = _mm_mul_pd(_mm_set1_pd(3.14159265358979323846), _mm_andnot_pd(signbit, _mm_set1_pd(x)));
        q = _mm_div_pd(q, t3);
        q = _mm_add_pd(q, _mm_set1_pd(0.5));

        // copysign
        q = _mm_or_pd(_mm_andnot_pd(signbit, q), _mm_and_pd(signbit, _mm_set1_pd(x)));

        _mm_storel_pd(c, q);
        _mm_storeh_pd(s, q);
    }
}

//------------------------------------------------------------------------------
vec3 segment::get_closest_point(vec2 p) const
{
    switch (type()) {
        case segment_type::line: {
            vec2 r = p - _initial_position;
            vec2 v = _initial_tangent;

            float s = min(max(dot(r, v), 0.f), _length);
            return vec3(_initial_position + v * s, s);
        }

        case segment_type::arc: {
            float radius = 1.f / _initial_curvature;
            vec2 center = _initial_position - _initial_tangent.cross(radius);
            vec2 final_pos = final_position();
            vec2 dir = p - center;

            vec2 initial_edge_dir = _initial_position - center;
            vec2 final_edge_dir = final_pos - center;
            float initial_edge_dist = dot(dir, initial_edge_dir.cross(-1));
            float final_edge_dist = dot(dir, final_edge_dir.cross(-1));

            if (radius < 0) {
                initial_edge_dist = -initial_edge_dist;
                final_edge_dist = -final_edge_dist;
            }

            if (initial_edge_dist < 0 && final_edge_dist > 0) {
                if (dot(dir, final_pos - _initial_position) < 0) {
                    return vec3(_initial_position, 0);
                } else {
                    return vec3(final_pos, _length);
                }
            } else if (initial_edge_dist < 0) {
                return vec3(_initial_position, 0);
            } else if (final_edge_dist > 0) {
                return vec3(final_pos, _length);
            } else {
                vec2 v = dir.normalize() * abs(radius);
                float theta = atan2(initial_edge_dist, dot(dir, initial_edge_dir));
                return vec3(center + v, theta * abs(radius));
            }
        }

        case segment_type::transition: {
            float s = dot(p - _initial_position, _initial_tangent);
            for (int ii = 0; ii < 32; ++ii) {
                vec2 r = evaluate(s);
                vec2 t = evaluate_tangent(s);
                float ds = dot(p - r, t);
                if (s > _length && ds > 0) {
                    return vec3(final_position(), _length);
                } else if (s < 0 && ds < 0) {
                    return vec3(_initial_position, 0);
                } else if (abs(ds) < 1e-6f) {
                    break;
                }
                s += ds;
            }
            return vec3(evaluate(s), s);
        }

        default:
            __assume(false);
    }
}

//------------------------------------------------------------------------------
inline float point_line_distance_sqr(vec2 a, vec2 b, vec2 c)
{
    return square(cross(c - a, b - a)) / length_sqr(b - a);
}

//------------------------------------------------------------------------------
bool intersect_line_triangle(vec2 a, vec2 b, vec2 c, vec2 d, vec2 e)
{
    float abc = cross(b - a, c - a);
    if (cross(b - a, d - a) * abc < 0
        && cross(b - a, e - a) * abc < 0) {
        return false;
    }

    if (cross(c - b, d - b) * abc < 0
        && cross(c - b, e - b) * abc < 0) {
        return false;
    }

    if (cross(a - c, d - c) * abc < 0
        && cross(a - c, e - c) * abc < 0) {
        return false;
    }

    float de = cross(c - d, e - d);
    if (cross(a - d, e - d) * de > 0
        && cross(b - d, e - d) * de > 0) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool intersect_segments(vec2 a, vec2 b, vec2 d, vec2 e, float& s1, float& s2)
{
    s1 = cross(d - a, e - d) / cross(b - a, e - d);
    s2 = cross(a - d, b - a) / cross(e - d, b - a);

    if (s1 < 0 || s1 > 1 || s2 < 0 || s2 > 1) {
        return false;
    }

    s1 *= (b - a).length();
    s2 *= (e - d).length();
    return true;
}

//------------------------------------------------------------------------------
bool intersect_segments_r(segment const& l1, vec2 d, vec2 e, float& s1, float& s2)
{
    vec2 a, b, c;
    l1.bounding_triangle(a, b, c);
    float d1 = point_line_distance_sqr(a, b, c);
    if (d1 < 1e-6f) {
        // line-line intersection test
        return intersect_segments(a, b, d, e, s1, s2);
    }

    if (!intersect_line_triangle(a, b, c, d, e)) {
        return false;
    }

    segment l11, l12;
    l1.split(.5f * l1.length(), l11, l12);
    if (intersect_segments_r(l11, d, e, s1, s2)) {
        return true;
    } else if (intersect_segments_r(l12, d, e, s1, s2)) {
        s1 += l11.length();
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool intersect_segments_r(vec2 a, vec2 b, segment const& l2, float& s1, float& s2)
{
    vec2 d, e, f;
    l2.bounding_triangle(d, e, f);
    float d2 = point_line_distance_sqr(d, e, f);
    if (d2 < 1e-6f) {
        // line-line intersection test
        return intersect_segments(a, b, d, e, s1, s2);
    }

    if (!intersect_line_triangle(d, e, f, a, b)) {
        return false;
    }

    segment l21, l22;
    l2.split(.5f * l2.length(), l21, l22);
    if (intersect_segments_r(a, b, l21, s1, s2)) {
        float s12, s22;
        if (intersect_segments_r(a, b, l22, s12, s22) && s1 > s12) {
            s1 = s12;
            s2 = s22 + l21.length();
            return true;
        }
        return true;
    } else if (intersect_segments_r(a, b, l22, s1, s2)) {
        s2 += l21.length();
        return true;
    } else {
        return false;
    }
}

//------------------------------------------------------------------------------
bool intersect_triangles(vec2 a, vec2 b, vec2 c, vec2 d, vec2 e, vec2 f)
{
    float abc = cross(b - a, c - a);
    if (cross(b - a, d - a) * abc < 0
        && cross(b - a, e - a) * abc < 0
        && cross(b - a, f - a) * abc < 0) {
        return false;
    }

    if (cross(c - b, d - b) * abc < 0
        && cross(c - b, e - b) * abc < 0
        && cross(c - b, f - b) * abc < 0) {
        return false;
    }

    if (cross(a - c, d - c) * abc < 0
        && cross(a - c, e - c) * abc < 0
        && cross(a - c, f - c) * abc < 0) {
        return false;
    }

    float def = cross(e - d, f - d);
    if (cross(e - d, a - d) * def < 0
        && cross(e - d, b - d) * def < 0
        && cross(e - d, c - d) * def < 0) {
        return false;
    }

    if (cross(f - e, a - e) * def < 0
        && cross(f - e, b - e) * def < 0
        && cross(f - e, c - e) * def < 0) {
        return false;
    }

    if (cross(d - f, a - f) * def < 0
        && cross(d - f, b - f) * def < 0
        && cross(d - f, c - f) * def < 0) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool intersect_segments_r(segment const& l1, segment const& l2, float& s1, float& s2)
{
    vec2 a, b, c, d, e, f;
    l1.bounding_triangle(a, b, c);
    l2.bounding_triangle(d, e, f);

    float d1 = point_line_distance_sqr(a, b, c);
    float d2 = point_line_distance_sqr(d, e, f);
    if (d1 < 1e-6f && d2 < 1e-6f) {
        // line-line intersection test
        return intersect_segments(a, b, d, e, s1, s2);
    } else if (d1 < 1e-6f) {
        // line-segment intersection test
        return intersect_segments_r(l1, d, e, s1, s2);
    } else if (d2 < 1e-6f) {
        // segment-line intersection test
        return intersect_segments_r(a, b, l2, s1, s2);
    }

    if (!intersect_triangles(a, b, c, d, e, f)) {
        return false;
    }

    segment l11, l12;
    l1.split(.5f * l1.length(), l11, l12);
    l11.bounding_triangle(a, b, c);
    if (intersect_triangles(a, b, c, d, e, f)) {
        segment l21, l22;
        l2.split(.5f * l2.length(), l21, l22);
        if (intersect_segments_r(l11, l21, s1, s2)) {
            return true;
        } else if (intersect_segments_r(l11, l22, s1, s2)) {
            s2 += l21.length();
            return true;
        }
    }

    l12.bounding_triangle(a, b, c);
    if (intersect_triangles(a, b, c, d, e, f)) {
        segment l21, l22;
        l2.split(.5f * l2.length(), l21, l22);
        if (intersect_segments_r(l12, l21, s1, s2)) {
            s1 += l11.length();
            return true;
        } else if (intersect_segments_r(l12, l22, s1, s2)) {
            s1 += l11.length();
            s2 += l21.length();
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool segment::intersect(segment const& other, float& s, float& t) const
{
    return intersect_segments_r(*this, other, s, t);
}

} // namespace clothoid

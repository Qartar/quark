// g_ship_editor.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"
#include "g_ship_editor.h"
#include "r_image.h"
#include "r_window.h"

#include "cm_filesystem.h"

#define NOMINMAX
#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
ship_outline::ship_outline(std::vector<vec2>&& vertices, std::vector<segment_type>&& segments)
    : _vertices(std::move(vertices))
    , _segments(std::move(segments))
{
}

//------------------------------------------------------------------------------
void ship_outline::draw(render::system* renderer, mat3 transform, vec2 vertex_size) const
{
    for (size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            vec2 a = _vertices[jj + 0] * transform;
            vec2 b = _vertices[jj + 1] * transform;
            renderer->draw_line(a, b, color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,0,0,1));
            jj += 1;
        } else if (_segments[ii] == quad) {
            vec2 a = _vertices[jj + 0] * transform;
            vec2 b = _vertices[jj + 1] * transform;
            vec2 c = _vertices[jj + 2] * transform;
            draw_bezier_quad(renderer, a, b, c, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,0,0,1));
            jj += 2;
        } else if (_segments[ii] == cube) {
            vec2 a = _vertices[jj + 0] * transform;
            vec2 b = _vertices[jj + 1] * transform;
            vec2 c = _vertices[jj + 2] * transform;
            vec2 d = _vertices[jj + 3] * transform;
            draw_bezier_cube(renderer, a, b, c, d, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,1,0,1));
            renderer->draw_box(vertex_size, d, color4(1,0,0,1));
            jj += 3;
        }
    }

    mat3 mirrored = mat3(1,0,0,0,-1,0,0,0,1) * transform;

    if (_vertices.front().y) {
        vec2 a = _vertices.front() * transform;
        vec2 b = _vertices.front() * mirrored;
        renderer->draw_line(a, b, color4(1,1,1,1), color4(1,1,1,1));
    }

    if (_vertices.back().y) {
        vec2 a = _vertices.back() * transform;
        vec2 b = _vertices.back() * mirrored;
        renderer->draw_line(a, b, color4(1,1,1,1), color4(1,1,1,1));
    }

    for (size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            vec2 a = _vertices[jj + 0] * mirrored;
            vec2 b = _vertices[jj + 1] * mirrored;
            renderer->draw_line(a, b, color4(1,1,1,1), color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,0,0,1));
            jj += 1;
        } else if (_segments[ii] == quad) {
            vec2 a = _vertices[jj + 0] * mirrored;
            vec2 b = _vertices[jj + 1] * mirrored;
            vec2 c = _vertices[jj + 2] * mirrored;
            draw_bezier_quad(renderer, a, b, c, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,0,0,1));
            jj += 2;
        } else if (_segments[ii] == cube) {
            vec2 a = _vertices[jj + 0] * mirrored;
            vec2 b = _vertices[jj + 1] * mirrored;
            vec2 c = _vertices[jj + 2] * mirrored;
            vec2 d = _vertices[jj + 3] * mirrored;
            draw_bezier_cube(renderer, a, b, c, d, color4(1,1,1,1));
            renderer->draw_box(vertex_size, a, color4(1,0,0,1));
            renderer->draw_box(vertex_size, b, color4(1,1,0,1));
            renderer->draw_box(vertex_size, c, color4(1,1,0,1));
            renderer->draw_box(vertex_size, d, color4(1,0,0,1));
            jj += 3;
        }
    }
}

//------------------------------------------------------------------------------
void ship_outline::draw_linearized(render::system* renderer, mat3 transform, vec2 vertex_size) const
{
    if (!_linearized.size()) {
        return;
    }

    vec2 v0 = _linearized[0] * transform;
    for (size_t ii = 0; ii + 1 < _linearized.size(); ++ii) {
        vec2 v1 = _linearized[ii + 1] * transform;
        renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_box(vertex_size, v0, color4(1,0,0,1));
        v0 = v1;
    }
    vec2 v1 = _linearized[0] * transform;
    renderer->draw_line(v0, v1, color4(1,1,1,1), color4(1,1,1,1));
    renderer->draw_box(vertex_size, v0, color4(1,0,0,1));
}

//------------------------------------------------------------------------------
void ship_outline::draw_bezier_quad(render::system* renderer, vec2 a, vec2 b, vec2 c, color4 color) const
{
    vec2 v0 = a;
    for (int ii = 1; ii < 32; ++ii) {
        double t = ii / 32.0;
        vec2 v1 = (1-t)*(1-t)*a + 2*(1-t)*t*b + t*t*c;
        renderer->draw_line(v0, v1, color, color);
        v0 = v1;
    }
    renderer->draw_line(v0, c, color, color);
}

//------------------------------------------------------------------------------
void ship_outline::draw_bezier_cube(render::system* renderer, vec2 a, vec2 b, vec2 c, vec2 d, color4 color) const
{
    vec2 v0 = a;
    for (int ii = 1; ii < 32; ++ii) {
        double t = ii / 32.0;
        double t2 = t * t;
        double s = 1.0 - t;
        double s2 = s * s;
        vec2 v1 = s2 * s * a + 3.0 * s2 * t * b + 3.0 * s * t2 * c + t * t2 * d;
        renderer->draw_line(v0, v1, color, color);
        v0 = v1;
    }
    renderer->draw_line(v0, d, color, color);
}

//------------------------------------------------------------------------------
double segment_closest_point(vec2 a, vec2 b, vec2 p)
{
    vec2 v = b - a;
    double num = dot(p - a, v);
    double den = dot(v, v);
    if (num >= den) {
        return 1.0;
    } else if (num <= 0.0) {
        return 0.0;
    } else {
        return num / den;
    }
}

//------------------------------------------------------------------------------
// https://www.shadertoy.com/view/MlKcDD
// Copyright © 2018 Inigo Quilez
double quad_closest_point(vec2 A, vec2 B, vec2 C, vec2 pos)
{
    vec2 a = B - A;
    vec2 b = A - 2.0 * B + C;
    vec2 c = a * 2.0;
    vec2 d = A - pos;

    double kk = 1.0 / dot(b, b);
    double kx = kk * dot(a, b);
    double ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0;
    double kz = kk * dot(d, a);

    double res = 0.0;
    double sgn = 0.0;
    double t = 0.0;

    double p = ky - kx * kx;
    double p3 = p * p * p;
    double q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
    double h = q * q + 4.0 * p3;

    if (h >= 0.0) {
        h = std::sqrt(h);
        vec2 x = (vec2(h, -h) - vec2(q)) / 2.0;
        vec2 uv = vec2(std::cbrt(x.x), std::cbrt(x.y));
        t = clamp(uv.x + uv.y - kx, 0.0, 1.0);
        vec2 qx = d + (c + b * t) * t;
        res = dot(qx, qx);
        sgn = cross(c + 2.0 * b * t, qx);
    } else {
        double z = std::sqrt(-p);
        double v = std::acos(q / (p * z * 2.0)) / 3.0;
        double m = std::cos(v);
        double n = std::sin(v) * 1.732050808; // sqrt(3)

        double tx = clamp((m + m) * z - kx, 0.0, 1.0);
        vec2 qx = d + (c + b * tx) * tx;
        double dx = dot(qx, qx);
        double sx = cross(c + 2.0 * b * tx, qx);

        double ty = clamp((-n - m) * z - kx, 0.0, 1.0);
        vec2 qy = d + (c + b * ty) * ty;
        double dy = dot(qy, qy);
        double sy = cross(c + 2.0 * b * ty, qy);

        // the third root cannot be the closest
        //float tz = clamp((n - m) * z - kx, 0.f, 1.f);
        //  ...

        res = (dx < dy) ? dx : dy;
        sgn = (dx < dy) ? sx : sy;
        t = (dx < dy) ? tx : ty;
    }

    return t;
    //return (1-t)*(1-t)*A + 2*(1-t)*t*B + t*t*C;
/*
    if (t == 0.f) {
        float den = dot(B - A, B - A);
        float det = cross(B - A, pos - A);
        return {std::copysign(res, -sgn), det * det / den};
    } else if (t == 1.f) {
        float den = dot(C - B, C - B);
        float det = cross(C - B, pos - C);
        return {std::copysign(res, -sgn), det * det / den};
    } else {
        return {std::copysign(res, -sgn), res};
    }
*/
}

//------------------------------------------------------------------------------
double cube_closest_point(vec2 A, vec2 B, vec2 C, vec2 D, vec2 pos)
{
    // Calculate standard form coefficients for faster evaluation
    vec2 a = -A + 3.0 * B - 3.0 * C + D;
    vec2 b = 3.0 * A - 6.0 * B + 3.0 * C;
    vec2 c = -3.0 * A + 3.0 * B;
    vec2 d = A;

    double t[5] = { 0, 0.25, 0.5, 0.75, 1.0 };
    vec2 p[5] = {
        A,
        ((a * 0.25 + b) * 0.25 + c) * 0.25 + d,
        ((a * 0.50 + b) * 0.50 + c) * 0.50 + d,
        ((a * 0.75 + b) * 0.75 + c) * 0.75 + d,
        D,
    };

    for (std::size_t ii = 0; ii < 32; ++ii) {
        std::size_t best_idx = 0;
        double best_dsqr = length_sqr(p[0] - pos);
        for (std::size_t jj = 1; jj < 5; ++jj) {
            double dsqr = length_sqr(p[jj] - pos);
            if (dsqr < best_dsqr) {
                best_idx = jj;
                best_dsqr = dsqr;
            }
        }

        if (best_dsqr < square(1e-4)) {
            return t[best_idx];
        }

        // Reframe search to points above and below closest point
        best_idx = clamp(best_idx, 1, 3);
        double tmin = t[best_idx - 1];
        double tmax = t[best_idx + 1];

        t[0] = tmin;
        t[1] = tmin + (tmax - tmin) * 0.25;
        t[2] = tmin + (tmax - tmin) * 0.5;
        t[3] = tmin + (tmax - tmin) * 0.75;
        t[4] = tmax;

        p[0] = p[best_idx - 1];
        for (std::size_t jj = 1; jj < 4; ++jj) {
            p[jj] = ((a * t[jj] + b) * t[jj] + c) * t[jj] + d;
        }
        p[4] = p[best_idx + 1];
    }

    return t[2];
}

//------------------------------------------------------------------------------
std::size_t ship_outline::closest_vertex(vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    double best_dsqr = DBL_MAX;
    for (std::size_t ii = 0; ii < _vertices.size(); ++ii) {
        double dsqr = length_sqr(_vertices[ii] - v);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }
    return best_idx;
}

//------------------------------------------------------------------------------
std::size_t ship_outline::closest_segment(vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    double best_dsqr = DBL_MAX;
    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            double t = segment_closest_point(_vertices[jj + 0], _vertices[jj + 1], v);
            vec2 p = _vertices[jj + 0] + (_vertices[jj + 1] - _vertices[jj + 0]) * t;
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (_segments[ii] == quad) {
            double t = quad_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * _vertices[jj + 0]
                + 2 * (1 - t) * t * _vertices[jj + 1]
                + t * t * _vertices[jj + 2];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (_segments[ii] == cube) {
            double t = cube_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], _vertices[jj + 3], v);
            double s = 1.0 - t;
            double t2 = t * t;
            double s2 = s * s;
            vec2 p = s2 * s * _vertices[jj + 0]
                + 3.0 * s2 * t * _vertices[jj + 1]
                + 3.0 * s * t2 * _vertices[jj + 2]
                + t * t2 * _vertices[jj + 3];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    return best_idx;
}

//------------------------------------------------------------------------------
vec2 ship_outline::closest_point(vec2 v) const
{
    std::size_t best_idx = SIZE_MAX;
    vec2 best_point = vec2_zero;
    double best_dsqr = DBL_MAX;
    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            double t = segment_closest_point(_vertices[jj + 0], _vertices[jj + 1], v);
            vec2 p = _vertices[jj + 0] + (_vertices[jj + 1] - _vertices[jj + 0]) * t;
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (_segments[ii] == quad) {
            double t = quad_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * _vertices[jj + 0]
                   + 2 * (1 - t) * t * _vertices[jj + 1]
                   + t * t * _vertices[jj + 2];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (_segments[ii] == cube) {
            double t = cube_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], _vertices[jj + 3], v);
            double s = 1.0 - t;
            double t2 = t * t;
            double s2 = s * s;
            vec2 p = s2 * s * _vertices[jj + 0]
                + 3.0 * s2 * t * _vertices[jj + 1]
                + 3.0 * s * t2 * _vertices[jj + 2]
                + t * t2 * _vertices[jj + 3];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    return best_point;
}

//------------------------------------------------------------------------------
bool ship_outline::insert_vertex(vec2 v, double minimum_vertex_dsqr)
{
    std::size_t best_idx = SIZE_MAX;
    double best_t = 0;
    vec2 best_point = vec2_zero;
    double best_dsqr = DBL_MAX;

    if (length_sqr(v - _vertices[0]) < minimum_vertex_dsqr) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            if (length_sqr(v - _vertices[jj + 1]) < minimum_vertex_dsqr) {
                return false;
            }
            double t = segment_closest_point(_vertices[jj + 0], _vertices[jj + 1], v);
            vec2 p = _vertices[jj + 0] + (_vertices[jj + 1] - _vertices[jj + 0]) * t;
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 1;
        } else if (_segments[ii] == quad) {
            if (length_sqr(v - _vertices[jj + 2]) < minimum_vertex_dsqr) {
                return false;
            }
            double t = quad_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], v);
            vec2 p = (1 - t) * (1 - t) * _vertices[jj + 0]
                + 2 * (1 - t) * t * _vertices[jj + 1]
                + t * t * _vertices[jj + 2];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 2;
        } else if (_segments[ii] == cube) {
            if (length_sqr(v - _vertices[jj + 3]) < minimum_vertex_dsqr) {
                return false;
            }
            double t = cube_closest_point(_vertices[jj + 0], _vertices[jj + 1], _vertices[jj + 2], _vertices[jj + 3], v);
            double s = 1.0 - t;
            double t2 = t * t;
            double s2 = s * s;
            vec2 p = s2 * s * _vertices[jj + 0]
                   + 3.0 * s2 * t * _vertices[jj + 1]
                   + 3.0 * s * t2 * _vertices[jj + 2]
                   + t * t2 * _vertices[jj + 3];
            double dsqr = length_sqr(p - v);
            if (dsqr < best_dsqr) {
                best_idx = ii;
                best_t = t;
                best_point = p;
                best_dsqr = dsqr;
            }
            jj += 3;
        }
    }

    if (best_idx == SIZE_MAX || best_dsqr > minimum_vertex_dsqr) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (ii != best_idx) {
            if (_segments[ii] == line) {
                jj += 1;
            } else if (_segments[ii] == quad) {
                jj += 2;
            } else if (_segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (_segments[ii] == line) {
            // convert the line to a bezier curve by inserting a control point
            _vertices.insert(_vertices.begin() + jj + 1, best_point);
            _segments.insert(_segments.begin() + ii, line);
            return true;
        }

        if (_segments[ii] == quad) {
            // use De Casteljau's algorithm to subdivide the curve at `best_point`
            _segments.insert(_segments.begin() + ii, quad);
            vec2 p01 = (1.0 - best_t) * _vertices[jj + 0] + best_t * _vertices[jj + 1];
            vec2 p12 = (1.0 - best_t) * _vertices[jj + 1] + best_t * _vertices[jj + 2];
            // [a b c] -> [a p12 c]
            _vertices[jj + 1] = p12;
            // [a p12 c] -> [a p01 p p12 c]
            _vertices.insert(_vertices.begin() + jj + 1, {p01, best_point});
            return true;
        }

        if (_segments[ii] == cube) {
            // use De Casteljau's algorithm to subdivide the curve at `best_point`
            _segments.insert(_segments.begin() + ii, cube);
            vec2 p01 = (1.0 - best_t) * _vertices[jj + 0] + best_t * _vertices[jj + 1];
            vec2 p12 = (1.0 - best_t) * _vertices[jj + 1] + best_t * _vertices[jj + 2];
            vec2 p23 = (1.0 - best_t) * _vertices[jj + 2] + best_t * _vertices[jj + 3];
            vec2 p012 = (1.0 - best_t) * p01 + best_t * p12;
            vec2 p123 = (1.0 - best_t) * p12 + best_t * p23;
            // [a b c d] -> [a p123 p23 d]
            _vertices[jj + 1] = p123;
            _vertices[jj + 2] = p23;
            // [a p123 p23 d] -> [a p01 p012 p p123 p23 d]
            _vertices.insert(_vertices.begin() + jj + 1, {p01, p012, best_point});
            return true;
        }
    }

    assert(false);
    return false;
}

//------------------------------------------------------------------------------
bool ship_outline::remove_vertex(vec2 v, double minimum_vertex_dsqr)
{
    std::size_t best_idx = SIZE_MAX;
    double best_dsqr = DBL_MAX;
    for (std::size_t ii = 1; ii + 1 < _vertices.size(); ++ii) {
        double dsqr = length_sqr(v - _vertices[ii]);
        if (dsqr < best_dsqr) {
            best_idx = ii;
            best_dsqr = dsqr;
        }
    }

    if (best_dsqr > minimum_vertex_dsqr) {
        return false;
    }

    // cannot delete first or last vertex
    if (best_idx == 0 || best_idx >= _vertices.size() - 1) {
        return false;
    }

    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            // deleting first point of a line
            if (best_idx == jj + 0) {
                _vertices.erase(_vertices.begin() + best_idx);
                _segments.erase(_segments.begin() + ii);
                return true;
            }
            jj += 1;
        } else if (_segments[ii] == quad) {
            // deleting first point of a quadratic bezier curve
            if (best_idx == jj + 0) {
                _vertices.erase(_vertices.begin() + best_idx, _vertices.begin() + best_idx + 2);
                _segments.erase(_segments.begin() + ii);
                return true;

                // deleting quadratic bezier control point, turn into a line
            } else if (best_idx == jj + 1) {
                _segments[ii] = line;
                _vertices.erase(_vertices.begin() + best_idx);
                return true;
            }
            jj += 2;
        } else if (_segments[ii] == cube) {
            // deleting first point of a cubic bezier curve
            if (best_idx == jj + 0) {
                _vertices.erase(_vertices.begin() + best_idx, _vertices.begin() + best_idx + 3);
                _segments.erase(_segments.begin() + ii);
                return true;

                // deleting cubic bezier control point, turn into a quadratic bezier curve
            } else if (best_idx == jj + 1 || best_idx == jj + 2) {
                _segments[ii] = quad;
                _vertices.erase(_vertices.begin() + best_idx);
                return true;
            }
            jj += 3;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_outline::upconvert_segment(vec2 v)
{
    std::size_t best_idx = closest_segment(v);
    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (ii != best_idx) {
            if (_segments[ii] == line) {
                jj += 1;
            } else if (_segments[ii] == quad) {
                jj += 2;
            } else if (_segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (_segments[ii] == line) {
            _vertices.insert(_vertices.begin() + jj + 1, v);
            _segments[ii] = quad;
        } else if (_segments[ii] == quad) {
            _vertices.insert(_vertices.begin() + jj + 1, v);
            _segments[ii] = cube;
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_outline::downconvert_segment(vec2 v)
{
    std::size_t best_idx = closest_segment(v);
    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (ii != best_idx) {
            if (_segments[ii] == line) {
                jj += 1;
            } else if (_segments[ii] == quad) {
                jj += 2;
            } else if (_segments[ii] == cube) {
                jj += 3;
            }
            continue;
        }

        if (_segments[ii] == quad) {
            _vertices.erase(_vertices.begin() + jj + 1);
            _segments[ii] = line;
        } else if (_segments[ii] == cube) {
            _vertices.erase(_vertices.begin() + jj + 1);
            _segments[ii] = quad;
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_outline::linearize()
{
    _linearized.resize(0);
    _linearized.push_back(_vertices[0]);

    for (std::size_t ii = 0, jj = 0; ii < _segments.size(); ++ii) {
        if (_segments[ii] == line) {
            _linearized.push_back(_vertices[jj + 1]);
            jj += 1;
        } else if (_segments[ii] == quad) {
            std::vector<vec2> v = subdivide([&](double t){
                double s = 1.0 - t;
                return s * s * _vertices[jj + 0]
                    + 2.0 * s * t * _vertices[jj + 1]
                    + t * t * _vertices[jj + 2];
                }, 0.1f);
            _linearized.insert(_linearized.end(), v.begin() + 1, v.end());
            jj += 2;
        } else if (_segments[ii] == cube) {
            std::vector<vec2> v = subdivide([&](double t){
                double s = 1.0 - t;
                double t2 = t * t;
                double s2 = s * s;
                return s2 * s * _vertices[jj + 0]
                     + 3.0 * s2 * t * _vertices[jj + 1]
                     + 3.0 * s * t2 * _vertices[jj + 2]
                     + t * t2 * _vertices[jj + 3];
                }, 0.1f);
            _linearized.insert(_linearized.end(), v.begin() + 1, v.end());
            jj += 3;
        }
    }

    // duplicate vertices along the bottom half
    if (_linearized.back().y) {
        _linearized.push_back(vec2(_linearized.back().x, -_linearized.back().y));
    }

    for (std::size_t ii = _linearized.size() - 2; ii > 0; --ii) {
        _linearized.push_back(vec2(_linearized[ii].x, -_linearized[ii].y));
    }

    // duplicate final vertex if necessary, i.e. for transom stern
    if (_linearized[0].y) {
        _linearized.push_back(vec2(_linearized[0].x, -_linearized[0].y));
    }
}

//------------------------------------------------------------------------------
std::vector<vec2> ship_outline::subdivide(std::function<vec2(double)> fn, float error)
{
    std::vector<vec2> p;
    std::vector<double> t;
    std::vector<double> e;

    p.push_back(fn(0.0));
    p.push_back(fn(1.0));
    t.push_back(0.0);
    t.push_back(1.0);
    e.push_back(0.0);

    for (std::size_t n = 1; ; ++n) {
        p.insert(p.end() - 1, vec2_zero);
        t.insert(t.end() - 1, 0.0);
        e.push_back(0.0);

        for (std::size_t ii = 1; ii < n + 1; ++ii) {
            t[ii] = double(ii) / double(n + 1);
            p[ii] = fn(t[ii]);
        }

        for (std::size_t jj = 0; jj < 128; ++jj) {
            double rms = 0.0;
            double emax = 0.0;
            for (std::size_t ii = 0; ii < n + 1; ++ii) {
                double t0 = 0.5 * (t[ii] + t[ii + 1]);
                vec2 p0 = fn(t0);
                vec2 v = p[ii + 1] - p[ii];
                vec2 r = (p[ii] - p0) - dot(p[ii] - p0, v) * v / length_sqr(v);
                e[ii] = length(r);
                emax = max(emax, e[ii]);
                rms += square(e[ii]);
            }

            if (emax < error) {
                return p;
            }

            rms = sqrt(rms / double(n));
            if (rms > 8.0 * error) {
                break;
            }

            e[0] = 1.0 / e[0];
            for (std::size_t ii = 1; ii < n + 1; ++ii) {
                e[ii] = e[ii - 1] + 1.0 / e[ii];
            }
            for (std::size_t ii = 1; ii < n + 1; ++ii) {
                t[ii] = 0.5 * e[ii - 1] / e.back() + 0.5 * t[ii];
                p[ii] = fn(t[ii]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
#define SHIP(L,B)               \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(0.f, 0.5f * B),        \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),


#define SHIP_CUBE(L,B)          \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(-0.25f * L, 0.5f * B), \
    vec2(-0.1f * L, 0.5f * B),  \
    vec2(0.1f * L, 0.5f * B),   \
    vec2(0.25f * L, 0.5f * B),  \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),

#define SHIP_CUBE2(L,B)          \
    vec2(-0.5f * L, 0.f),       \
    vec2(-0.5f * L, 0.5f * B),  \
    vec2(-0.25f * L, 0.5f * B), \
    vec2(0, 0.5f * B),          \
    vec2(0.25f * L, 0.5f * B),  \
    vec2(0.3f * L, 0.5f * B),   \
    vec2(0.5f * L, 0.f),

//------------------------------------------------------------------------------
ship_editor::ship_editor()
    : _view{}
    , _cursor{}
    , _snap_distance(1.f)
    , _snap_to_grid(true)
    , _snap_to_edge(true)
    , _draw_grid(true)
    , _draw_linearized(false)
    , _is_panning(false)
    , _is_panning_image(false)
    , _control(false)
    , _is_dragging(false)
    , _feature(feature::none)
    , _feature_index(0)
    , _feature_outline(0)
    , _feature_transform(mat3_identity)
    , _image(nullptr)
    , _image_offset(vec2_zero)
    , _image_scale("image_scale", 1.f/15.175f, 0, "")
    , _mode(editor_mode::deck)
    , _turret_instance(0)
{
    clear();
}

//------------------------------------------------------------------------------
ship_editor::~ship_editor()
{
}

//------------------------------------------------------------------------------
vec2 ship_editor::cursor_to_world() const
{
    //return _view.origin + _view.size * (_cursor / 
    //    vec2(_view.viewport.maxs() - _view.viewport.mins()) - vec2(.5f,.5f));
    return _view.origin + _view.size * _cursor;
}

//------------------------------------------------------------------------------
double ship_editor::snap_radius(double r) const
{
    double snap_distance = _snap_to_grid ? _snap_distance : 0.01;
    return std::floor(r / snap_distance + 0.5) * snap_distance;
}

//------------------------------------------------------------------------------
vec2 ship_editor::snap_vertex(vec2 pos) const
{
    vec2 out = pos;
    double snap_distance = _snap_to_grid ? _snap_distance : 0.01;

    vec2 grid_snap = {
        std::floor(pos.x / snap_distance + 0.5) * snap_distance,
        std::floor(pos.y / snap_distance + 0.5) * snap_distance,
    };

    out = grid_snap;

    return out;
}

//------------------------------------------------------------------------------
void ship_editor::draw(render::system* renderer, time_value /*time*/) const
{
    if (_image) {
        renderer->set_view(_view);
        vec2 image_size = vec2(vec2i(_image->width(), _image->height())) * _image_scale;
        renderer->draw_image(_image, _image_offset - 0.5 * image_size, image_size, color4(1,1,1,.5f));
    }

    renderer->set_view(_view);
    vec2 vmin = _view.origin - 0.5 * _view.size;
    vec2 vmax = _view.origin + 0.5 * _view.size;
    vec2 vertex_size = vec2(render_vertex_size());

    //
    // draw grid
    //

    {
        color4 c(1.f, 1.f, 1.f, .2f);
        renderer->draw_line(vec2(0, vmin.y), vec2(0, vmax.y), c, c);
        renderer->draw_line(vec2(vmin.x, 0), vec2(vmax.x, 0), c, c);
        if (_draw_grid) {
            vec2 mins = _snap_distance * vec2(std::round((vmin.x) / _snap_distance),
                                              std::round((vmin.y) / _snap_distance));
            vec2 maxs = _view.origin + 0.5 * _view.size;

            for (double x = mins.x; x < maxs.x; x += _snap_distance) {
                renderer->draw_line(vec2(x, vmin.y), vec2(x, vmax.y), c, c);
            }
            for (double y = mins.y; y < maxs.y; y += _snap_distance) {
                renderer->draw_line(vec2(vmin.x, y), vec2(vmax.x, y), c, c);
            }
        }
    }

    //
    // draw deck outline
    //

    if (_draw_linearized) {
        _outlines[0].draw_linearized(renderer, mat3_identity, vertex_size);
    } else {
        _outlines[0].draw(renderer, mat3_identity, vertex_size);
    }

    //
    // draw turrets
    //

    for (std::size_t ii = 0; ii < _turret_instances.size(); ++ii) {
        turret_instance const& i = _turret_instances[ii];
        turret const& t = _turrets[i.index];
        if (_draw_linearized) {
            _outlines[t.index].draw_linearized(renderer, i.transform, vertex_size);
        } else {
            _outlines[t.index].draw(renderer, i.transform, vertex_size);
            vec2 center = vec2_zero * i.transform;
            double offset = render_vertex_size() * 2.0;

            if (_feature == feature::turret && ii == _turret_instance) {
                renderer->draw_line(center - vec2(0,offset), center + vec2(0,offset), color4(0,1,0,1), color4(0,1,0,1));
                renderer->draw_line(center - vec2(offset,0), center + vec2(offset,0), color4(0,1,0,1), color4(0,1,0,1));
            } else {
                renderer->draw_line(center - vec2(0,offset), center + vec2(0,offset), color4(1,0,0,1), color4(1,0,0,1));
                renderer->draw_line(center - vec2(offset,0), center + vec2(offset,0), color4(1,0,0,1), color4(1,0,0,1));
            }

            if (_mode == editor_mode::turret && ii == _turret_instance) {
                if (_feature == feature::turret_radius && _feature_index == i.index) {
                    renderer->draw_arc(center, float(_turrets[i.index].radius), 0, 0, math::twopi, color4(0,1,0,1));
                } else {
                    renderer->draw_arc(center, float(_turrets[i.index].radius), 0, 0, math::twopi, color4(0,1,1,1));
                }

                vec2 v0 = vec2(_turrets[i.index].radius, 0) * i.transform;
                vec2 v1 = vec2(_turrets[i.index].radius + 1.f, 0) * i.transform;
                renderer->draw_line(v0, v1, color4(0,1,1,1), color4(0,1,1,1));
                if (_feature == feature::turret_rotation && _feature_index == _turret_instance) {
                    renderer->draw_box(vertex_size, v1, color4(0,1,0,1));
                } else {
                    renderer->draw_box(vertex_size, v1, color4(1,0,0,1));
                }
            }
        }
    }

    //
    // draw vertex highlight and closest point
    //

    if (_feature == feature::vertex) {
        ship_outline const& drag_outline = _outlines[_feature_outline];
        renderer->draw_box(vertex_size, drag_outline.vertices()[_feature_index] * _feature_transform, color4(0,1,0,1));
    } else if (_feature == feature::vertex_mirror) {
        ship_outline const& drag_outline = _outlines[_feature_outline];
        renderer->draw_box(vertex_size, drag_outline.vertices()[_feature_index] * vec2(1,-1) * _feature_transform, color4(0,1,0,1));
    }

    vec2 world_pos = snap_vertex(cursor_to_world());

    {
        ship_outline const& outline = _outlines[_feature_outline];

        vec2 crosshair = world_pos;

        string::view s = "";
        if (_feature == feature::turret_radius) {
            // draw turret radius
            assert(_turret_instance < _turret_instances.size());
            s = va("(%g)", _turrets[_turret_instances[_turret_instance].index].radius);
        } else if (_feature == feature::turret_rotation) {
            // draw turret rotation
            assert(_turret_instance < _turret_instances.size());
            auto& instance = _turret_instances[_turret_instance];
            int angle = int(std::round(math::rad2deg(std::atan2(instance.transform[0][1], instance.transform[0][0]))));
            s = va("(%d\xb0)", angle);
        } else if (_feature == feature::turret) {
            // draw turret origin in world space
            assert(_turret_instance < _turret_instances.size());
            s = va("(%g, %g)", _turret_instances[_turret_instance].transform[2][0], _turret_instances[_turret_instance].transform[2][1]);
            crosshair = vec2(_turret_instances[_turret_instance].transform[2][0], _turret_instances[_turret_instance].transform[2][1]);
        } else if (_feature == feature::vertex) {
            // draw vertex position in local space
            vec2 v = outline.vertices()[_feature_index];
            s = va("(%g, %g)", v.x, v.y);
            crosshair = v * _feature_transform;
        } else if (_feature == feature::vertex_mirror) {
            // draw vertex position in local space
            vec2 v = outline.vertices()[_feature_index] * vec2(1,-1);
            s = va("(%g, %g)", v.x, v.y);
            crosshair = v * _feature_transform;
        } else if (_mode == editor_mode::turret && _turret_instance < _turret_instances.size()) {
            // draw cursor position in turret-local space
            vec2 local_pos = snap_vertex(cursor_to_world() * _turret_instances[_turret_instance].transform.inverse_transform());
            s = va("(%g, %g)", local_pos.x, local_pos.y);
            crosshair = local_pos * _turret_instances[_turret_instance].transform;
        } else {
            // draw cursor position in world space
            s = va("(%g, %g)", world_pos.x, world_pos.y);
        }
        vec2 size = renderer->string_size(s);
        renderer->draw_string(s, _view.origin + _view.size * 0.49 - size, color4(1,1,1,0.5));
        // draw crosshair
        if (_feature != feature::turret_radius && _feature != feature::turret_rotation) {
            renderer->draw_line(vec2(crosshair.x, vmin.y), vec2(crosshair.x, vmax.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
            renderer->draw_line(vec2(vmin.x, crosshair.y), vec2(vmax.x, crosshair.y), color4(0,1,1,.2f), color4(0,1,1,.2f));
        }
    }
    {
        vec2 size = renderer->monospace_size("foo");
        renderer->draw_monospace(_filename, _view.origin - _view.size * vec2(0.5,-0.5) - vec2(0,size.y*1.0), color4(1,1,1,1));

        renderer->draw_monospace("(s) snap to grid", _view.origin - _view.size * vec2(0.5,-0.5) - vec2(0,size.y*3.0), color4(1,1,1, _snap_to_grid ? .6f : .3f));
        renderer->draw_monospace("( ) snap to edge", _view.origin - _view.size * vec2(0.5,-0.5) - vec2(0,size.y*4.0), color4(1,1,1, _snap_to_edge ? .6f : .3f));

        renderer->draw_monospace("(d) deck mode", _view.origin - _view.size * vec2(0.5,-0.5) - vec2(0,size.y*6.0), color4(1,1,1, _mode == editor_mode::deck ? .6f : .3f));
        renderer->draw_monospace("(t) turret mode", _view.origin - _view.size * vec2(0.5,-0.5) - vec2(0,size.y*7.0), color4(1,1,1, _mode == editor_mode::turret ? .6f : .3f));
    }
}

//------------------------------------------------------------------------------
bool ship_editor::insert_turret(vec2 v)
{
    if (_turret_instance < _turret_instances.size()) {
        std::size_t index = _turret_instances[_turret_instance].index;
        if (_outlines[_turrets[index].index].insert_vertex(
            v * _turret_instances[_turret_instance].transform.inverse_transform(),
            minimum_vertex_dsqr)) {
            return true;
        }
    }

    if (!_turrets.size() || _control) {
        _turrets.push_back({{5.25}, _outlines.size()});
        _outlines.push_back(ship_outline(
            {vec2(-5,0), vec2(0,5), vec2(5,0)},
            {ship_outline::line, ship_outline::line}));
    }

    _turret_instances.push_back({mat3::transform(v, rot2_identity), _turrets.size() - 1});
    _turret_instance = _turret_instances.size() - 1;

    return true;
}

//------------------------------------------------------------------------------
bool ship_editor::remove_turret(vec2 v)
{
    if (_turret_instance < _turret_instances.size()) {
        std::size_t index = _turret_instances[_turret_instance].index;
        if (_outlines[_turrets[index].index].remove_vertex(
            v * _turret_instances[_turret_instance].transform.inverse_transform(),
            minimum_vertex_dsqr)) {
            return true;
        }
    }
    std::size_t best_instance = SIZE_MAX;
    double best_dsqr = DBL_MAX;
    for (std::size_t ii = 0; ii < _turret_instances.size(); ++ii) {
        double dsqr = length_sqr(v * _turret_instances[ii].transform.inverse_transform());
        if (dsqr < best_dsqr) {
            best_dsqr = dsqr;
            best_instance = ii;
        }
    }

    if (best_dsqr > minimum_vertex_dsqr) {
        return false;
    }

    if (_turret_instance == best_instance) {
        _turret_instance = SIZE_MAX;
    }
    _turret_instances.erase(_turret_instances.begin() + best_instance);
    return true;
}

//------------------------------------------------------------------------------
bool ship_editor::key_event(int key, bool down)
{
    if (down) {
        if (key == K_MOUSE2) {
            if (_control) {
                _is_panning_image = true;
            } else {
                _is_panning = true;
            }
            return true;
        }
        if (key == K_CTRL) {
            _control = true;
        }
    } else if (key == K_MOUSE2) {
        if (_is_panning) {
            _is_panning = false;
            return true;
        } if (_is_panning_image) {
            _is_panning_image = false;
            return true;
        }
    } else {
        if (key == K_CTRL) {
            _control = false;
        } else if (key == K_MOUSE1) {
            _is_dragging = false;
            update_highlight();
        }
    }

    if (!down) {
        return false;
    }

    switch (key) {
        case '[':
            _snap_distance *= 2.0;
            return true;

        case ']':
            _snap_distance *= 0.5;
            return true;

        case 'd':
            _mode = editor_mode::deck;
            return true;

        case 'g':
            _draw_grid = !_draw_grid;
            return true;

        case 'i':
            if (_control) {
                string::buffer filename;
                if (get_image_filename(filename)) {
                    _image = application::singleton()->window()->renderer()->load_image(filename);
                }
                _control = false; // file dialog eats the control up key event
                return true;
            }
            break;

        case 'o':
            if (_control) {
                string::buffer filename;
                if (get_load_filename(filename) && load(filename)) {
                    _filename = filename;
                }
                _control = false; // file dialog eats the control up key event
                return true;
            }
            break;

        case 's':
            if (_control) {
                string::buffer filename;
                if (_filename.length() && save(_filename)) {
                    // no-op
                } else if (get_save_filename(filename) && save(filename)) {
                    _filename = filename;
                }
                _control = false; // file dialog eats the control up key event
                return true;
            } else {
                _snap_to_grid = !_snap_to_grid;
                return true;
            }

        case 't':
            _mode = editor_mode::turret;
            return true;

        case 'l':
            _draw_linearized = !_draw_linearized;
            if (_draw_linearized) {
                for (auto& outline : _outlines) {
                    outline.linearize();
                }
            }
            return true;

        case K_F5:
            save("assets/ref/design/quicksave.design");
            return true;

        case K_F8:
            load("assets/ref/design/quicksave.design");
            return true;

        case K_F9:
            export_verts("editor.log");
            return true;

        case K_MOUSE1: {
            if (_feature != feature::none) {
                _is_dragging = true;
                return true;
            }
            break;
        }

        case K_MOUSE2:
            break;

        case K_MWHEELUP:
            _view.size /= 1.25;
            return true;

        case K_MWHEELDOWN:
            _view.size *= 1.25;
            return true;

        case K_INS:
            if (_mode == editor_mode::deck) {
                _outlines[0].insert_vertex(cursor_to_world(), minimum_vertex_dsqr);
            } else if (_mode == editor_mode::turret) {
                insert_turret(cursor_to_world());
            }
            update_highlight();
            return true;

        case K_DEL:
            if (_mode == editor_mode::deck) {
                _outlines[0].remove_vertex(cursor_to_world(), minimum_vertex_dsqr);
            } else if (_mode == editor_mode::turret) {
                remove_turret(cursor_to_world());
            }
            update_highlight();
            return true;

        case K_PGUP:
            if (_mode == editor_mode::deck) {
                _outlines[0].upconvert_segment(cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                if (_turret_instance < _turret_instances.size()) {
                    auto& instance = _turret_instances[_turret_instance];
                    auto& turret = _turrets[instance.index];
                    auto& outline = _outlines[turret.index];
                    return outline.upconvert_segment(
                        cursor_to_world() * instance.transform.inverse_transform());
                }
            }
            break;

        case K_PGDN:
            if (_mode == editor_mode::deck) {
                _outlines[0].downconvert_segment(cursor_to_world());
            } else if (_mode == editor_mode::turret) {
                if (_turret_instance < _turret_instances.size()) {
                    auto& instance = _turret_instances[_turret_instance];
                    auto& turret = _turrets[instance.index];
                    auto& outline = _outlines[turret.index];
                    return outline.downconvert_segment(
                        cursor_to_world() * instance.transform.inverse_transform());
                }
            }
            break;

        case K_KP_PLUS:
            if (_mode == editor_mode::turret) {
                if (_control && _turret_instance < _turret_instances.size()) {
                    if (_turret_instances[_turret_instance].index + 1 < _turrets.size()) {
                        ++_turret_instances[_turret_instance].index;
                        return true;
                    }
                } else if (!_control) {
                    if (_turret_instance + 1 < _turret_instances.size()) {
                        ++_turret_instance;
                        return true;
                    }
                }
            }
            break;

        case K_KP_MINUS:
            if (_mode == editor_mode::turret) {
                if (_control && _turret_instance < _turret_instances.size()) {
                    if (_turret_instances[_turret_instance].index > 0) {
                        --_turret_instances[_turret_instance].index;
                        return true;
                    }
                } else if (!_control) {
                    if (_turret_instance > 0) {
                        --_turret_instance;
                        return true;
                    }
                }
            }
            break;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::cursor_event(vec2 position)
{
    if (_is_panning) {
        _view.origin -= (position - _cursor) * _view.size;
    } else if (_is_panning_image) {
        _image_offset = (position - _cursor) * _view.size + _image_offset;
    } else if (_is_dragging && _feature == feature::vertex) {
        vec2 pos = snap_vertex((_view.origin + _view.size * position) * _feature_transform.inverse_transform());
        _outlines[_feature_outline].vertices()[_feature_index] = pos;
    } else if (_is_dragging && _feature == feature::vertex_mirror) {
        vec2 pos = snap_vertex((_view.origin + _view.size * position) * _feature_transform.inverse_transform()) * vec2(1,-1);
        _outlines[_feature_outline].vertices()[_feature_index] = pos;
    } else if (_is_dragging && _feature == feature::turret) {
        auto& instance = _turret_instances[_turret_instance];
        vec2 pos = snap_vertex(_view.origin + _view.size * position);
        instance.transform[2][0] = pos.x;
        instance.transform[2][1] = pos.y;
    } else if (_is_dragging && _feature == feature::turret_radius) {
        auto& instance = _turret_instances[_turret_instance];
        auto& turret = _turrets[instance.index];
        vec2 pos = _view.origin + _view.size * position;
        turret.radius = snap_radius(length(pos - vec2(instance.transform[2][0], instance.transform[2][1])));
    } else if (_is_dragging && _feature == feature::turret_rotation) {
        auto& instance = _turret_instances[_turret_instance];
        vec2 dir = (_view.origin + _view.size * position) - vec2(instance.transform[2][0], instance.transform[2][1]);
        // round to nearest degree
        double angle = math::deg2rad(std::round(math::rad2deg(std::atan2(dir.y, dir.x))));
        double c = cos(angle);
        double s = sin(angle);
        instance.transform[0][0] = c;
        instance.transform[0][1] = s;
        instance.transform[1][0] = -s;
        instance.transform[1][1] = c;
    }
    _cursor = position;

    if (!_is_dragging) {
        update_highlight();
    }
}

//------------------------------------------------------------------------------
void ship_editor::update_highlight()
{
    feature best_feature = feature::none;
    std::size_t best_index = 0;
    double best_dsqr = DBL_MAX;

    if (_mode == editor_mode::deck) {
        // check nearest vertex
        best_feature = feature::vertex;
        best_index = _outlines[0].closest_vertex(cursor_to_world());
        best_dsqr = length_sqr(_outlines[0].vertices()[best_index] - cursor_to_world());
        // check nearest vertex mirror
        std::size_t mirror_index = _outlines[0].closest_vertex(cursor_to_world() * vec2(1,-1));
        double mirror_dsqr = length_sqr(_outlines[0].vertices()[mirror_index] - cursor_to_world() * vec2(1,-1));
        if (mirror_dsqr < best_dsqr) {
            best_feature = feature::vertex_mirror;
            best_index = mirror_index;
            best_dsqr = mirror_dsqr;
        }
    } else if (_mode == editor_mode::turret && _turret_instance < _turret_instances.size()) {
        auto& instance = _turret_instances[_turret_instance];
        auto& turret = _turrets[instance.index];
        auto& outline = _outlines[turret.index];
        vec2 cursor_local = cursor_to_world() * instance.transform.inverse_transform();
        // check nearest vertex
        best_feature = feature::vertex;
        best_index = outline.closest_vertex(cursor_local);
        best_dsqr = length_sqr(outline.vertices()[best_index] - cursor_local);
        // check nearest vertex mirror
        std::size_t mirror_index = outline.closest_vertex(cursor_local * vec2(1,-1));
        double mirror_dsqr = length_sqr(outline.vertices()[mirror_index] - cursor_local * vec2(1,-1));
        if (mirror_dsqr < best_dsqr) {
            best_feature = feature::vertex_mirror;
            best_index = mirror_index;
            best_dsqr = mirror_dsqr;
        }
        // check translation widget
        double origin_dsqr = length_sqr(cursor_local);
        if (origin_dsqr < best_dsqr) {
            best_feature = feature::turret;
            best_index = _turret_instance;
            best_dsqr = origin_dsqr;
        }
        // check radius widget
        double radius_dsqr = square(length(cursor_local) - turret.radius);
        if (radius_dsqr < best_dsqr) {
            best_feature = feature::turret_radius;
            best_index = instance.index;
            best_dsqr = radius_dsqr;
        }
        // check rotation widget
        vec2 rotation_pos = vec2(turret.radius + 1.0, 0) * instance.transform;
        double rotation_dsqr = length_sqr(rotation_pos - cursor_to_world());
        if (rotation_dsqr < best_dsqr) {
            best_feature = feature::turret_rotation;
            best_index = _turret_instance;
            best_dsqr = rotation_dsqr;
        }
    }

    double minimum_dsqr = length_sqr(0.01 * _view.size);
    if (best_dsqr < minimum_dsqr) {
        _feature_index = best_index;
        _feature = best_feature;
        if (_mode == editor_mode::deck) {
            _feature_outline = 0;
            _feature_transform = mat3_identity;
        } else {
            auto& instance = _turret_instances[_turret_instance];
            auto& turret = _turrets[instance.index];
            _feature_outline = turret.index;
            _feature_transform = instance.transform;
        }
    } else {
        _feature_index = 0;
        _feature = feature::none;
    }
}

//------------------------------------------------------------------------------
string::buffer normalize_path(WCHAR const* path)
{
    string::buffer out;

    // Strip current working directory if path is in a subdirectory
    WCHAR cwd[1024] = {};
    DWORD cwd_len = GetCurrentDirectoryW(narrow_cast<DWORD>(countof(cwd)), cwd);

    if (_wcsnicmp(path, cwd, cwd_len) == 0) {
        path += cwd_len;
        if (*path == '\\') {
            ++path;
        }
    }

    // Get length of path as UTF8, including null terminator
    int len = WideCharToMultiByte(
        CP_UTF8,
        WC_NO_BEST_FIT_CHARS,
        path,
        -1,
        0,
        0,
        NULL,
        NULL);

    // Convert in-place into output string buffer
    out.resize(len - 1);
    WideCharToMultiByte(CP_UTF8,
        WC_NO_BEST_FIT_CHARS,
        path,
        -1,
        out.data(),
        len,
        NULL,
        NULL);

    // Replace backslashes with forward slashes
    for (std::size_t ii = 0; ii < out.length(); ++ii) {
        if (out[ii] == '\\') {
            out[ii] = '/';
        }
    }

    return out;
}

//------------------------------------------------------------------------------
bool ship_editor::get_save_filename(string::buffer& filename) const
{
    OPENFILENAMEW ofn = {};
    WCHAR buffer[1024] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = narrow_cast<int>(countof(buffer));
    MultiByteToWideChar(
        CP_UTF8,
        0,
        _filename.begin(),
        narrow_cast<int>(_filename.length()),
        buffer,
        narrow_cast<int>(countof(buffer)));
    ofn.lpstrFilter = L"Design Files (*.design)\0*.design\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 0;
    ofn.lpstrDefExt = L"design";
    ofn.Flags = OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn) == TRUE) {
        filename = normalize_path(buffer);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::get_load_filename(string::buffer& filename) const
{
    OPENFILENAMEW ofn = {};
    WCHAR buffer[1024] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = narrow_cast<int>(countof(buffer));
    ofn.lpstrFilter = L"Design Files (*.design)\0*.design\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        filename = normalize_path(buffer);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool ship_editor::get_image_filename(string::buffer& filename) const
{
    OPENFILENAMEW ofn = {};
    WCHAR buffer[1024] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = narrow_cast<int>(countof(buffer));
    ofn.lpstrFilter = L"Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        filename = normalize_path(buffer);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void ship_editor::clear()
{
    _filename.clear();

    _view.viewport.maxs() = application::singleton()->window()->size();
    _view.size = {64.0, 64.0 * double(_view.viewport.maxs().y) / double(_view.viewport.maxs().x)};

    _image = application::singleton()->window()->renderer()->load_image(
        //"C:\\Users\\Carter\\OneDrive\\Pictures\\Trade Wars\\Constellation.bmp"
        //"D:\\Users\\Carter\\Pictures\\Yamato1945.bmp"
        //"D:\\Users\\Carter\\Pictures\\Kongo1944.bmp"
        //"D:\\Users\\Carter\\Pictures\\Fuso1944.bmp"
        //"D:\\Users\\Carter\\Pictures\\Iowa_classe_battleships_drawing.bmp"
        //"D:\\Users\\Carter\\Pictures\\KGV-as-built.bmp"
        //"D:\\Users\\Carter\\Pictures\\3607_Richelieu1941-09BattleofDakar_20231216154932.bmp"
        "D:\\Users\\Carter\\Pictures\\6474_Bismarck1941-05-271_20240302131526.bmp"
        //"D:\\Users\\Carter\\Pictures\\Littorio_class_battleship-drawing-2views.bmp"
    );

    _outlines.clear();
    //_outlines.push_back(ship_outline(
    //    { SHIP(219.61f, 33.1f) },
    //    { ship_outline::quad, ship_outline::quad }
    //));

    //_deck_vertices = { SHIP_CUBE(219.61f, 28.04f) }; // Kongo
    //_deck_vertices = { SHIP_CUBE(210.f, 33.1f) }; // Fuso
    //_deck_vertices = { SHIP_CUBE(270.f, 33.f) }; // Iowa
    //_deck_vertices = { SHIP_CUBE(227.f, 31.5f) }; // KGV
    //_deck_segments = { cube, line, cube };
    //_deck_vertices = { SHIP_CUBE2(247.85f, 33.08f) }; // Richelieu (scale 0.1524)
    _outlines.push_back(ship_outline(
        { SHIP_CUBE2(251.f, 36.f) }, // Bismarck (scale 0.1503)
        { ship_outline::cube, ship_outline::cube }
    ));
    //_deck_vertices = { SHIP_CUBE(237.76f, 32.82f) }; // Littorio (scale 0.415)
    //_deck_segments = { cube, line, cube };

    _turrets.clear();
    _turret_instances.clear();
    _turret_instance = 0;
}

//------------------------------------------------------------------------------
struct file_header
{
    enum version {
        //! Added turrets and turret instances
        v2 = 104,
        //! Conversion from single-precision to double-precision
        v3 = 112,
    };

    std::size_t header_size;
    std::size_t image_name_size;
    std::size_t image_name_offset;
    vec2 image_offset;
    float image_scale;
    std::size_t deck_vertices_size;
    std::size_t deck_vertices_offset;
    std::size_t deck_segments_size;
    std::size_t deck_segments_offset;
    // turrets
    std::size_t turrets_size;
    std::size_t turrets_offset;
    // turret instances
    std::size_t turret_instances_size;
    std::size_t turret_instances_offset;
};

//------------------------------------------------------------------------------
struct turret_header
{
    double radius;
    std::size_t vertices_size;
    std::size_t vertices_offset;
    std::size_t segments_size;
    std::size_t segments_offset;
};

//------------------------------------------------------------------------------
bool ship_editor::save(string::view filename) const
{
    g_Game->message("saving '%s'...", filename.c_str());

    file::stream s = file::open(filename, file::mode::write);
    if (!s) {
        return false;
    }

    file_header h;

    h.header_size = sizeof(file_header);
    h.image_name_size = _image->name().length();
    h.image_name_offset = 0;
    h.image_offset = _image_offset;
    h.image_scale = _image_scale;
    h.deck_vertices_size = _outlines[0].vertices().size() * sizeof(_outlines[0].vertices()[0]);
    h.deck_vertices_offset = 0;
    h.deck_segments_size = _outlines[0].segments().size() * sizeof(_outlines[0].segments()[0]);
    h.deck_segments_offset = 0;
    h.turrets_size = _turrets.size() * sizeof(turret_header);
    h.turrets_offset = 0;
    h.turret_instances_size = _turret_instances.size() * sizeof(_turret_instances[0]);
    h.turret_instances_offset = 0;

    // write header placeholder
    s.write((file::byte const*)&h, h.header_size);
    // write image name
    h.image_name_offset = s.tell();
    s.write((file::byte const*)_image->name().c_str(), h.image_name_size);
    // write deck vertices
    h.deck_vertices_offset = s.tell();
    s.write((file::byte const*)_outlines[0].vertices().data(), h.deck_vertices_size);
    // write deck segments
    h.deck_segments_offset = s.tell();
    s.write((file::byte const*)_outlines[0].segments().data(), h.deck_segments_size);
    // write turrets
    turret_header th[128];
    for (std::size_t ii = 0; ii < _turrets.size(); ++ii) {
        assert(ii < countof(th));
        auto& outline = _outlines[_turrets[ii].index];
        th[ii].radius = _turrets[ii].radius;
        th[ii].vertices_size = outline.vertices().size() * sizeof(outline.vertices()[0]);
        th[ii].segments_size = outline.segments().size() * sizeof(outline.segments()[0]);
        // write turret vertices
        th[ii].vertices_offset = s.tell();
        s.write((file::byte const*)outline.vertices().data(), th[ii].vertices_size);
        // write turret segments
        th[ii].segments_offset = s.tell();
        s.write((file::byte const*)outline.segments().data(), th[ii].segments_size);
    }
    h.turrets_offset = s.tell();
    s.write((file::byte const*)th, h.turrets_size);
    // write turret instances
    h.turret_instances_offset = s.tell();
    s.write((file::byte const*)_turret_instances.data(), h.turret_instances_size);
    // rewrite the file header with populated offset fields
    s.seek(0, file::seek::set);
    s.write((file::byte const*)&h, h.header_size);

    s.close();
    return true;
}

//------------------------------------------------------------------------------
bool ship_editor::load(string::view filename)
{
    g_Game->message("loading '%s'...", filename.c_str());

    file::buffer b = file::read(filename);
    if (!b.size()) {
        return false;
    }

    file_header const* h = reinterpret_cast<file_header const*>(b.data());
    std::size_t header_size = h->header_size;

    string::view image_name(
        (char const*)b.data() + h->image_name_offset,
        (char const*)b.data() + h->image_name_offset + h->image_name_size);
    _image = application::singleton()->window()->renderer()->load_image(image_name);

    if (header_size < file_header::version::v3) {
        _image_offset.x = reinterpret_cast<float const*>(&h->image_offset)[0];
        _image_offset.y = reinterpret_cast<float const*>(&h->image_offset)[1];
        _image_scale = reinterpret_cast<float const*>(&h->image_offset)[2];
        // Realign header with fields after image scale, offset is only 8 bytes due to padding
        h = reinterpret_cast<file_header const*>(reinterpret_cast<byte const*>(h) - 8);
    } else {
        _image_offset = h->image_offset;
        _image_scale = h->image_scale;
    }

    std::vector<vec2> vertices;
    std::vector<ship_outline::segment_type> segments;

    if (header_size < file_header::version::v3) {
        vertices.resize(h->deck_vertices_size / sizeof(vec2f));
        for (std::size_t ii = 0, sz = vertices.size(); ii < sz; ++ii) {
            vertices[ii] = reinterpret_cast<vec2f const*>(b.data() + h->deck_vertices_offset)[ii];
        }
    } else {
        vertices.resize(h->deck_vertices_size / sizeof(vertices[0]));
        memcpy(vertices.data(), b.data() + h->deck_vertices_offset, h->deck_vertices_size);
    }

    segments.resize(h->deck_segments_size / sizeof(segments[0]));
    memcpy(segments.data(), b.data() + h->deck_segments_offset, h->deck_segments_size);

    _outlines.clear();
    _outlines.push_back({std::move(vertices), std::move(segments)});

    //
    // Load turrets and turret instances
    //

    if (header_size >= file_header::version::v2) {
        _turrets.resize(h->turrets_size / sizeof(turret_header));
        turret_header const* th = reinterpret_cast<turret_header const*>(b.data() + h->turrets_offset);
        for (std::size_t ii = 0; ii < _turrets.size(); ++ii, ++th) {

            if (header_size < file_header::version::v3) {
                _turrets[ii].radius = *reinterpret_cast<float const*>(&th->radius);
                // No realignment required due to padding
            } else {
                _turrets[ii].radius = th->radius;
            }

            if (header_size < file_header::version::v3) {
                vertices.resize(th->vertices_size / sizeof(vec2f));
                for (std::size_t jj = 0, sz = vertices.size(); jj < sz; ++jj) {
                    vertices[jj] = reinterpret_cast<vec2f const*>(b.data() + th->vertices_offset)[jj];
                }
            } else {
                vertices.resize(th->vertices_size / sizeof(vertices[0]));
                memcpy(vertices.data(), b.data() + th->vertices_offset, th->vertices_size);
            }

            segments.resize(th->segments_size / sizeof(segments[0]));
            memcpy(segments.data(), b.data() + th->segments_offset, th->segments_size);

            _turrets[ii].index = _outlines.size();
            _outlines.push_back({std::move(vertices), std::move(segments)});
        }

        if (header_size < file_header::version::v3) {
            struct turret_instance_f { float transform[9]; std::size_t index; };
            _turret_instances.resize(h->turret_instances_size / sizeof(turret_instance_f));

            turret_instance_f const* ti = reinterpret_cast<turret_instance_f const*>(b.data() + h->turret_instances_offset);
            for (std::size_t jj = 0; jj < _turret_instances.size(); ++jj, ++ti) {
                _turret_instances[jj].transform = mat3(ti->transform[0], ti->transform[1], ti->transform[2],
                                                       ti->transform[3], ti->transform[4], ti->transform[5],
                                                       ti->transform[6], ti->transform[7], ti->transform[8]);
                _turret_instances[jj].index = ti->index;
            }
        } else {
            _turret_instances.resize(h->turret_instances_size / sizeof(_turret_instances[0]));
            memcpy(_turret_instances.data(), b.data() + h->turret_instances_offset, h->turret_instances_size);
        }
    } else {
        _turrets.resize(0);
        _turret_instances.resize(0);
    }

    for (auto& outline : _outlines) {
        outline.linearize();
    }

    return true;
}

//------------------------------------------------------------------------------
void ship_editor::export_verts(string::view filename) const
{
    g_Game->message("exporting '%s'...", filename.c_str());

    file::stream s = file::open(filename, file::mode::write);
    if (!s) {
        return;
    }

    s.printf("{\n    ");
    for (std::size_t ii = 0; ii < _outlines[0].linearized().size(); ++ii) {
        s.printf("vec2(%.2ff, %.2ff), ", _outlines[0].linearized()[ii].x, _outlines[0].linearized()[ii].y);
    }
    s.printf("\n}\n");
    for (std::size_t jj = 0; jj < _turrets.size(); ++jj) {
        s.printf("{\n    ");
        for (std::size_t ii = 0; ii < _outlines[_turrets[jj].index].linearized().size(); ++ii) {
            s.printf("vec2(%.2ff, %.2ff), ", _outlines[_turrets[jj].index].linearized()[ii].x, _outlines[_turrets[jj].index].linearized()[ii].y);
        }
        s.printf("\n}\n");
    }
}

} // namespace game

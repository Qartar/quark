// r_draw.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "r_model.h"
#include "win_include.h"
#include <GL/gl.h>

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
void system::draw_string(string::view string, vec2 position, color4 color)
{
    vec2 scale(_view.size.x / _framebuffer_size.x,
               _view.size.y / _framebuffer_size.y);
    _default_font->draw(string, position, color, scale);
}

//------------------------------------------------------------------------------
vec2 system::string_size(string::view string) const
{
    vec2 scale(_view.size.x / _framebuffer_size.x,
               _view.size.y / _framebuffer_size.y);
    return _default_font->size(string, scale);
}

//------------------------------------------------------------------------------
void system::draw_monospace(string::view string, vec2 position, color4 color)
{
    vec2 scale(_view.size.x / _framebuffer_size.x,
               _view.size.y / _framebuffer_size.y);
    _monospace_font->draw(string, position, color, scale);
}

//------------------------------------------------------------------------------
vec2 system::monospace_size(string::view string) const
{
    vec2 scale(_view.size.x / _framebuffer_size.x,
               _view.size.y / _framebuffer_size.y);
    return _monospace_font->size(string, scale);
}

//------------------------------------------------------------------------------
void system::draw_arc(vec2 center, float radius, float width, float min_angle, float max_angle, color4 color)
{
    if (!_view_bounds.intersects_circle(center, radius + width * .5f)) {
        return;
    }

    bounds arc_bounds;
    {
        // minimum angle normalized to (-pi, pi]
        float amin = std::remainder(min_angle, 2.f * math::pi<float>);
        // maximum angle, normalized to (-pi, ...]
        float amax = amin + (max_angle - min_angle);

        vec2 points[2] = {
            center + vec2(std::cos(min_angle), std::sin(min_angle)) * radius,
            center + vec2(std::cos(max_angle), std::sin(max_angle)) * radius,
        };
        arc_bounds = bounds::from_points(points);

        if (amin <= -.5f * math::pi<float> && amax >= -.5f * math::pi<float>) {
            arc_bounds[0][1] = center.y - radius;
        }
        if (amin <= 0.f && amax >= 0.f) {
            arc_bounds[1][0] = center.x + radius;
        }
        if (amin <= .5f * math::pi<float> && amax >= .5f * math::pi<float>) {
            arc_bounds[1][1] = center.y + radius;
        }
        if (amin <= math::pi<float> && amax >= math::pi<float>) {
            arc_bounds[0][0] = center.x - radius;
        }

        arc_bounds = arc_bounds.expand(width * .5f);
    }

    if (!_view_bounds.intersects(arc_bounds)) {
        return;
    }

    // Scaling factor for circle tessellation
    const float view_scale = sqrtf(_framebuffer_size.length_sqr() / _view.size.length_sqr());

    // Number of circle segments, approximation for pi / acos(1 - 1/2x)
    int n = 1 + static_cast<int>(0.5f * (max_angle - min_angle) * sqrtf(max(0.f, radius * view_scale - 0.25f)));
    float step = (max_angle - min_angle) / n;

    glColor4fv(color);

    if (width == 0.f) {
        glBegin(GL_LINE_STRIP);
            for (int ii = 0; ii <= n; ++ii) {
                float a = float(ii) * step + min_angle;
                float s = sinf(a);
                float c = cosf(a);

                glVertex2fv(center + vec2(c, s) * radius);
            }
        glEnd();
    } else {
        glBegin(GL_TRIANGLE_STRIP);
            for (int ii = 0; ii <= n; ++ii) {
                float a = float(ii) * step + min_angle;
                float s = sinf(a);
                float c = cosf(a);

                glVertex2fv(center + vec2(c, s) * (radius + width * .5f));
                glVertex2fv(center + vec2(c, s) * (radius - width * .5f));
            }
        glEnd();
    }
}

//------------------------------------------------------------------------------
void system::draw_box(vec2 size, vec2 position, color4 color)
{
    float   xl, xh, yl, yh;

    glColor4fv(color);

    xl = position.x - size.x / 2;
    xh = position.x + size.x / 2;
    yl = position.y - size.y / 2;
    yh = position.y + size.y / 2;

    glBegin(GL_QUADS);
        glVertex2f(xl, yl);
        glVertex2f(xh, yl);
        glVertex2f(xh, yh);
        glVertex2f(xl, yh);
    glEnd();
}

//------------------------------------------------------------------------------
void system::draw_triangles(vec2 const* position, color4 const* color, int const* indices, std::size_t num_indices)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, position);
    glColorPointer(4, GL_FLOAT, 0, color);

    glDrawElements(GL_TRIANGLES, (GLsizei)num_indices, GL_UNSIGNED_INT, indices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

//------------------------------------------------------------------------------
void system::draw_particles(time_value time, render::particle const* particles, std::size_t num_particles)
{
    // Scaling factor for particle tessellation
    const float view_scale = sqrtf(_framebuffer_size.length_sqr() / _view.size.length_sqr());

    render::particle const* end = particles + num_particles;
    for (render::particle const*p = particles; p < end; ++p) {
        float ptime = (time - p->time).to_seconds();

        if (ptime < 0) {
            continue;
        }

        float vtime = p->drag ? tanhf(p->drag * ptime) / p->drag : ptime;

        float radius = p->size + p->size_velocity * ptime;
        color4 color = p->color + p->color_velocity * ptime;
        vec2 position = p->position
                      + p->velocity * vtime
                      + p->acceleration * 0.5f * vtime * vtime;

        color4 color_in = p->flags & render::particle::invert ? color * color4(1,1,1,0.25f) : color;
        color4 color_out = p->flags & render::particle::invert ? color : color * color4(1,1,1,0.25f);

        glBegin(GL_TRIANGLE_FAN);

        // Number of circle segments, approximation for pi / acos(1 - 1/2x)
        int n = 1 + static_cast<int>(math::pi<float> * sqrtf(max(0.f, radius * view_scale - 0.25f)));
        int k = std::max<int>(1, narrow_cast<int>(countof(_costbl) / n));

        glColor4fv(color_in);
        glVertex2fv(position);

        if (!(p->flags & render::particle::tail)) {
            // draw circle outline
            glColor4fv(color_out);
            for (int ii = 0; ii < countof(_costbl); ii += k) {
                vec2 vertex = position + vec2(_costbl[ii], _sintbl[ii]) * radius;
                glVertex2fv(vertex);
            }
            glVertex2f(position.x + radius, position.y);
        } else {
            float tail_time = std::max<float>(0.0f, (time - p->time - FRAMETIME).to_seconds());
            float tail_vtime = p->drag ? tanhf(p->drag * tail_time) / p->drag : tail_time;

            vec2 tail_position = p->position
                               + p->velocity * tail_vtime
                               + p->acceleration * 0.5f * tail_vtime * tail_vtime;

            // calculate forward and tangent vectors
            vec2 normal = position - tail_position;
            float distance = normal.length();
            normal /= distance;
            distance = std::max<float>(distance, radius);
            vec2 tangent = vec2(-normal.y, normal.x);

            // particle needs at least 4 verts to look reasonable
            int n0 = std::max<int>(4, n);
            int k0 = std::max<int>(1, narrow_cast<int>(countof(_costbl) / n0));

            glColor4fv(color_in);
            for (int ii = 0; ii < countof(_costbl); ii += k0) {
                if (ii < countof(_costbl) / 2) {
                    // draw forward-facing half-circle
                    vec2 vertex = position + (tangent * _costbl[ii] + normal * _sintbl[ii]) * radius;
                    glVertex2fv(vertex);
                } else {
                    // draw backward-facing elliptical tail
                    float alpha = -_sintbl[ii];
                    color4 vcolor = color_out * alpha + color_in * (1.0f - alpha);
                    vec2 vertex = position + tangent * _costbl[ii] * radius + normal * _sintbl[ii] * distance;

                    glColor4fv(vcolor);
                    glVertex2fv(vertex);
                }
            }
            glColor4fv(color_in);
            glVertex2f(position.x + tangent.x * radius, position.y + tangent.y * radius);
        }

        glEnd();
    }
}

//------------------------------------------------------------------------------
void system::draw_line(vec2 start, vec2 end, color4 start_color, color4 end_color)
{
    glBegin(GL_LINES);
        glColor4fv(start_color);
        glVertex2fv(start);
        glColor4fv(end_color);
        glVertex2fv(end);
    glEnd();
}

//------------------------------------------------------------------------------
void system::draw_model(render::model const* model, mat3 tx, color4 color)
{
    if (!_view_bounds.intersects(model->bounds().transform(tx))) {
        return;
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Convert mat3 homogenous transform to mat4
    mat4 m(tx[0][0], tx[0][1], 0, tx[0][2],
           tx[1][0], tx[1][1], 0, tx[1][2],
           0,        0,        1, 0,
           tx[2][0], tx[2][1], 0, tx[2][2]);

    glMultMatrixf((float const*)&m);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glBlendColor(color.r, color.g, color.b, color.a);

    glVertexPointer(2, GL_FLOAT, 0, model->_vertices.data());
    glColorPointer(3, GL_FLOAT, 0, model->_colors.data());
    glDrawElements(GL_TRIANGLES, (GLsizei)model->_indices.size(), GL_UNSIGNED_SHORT, model->_indices.data());

    if (_draw_tris) {
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glDrawElements(GL_TRIANGLES, (GLsizei)model->_indices.size(), GL_UNSIGNED_SHORT, model->_indices.data());

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix();
}

//------------------------------------------------------------------------------
void system::draw_line(float width, vec2 start, vec2 end, color4 start_color, color4 end_color, color4 start_edge_color, color4 end_edge_color)
{
    // Scaling factor for particle tessellation
    const float view_scale = sqrtf(_framebuffer_size.length_sqr() / _view.size.length_sqr());

    vec2 direction = (end - start).normalize() * .5f * width;
    vec2 normal = direction.cross(1.f);

    glBegin(GL_TRIANGLE_STRIP);
        glColor4fv(start_edge_color);
        glVertex2fv(start - normal);
        glColor4fv(end_edge_color);
        glVertex2fv(end - normal);

        glColor4fv(start_color);
        glVertex2fv(start);
        glColor4fv(end_color);
        glVertex2fv(end);

        glColor4fv(start_edge_color);
        glVertex2fv(start + normal);
        glColor4fv(end_edge_color);
        glVertex2fv(end + normal);
    glEnd();

    // Number of circle segments, approximation for pi / acos(1 - 1/2x)
    int n = 1 + static_cast<int>(math::pi<float> * sqrtf(max(0.f, width * view_scale - 0.25f)));
    int k = std::max<int>(4, 360 / n);

    // Draw half-circle at start
    if ((start_color.a || start_edge_color.a)
            && _view_bounds.intersects_circle(start, width * .5f)) {
        glBegin(GL_TRIANGLE_FAN);
            glColor4fv(start_color);
            glVertex2fv(start);
            glColor4fv(start_edge_color);
            glVertex2fv(start + normal);
            for (int ii = k; ii < 180 ; ii += k) {
                vec2 vertex = start + normal * _costbl[ii] - direction * _sintbl[ii];
                glVertex2fv(vertex);
            }
            glVertex2fv(start - normal);
        glEnd();
    }

    // Draw half-circle at end
    if ((end_color.a || end_edge_color.a)
            && _view_bounds.intersects_circle(end, width * .5f)) {
        glBegin(GL_TRIANGLE_FAN);
            glColor4fv(end_color);
            glVertex2fv(end);
            glColor4fv(end_edge_color);
            glVertex2fv(end - normal);
            for (int ii = k; ii < 180 ; ii += k) {
                vec2 vertex = end - normal * _costbl[ii] + direction * _sintbl[ii];
                glVertex2fv(vertex);
            }
            glVertex2fv(end + normal);
        glEnd();
    }

    // Aliasing causes thin lines to not render at full brightness
    // when rendering lines with width less than one pixel, draw a
    // line primitive on top with the center color to compensate.
    if (start_color != start_edge_color || end_color != end_edge_color) {
        draw_line(start, end, start_color, end_color);
    }
}

//------------------------------------------------------------------------------
namespace {

vec3 const grad3[12] = {
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, 1, -1), vec3(0, -1, -1)
};

int const perm[512] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
//  repeat
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
};

double dot(vec3 p, double a, double b)
{
    return p.x * a + p.y * b;
}

float simplex(vec2 P)
{
    constexpr float sqrt_three = 1.73205080756887729352f;
    double  n0, n1, n2;

    //  skew input space to determine simplex cell
    double const F2 = 0.5 * (sqrt_three - 1.0f);
    double s = (P.x + P.y) * F2;
    int i = int(floor(P.x + s));
    int j = int(floor(P.y + s));

    //  unskew back into xy space
    double const G2 = (3.0 - sqrt_three) / 6.0;
    double t = (i + j) * G2;
    double X0 = i - t;
    double Y0 = j - t;
    double x0 = P.x - X0;
    double y0 = P.y - Y0;

    //  determine simplex
    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    //  coordinates of other corners
    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + G2 * 2.0;
    double y2 = y0 - 1.0 + G2 * 2.0;

    //  work out hashed gradient indices
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = perm[ii + perm[jj]] % 12;
    int gi1 = perm[(ii + i1 + perm[jj + j1]) & 255] % 12;
    int gi2 = perm[(ii + 1 + perm[jj + 1]) & 255] % 12;

    //  calculate contributions
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0.0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0);
    }

    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0.0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
    }

    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0.0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
    }

    //  sum and scale to range [-1,1]
    return float(70.0 * (n0 + n1 + n2));
}

} // anonymous namespace

//------------------------------------------------------------------------------
void system::draw_starfield(vec2 streak_vector)
{
#if 0
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glPointSize(0.1f);
    glBlendFunc(GL_CONSTANT_COLOR, GL_ONE);

    float s = std::log2(_view.size.x);
    float i = std::exp2(std::floor(s));
    float scale[5] = {
        i * .5f,
        i,
        i * 2.f,
        i * 4.f,
        i * 8.f,
    };

    for (int ii = 0; ii < 5; ++ii) {
        float r = (std::floor(s) + ii) * 145.f;

        vec2 p = _view.origin * mat2::rotate(math::deg2rad(-r));

        float tx = -p.x / scale[ii];
        float ty = -p.y / scale[ii];

        float ix = std::floor(tx);
        float iy = std::floor(ty);

        float a = ii == 0 ? square(std::ceil(s) - s)
                : ii == 4 ? square(s - std::floor(s)) : 1.f;

        glBlendColor(a, a, a, 1.f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        if (streak_vector != vec2_zero) {
            vec2 u = streak_vector;
            vec2 v = u * -.5f;

            float shear[16] = {
                1.f, 0.f, 0.f, 0.f,
                0.f, 1.f, 0.f, 0.f,
                u.x, u.y, 1.f, 0.f,
                v.x, v.y, 0.f, 1.f,
            };
            glMultMatrixf(shear);
        }

        glTranslatef(_view.origin.x, _view.origin.y, 0);
        glScalef(scale[ii], scale[ii], 1);
        glRotatef(r, 0, 0, 1);

        for (int xx = 0; xx < 3; ++xx) {
            for (int yy = 0; yy < 3; ++yy) {
                glPushMatrix();
                glTranslatef(tx - ix + xx - 2, ty - iy + yy - 2, 0);

                if (streak_vector != vec2_zero) {
                    glVertexPointer(3, GL_FLOAT, 0, _starfield_points.data());
                    glColorPointer(3, GL_FLOAT, 0, _starfield_colors.data());
                    glDrawArrays(GL_LINES, 0, (GLsizei)_starfield_points.size());
                }

                glVertexPointer(3, GL_FLOAT, sizeof(vec3) * 2, _starfield_points.data());
                glColorPointer(3, GL_FLOAT, sizeof(color3) * 2, _starfield_colors.data());
                glDrawArrays(GL_POINTS, 0, (GLsizei)_starfield_points.size() / 2);

                glPopMatrix();
            }
        }

        glPopMatrix();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(2.f);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
#else
    (void)streak_vector;

    float xmin = _view.origin.x - _view.size.x * .5f;
    float xmax = _view.origin.x + _view.size.x * .5f;
    float ymin = _view.origin.y - _view.size.y * .5f;
    float ymax = _view.origin.y + _view.size.y * .5f;

    constexpr int sz = 16;

    float dx = sz * (xmax - xmin) / _framebuffer_size.x;
    float dy = sz * (ymax - ymin) / _framebuffer_size.y;

    auto const noise = [](vec2 v) {
        return -.25f * simplex(v * .001f * .5f)
             - .5f * simplex(vec2(-9, 17) + v * .0005f * .5f)
             - 1.f * simplex(vec2(13, 7) + v * .00025f * .5f)
             - 2.f * simplex(vec2(29, -51) + v * .000125f * .5f)
             - 1.f * simplex(vec2(7, 13) + v * .0000625f * .5f)
             - 1.f * simplex(vec2(7, 13) + v * .00003125f * .5f)
             - 1.5f
             - v.length_sqr() * .00000000015f;
    };

    static tree my_tree;
    static bool initialized = false;
    if (!initialized) {
        //my_tree.build({{-131072.f, -131072.f}, {131072.f, 131072.f}}, 64.f, noise);
        my_tree.build({{-16384.f, -16384.f}, {16384.f, 16384.f}}, 64.f, noise);
        //my_tree.build({_view.origin - _view.size, _view.origin + _view.size}, dx, noise);
        initialized = true;
    }
    my_tree.draw(this, {_view.origin - _view.size * .5f, _view.origin + _view.size * .5f}, .125f * (dx + dy));

    {
        for (int ii = 0; ii < 4; ++ii) {
            bounds aabb{_view.origin - _view.size * .05f, _view.origin + _view.size * .05f};
            aabb += _view.size * vec2((ii & 1) ? -.35f : .35f, (ii & 2) ? -.15f : .15f);

            glBegin(GL_LINE_LOOP);
            bool is_above = my_tree.is_above(aabb);
            bool is_below = my_tree.is_below(aabb);
            if (is_above && is_below) {
                glColor4f(1,1,1,1);
            } else if (is_above) {
                glColor4f(.5f,1,.5f,1);
            } else if (is_below) {
                glColor4f(.5f,.5f,1,1);
            } else {
                glColor4f(1,.75f,.5f,1);
            }
            glVertex2f(aabb[0][0], aabb[0][1]);
            glVertex2f(aabb[1][0], aabb[0][1]);
            glVertex2f(aabb[1][0], aabb[1][1]);
            glVertex2f(aabb[0][0], aabb[1][1]);
            glEnd();
        }
    }

#if 1
    (void)dx;
    (void)dy;
#else
    glBegin(GL_LINES);
    glColor4f(1,1,1,1);

    for (int jj = 0; sz * jj < _framebuffer_size.y; ++jj) {
        float y0 = ymin + dy * jj;

        for (int ii = 0; sz * ii < _framebuffer_size.x; ++ii) {
            float x0 = xmin + dx * ii;

            float h00, h10;
            float h01, h11;

            h00 = noise(vec2(x0,      y0     ));
            h10 = noise(vec2(x0 + dx, y0     ));
            h01 = noise(vec2(x0,      y0 + dy));
            h11 = noise(vec2(x0 + dx, y0 + dy));

            /*
                h00 h10
                h01 h11

                --      +-      ++      ++      +-      ++
                --      --      --      -+      -+      ++

                h0 + (h1 - h0)t = 0
                t = -h0 / (h1 - h0)

            */

            float x2 = x0 - dx * h00 / (h10 - h00);
            float x3 = x0 - dx * h01 / (h11 - h01);
            float y2 = y0 - dy * h00 / (h01 - h00);
            float y3 = y0 - dy * h10 / (h11 - h10);

            /*

                (x0, y0)        (x2, y0)        (x0 + dx, y0)

                (x0, y2)                        (x0 + dx, y3)

                (x0, y0 + dy)   (x3, y0 + dy)   (x0 + dx, y0 + dy)

            */

            int mask = ((h00 < 0.f) ? 1 : 0)
                     | ((h10 < 0.f) ? 2 : 0)
                     | ((h01 < 0.f) ? 4 : 0)
                     | ((h11 < 0.f) ? 8 : 0);

            switch (mask) {
            case 1:
            case 15 ^ 1:
                glVertex2f(x0, y2);
                glVertex2f(x2, y0);
                break;

            case 2:
            case 15 ^ 2:
                glVertex2f(x2, y0);
                glVertex2f(x0 + dx, y3);
                break;

            case 4:
            case 15 ^ 4:
                glVertex2f(x0, y2);
                glVertex2f(x3, y0 + dy);
                break;

            case 8:
            case 15 ^ 8:
                glVertex2f(x3, y0 + dy);
                glVertex2f(x0 + dx, y3);
                break;

            case 1 | 2:
            case 4 | 8:
                glVertex2f(x0, y2);
                glVertex2f(x0 + dx, y3);
                break;

            case 1 | 4:
            case 2 | 8:
                glVertex2f(x2, y0);
                glVertex2f(x3, y0 + dx);
                break;

            case 1 | 8:
                glVertex2f(x2, y0);
                glVertex2f(x0 + dx, y3);
                glVertex2f(x0, y2);
                glVertex2f(x3, y0 + dy);
                break;

            case 2 | 4:
                glVertex2f(x0, y2);
                glVertex2f(x2, y0);
                glVertex2f(x3, y0 + dy);
                glVertex2f(x0 + dx, y3);
                break;
            }
        }
    }
    glEnd();
#endif

#if 0
    glBegin(GL_TRIANGLES);
    for (int jj = 0; sz * jj < _framebuffer_size.y; ++jj) {
        float y0 = ymin + dy * jj;

        for (int ii = 0; sz * ii < _framebuffer_size.x; ++ii) {
            float x0 = xmin + dx * ii;

            float h0 = noise(vec2(x0,      y0     ));

            if (h0 < 0.f) {
                glColor4f(0,0,1,.1f);
            } else {
                glColor4f(0,1,0,.1f);
            }

            glVertex2f(x0 - dx * .5f, y0 - dy * .5f);
            glVertex2f(x0 + dx * .5f, y0 - dy * .5f);
            glVertex2f(x0 + dx * .5f, y0 + dy * .5f);

            glVertex2f(x0 - dx * .5f, y0 - dy * .5f);
            glVertex2f(x0 + dx * .5f, y0 + dy * .5f);
            glVertex2f(x0 - dx * .5f, y0 + dy * .5f);
        }
    }
    glEnd();
#endif

#endif
}

} // namespace render

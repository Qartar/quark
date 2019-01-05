// r_draw.cpp
//

#include "precompiled.h"
#pragma hdrstop

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
    if (start_color.a || start_edge_color.a) {
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
    if (end_color.a || end_edge_color.a) {
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
void system::draw_starfield(vec2 streak_vector)
{
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
}

} // namespace render

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

} // namespace render

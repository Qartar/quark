// game/g_globe.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_globe.h"
#include "cm_keys.h"

#include "render/gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

namespace {
using PFNGLMULTIDRAWARRAYS = void (APIENTRY*)(GLenum mode, GLint const* first, GLsizei const* count, GLsizei drawcount);
PFNGLMULTIDRAWARRAYS glMultiDrawArrays;
} // anonymous namespace

//------------------------------------------------------------------------------
globe::globe()
{
    _gshhg[0].load(gshhg::resolution::full);
    _gshhg[1].load(gshhg::resolution::high);
    _gshhg[2].load(gshhg::resolution::intermediate);
    _gshhg[3].load(gshhg::resolution::low);
    _gshhg[4].load(gshhg::resolution::crude);
}

//------------------------------------------------------------------------------
void globe::init()
{
    glMultiDrawArrays = (PFNGLMULTIDRAWARRAYS )wglGetProcAddress("glMultiDrawArrays");

    for (int ii = 0; ii < 5; ++ii ) {
        _vbo[ii] = render::gl::vertex_buffer<vec3f>(
            render::gl::buffer_usage::static_,
            render::gl::buffer_access::draw,
            _gshhg[ii].vertices().size(),
            _gshhg[ii].vertices().data());
        _vao[ii] = render::gl::vertex_array({
            render::gl::vertex_array_attrib{3, GL_FLOAT, render::gl::vertex_attrib_type::float_, 0}});
        _vao[ii].bind_buffer(_vbo[ii], 0);

        _first[ii].clear();
        _count[ii].clear();

        for (std::size_t jj = 0; jj < _gshhg[ii].polygons().size(); ++jj) {
            // Skip everything except islands/continents (1) and Antarctic ice-front (5)
            if ((_gshhg[ii].polygons()[jj].flags & 255) != 1 && (_gshhg[ii].polygons()[jj].flags & 255) != 5) {
                continue;
            }
            _first[ii].push_back(_gshhg[ii].polygons()[jj].start);
            _count[ii].push_back(_gshhg[ii].polygons()[jj].count);
        }
    }

    vec2f outline[4096];
    constexpr float dd = math::twopi / float(countof(outline));
    for (std::size_t ii = 0; ii < countof(outline); ++ii) {
        float c = std::cos(float(ii) * dd);
        float s = std::sin(float(ii) * dd);
        outline[ii] = vec2f(c * float(mean_radius), s * float(mean_radius));
    }

    _outline_vbo = render::gl::vertex_buffer<vec2f>(
        render::gl::buffer_usage::static_,
        render::gl::buffer_access::draw,
        countof(outline),
        outline);
    _outline_vao = render::gl::vertex_array({
        render::gl::vertex_array_attrib{2, GL_FLOAT, render::gl::vertex_attrib_type::float_, 0}});
    _outline_vao.bind_buffer(_outline_vbo, 0);
}

//------------------------------------------------------------------------------
void globe::draw(render::system* renderer, time_value /*time*/) const
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    int res = clamp<int>(3e-1 * std::log2(renderer->view().size.length()) - 4.5, 0, 4);

    //
    // draw outline
    //

    glLoadIdentity();
    _outline_vao.bind();
    glColor4f(.1f,.2f,.4f,1);
    glDrawArrays(GL_TRIANGLE_FAN, 0, narrow_cast<GLsizei>(_outline_vbo.num_elements()));
    glColor4f(1,1,1,.5f);
    glDrawArrays(GL_LINE_LOOP, 0, narrow_cast<GLsizei>(_outline_vbo.num_elements()));
    // FIXME: hard-coded depth range
    glClearDepth((mean_radius + 99999.0) / (9999999.0 + 99999.0));
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //
    // draw edges
    //

    glLoadMatrixd(renderer->view().transform);
    glColor4f(1,1,1,.5f);

    _vao[res].bind();
    glMultiDrawArrays(
        GL_LINE_LOOP,
        _first[res].data(),
        _count[res].data(),
        narrow_cast<GLsizei>(_count[res].size()));
    render::gl::vertex_array().bind();

    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
}

//------------------------------------------------------------------------------
vec3 globe::lonlat_to_surface(vec2 lonlat)
{
    double cl = cos(lonlat.x);
    double sl = sin(lonlat.x);
    double cp = cos(lonlat.y);
    double sp = sin(lonlat.y);

    // Using spherical globe approximation (not ellipsoidal)
    return mean_radius * vec3(cl * cp, sl * cp, sp);
}

//------------------------------------------------------------------------------
vec2 globe::surface_to_lonlat(vec3 surface)
{
    // Using spherical globe approximation (not ellipsoidal)
    return vec2(
        atan2(surface.y, surface.x),
        atan2(surface.z, sqrt(surface.x * surface.x + surface.y * surface.y)));
}

//------------------------------------------------------------------------------
mat4 globe::surface_projection(vec3 surface)
{
    // Using spherical globe approximation (not ellipsoidal)
    vec3 z = surface.normalize();
    vec3 x = cross(vec3(0,0,1), z).normalize();
    vec3 y = cross(z, x);

    vec3 t = mean_radius * z;

    return mat4(x.x, x.y, x.z, 0,
                y.x, y.y, y.z, 0,
                z.x, z.y, z.z, 0,
                t.x, t.y, t.z, 1);
}

//------------------------------------------------------------------------------
mat4 globe::surface_inverse_projection(vec3 surface)
{
    // Using spherical globe approximation (not ellipsoidal)
    vec3 z = surface.normalize();
    vec3 x = cross(vec3(0,0,1), z).normalize();
    vec3 y = cross(z, x);

    vec3 p = mean_radius * -z;
    vec3 t = p * mat3(x, y, z).transpose();

    return mat4(x.x, y.x, z.x, 0,
                x.y, y.y, z.y, 0,
                x.z, y.z, z.z, 0,
                t.x, t.y, t.z, 1);
}

//------------------------------------------------------------------------------
double globe::intersect(vec3 start, vec3 direction)
{
    // Using spherical globe approximation (not ellipsoidal)
    double a = dot(direction, direction);
    double b = 2.0 * dot(start, direction);
    double c = dot(start, start) - mean_radius * mean_radius;
    double d = b * b - 4.0 * a * c;

    if (d < 0.0) {
        return DBL_MAX;
    } else {
        double q = -0.5 * (b + std::copysign(std::sqrt(d), b));
        return std::min(q / a, c / q);
    }
}

//------------------------------------------------------------------------------
double globe::altitude(vec3 point)
{
    // Using spherical globe approximation (not ellipsoidal)
    return length(point) - mean_radius;
}

//------------------------------------------------------------------------------
vec3 globe::gravity(vec3 point)
{
    double rsqr = length_sqr(point);
    // Assumes |r| >= mean_radius
    return vec3(-point * GM / (rsqr * std::sqrt(rsqr)));
}

//------------------------------------------------------------------------------
vec3 globe::gravity_normal(vec3 point)
{
    // Using spherical globe approximation (not ellipsoidal)
    return normalize(-point);
}

//------------------------------------------------------------------------------
double globe::distance(vec3 a, vec3 b)
{
    return mean_radius * std::atan2(length(cross(a, b)), dot(a, b));
}

//------------------------------------------------------------------------------
rot2 globe::heading(vec3 position, rot3 rotation)
{
    // Using spherical globe approximation (not ellipsoidal)
    vec3 z = position.normalize();
    vec3 x = cross(vec3(0,0,1), z); // Note: not necessarily unit-length
    vec3 y = cross(z, x);

    vec3 forward = vec3(1,0,0) * rotation;
    return rot2(dot(forward, x), dot(forward, y));
}

//------------------------------------------------------------------------------
rot2 globe::bearing(vec3 position, vec3 target)
{
    // Using spherical globe approximation (not ellipsoidal)
    vec3 z = position.normalize();
    vec3 x = cross(vec3(0,0,1), z); // Note: not necessarily unit-length
    vec3 y = cross(z, x);

    vec3 direction = target - position;
    vec2 projection = normalize(vec2(dot(direction, x), dot(direction, y)));
    return rot2(projection.x, projection.y);
}

//------------------------------------------------------------------------------
void globe::offset(vec3 position, vec2 const* cardinal_offset, vec3* offset_position, std::size_t size)
{
    // Using spherical globe approximation (not ellipsoidal)
    vec3 z = position.normalize();
    vec3 x = cross(vec3(0,0,1), z).normalize();
    vec3 y = cross(z, x);

    for (std::size_t ii = 0; ii < size; ++ii) {
        double s = length(cardinal_offset[ii]);
        double d = mean_radius * std::tan(s * (1.0 / mean_radius));
        vec3 v = x * cardinal_offset[ii].x + y * cardinal_offset[ii].y;
        offset_position[ii] = normalize(position + v * (d / s)) * mean_radius;
    }
}

//------------------------------------------------------------------------------
vec3 globe::offset(vec3 position, vec2 cardinal_offset)
{
    vec3 offset_position;
    offset(position, &cardinal_offset, &offset_position, 1);
    return offset_position;
}

} // namespace game

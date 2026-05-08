// game/g_globe.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_globe.h"
#include "cm_keys.h"

#include "render/gl/gl_include.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
globe::globe()
    : _resolution(0)
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
    for (int ii = 0; ii < 5; ++ii ) {
        _vbo[ii] = render::gl::vertex_buffer<vec3f>(
            render::gl::buffer_usage::static_,
            render::gl::buffer_access::draw,
            _gshhg[ii].vertices().size(),
            _gshhg[ii].vertices().data());
        _vao[ii] = render::gl::vertex_array({
            render::gl::vertex_array_attrib{3, GL_FLOAT, render::gl::vertex_attrib_type::float_, 0}});
        _vao[ii].bind_buffer(_vbo[ii], 0);
    }
}

//------------------------------------------------------------------------------
void globe::draw(render::system* renderer, time_value /*time*/) const
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(renderer->view().transform);

    _vao[_resolution].bind();
    glColor4f(1,1,1,.5f);
    for (std::size_t ii = 0; ii < _gshhg[_resolution].polygons().size(); ++ii) {
        // Skip everything except islands/continents (1) and Antarctic ice-front (5)
        if ((_gshhg[_resolution].polygons()[ii].flags & 255) != 1 && (_gshhg[_resolution].polygons()[ii].flags & 255) != 5) {
            continue;
        }
        glDrawArrays(
            GL_LINE_LOOP,
            _gshhg[_resolution].polygons()[ii].start,
            _gshhg[_resolution].polygons()[ii].count);
    }
    glPopMatrix();
    render::gl::vertex_array().bind();
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
vec3 globe::planar_to_surface(vec2 v)
{
    static constexpr vec2 offset = vec2(117.9167, -1.95) * (math::pi / 180.0); // Makassar Strait
    // Pretend x/y are lon/lat
    return lonlat_to_surface(v * (1.f / mean_radius) + offset);
}

} // namespace game

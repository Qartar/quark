// game/g_globe.h
//

#pragma once

#include "cm_time.h"
#include "cm_vector.h"
#include "cm_gshhg.h"

#include "render/gl/gl_buffer.h"
#include "render/gl/gl_vertex_array.h"

namespace render {
class system;
} // namespace render

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
// Visual test harness for topography data
class globe
{
public:
    globe();

    //! Initialize rendering data
    void init();
    void draw(render::system* renderer, time_value time) const;

    //! Convert longitude/latitude in radians into cartesian coordinates of a point on the surface
    static vec3 lonlat_to_surface(vec2 lonlat);
    //! Convert cartesian coordinates of a point on the surface into longitude/latitude in radians
    static vec2 surface_to_lonlat(vec3 surface);
    //! Create an orthogonal projection matrix centered on the given point on the suface
    static mat4 surface_projection(vec3 surface);
    //! Create an orthogonal inverse projection matrix centered on the given point on the surface
    static mat4 surface_inverse_projection(vec3 surface);

    //! Convert legacy 2D coordinates to 3D surface coordinates
    static vec3 planar_to_surface(vec2 v);

protected:
    gshhg _gshhg[5];

    render::gl::vertex_buffer<vec3f> _vbo[5];
    render::gl::vertex_array _vao[5];

    std::vector<GLint> _first[5];
    std::vector<GLsizei> _count[5];

    int _resolution;

protected:
    //! IUGG arithmetic mean radius of the Earth. Making this protected because
    //! if a calculation is relying on this value it should probably be inlined
    //! into this class.
    static constexpr double mean_radius = 6371008.7714;
};

} // namespace game

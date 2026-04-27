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

    void draw(render::system* renderer, time_value time);

    bool key_event(int key, bool down);
    void cursor_event(vec2 position);

protected:
    gshhg _gshhg[5];

    render::gl::vertex_buffer<gshhg::point> _vbo[5];
    render::gl::vertex_array _vao[5];

    int _resolution;

    double _longitude;
    double _latitude;

    float _zoom;

    bool _is_dragging;
    vec2 _cursor;
};

} // namespace game

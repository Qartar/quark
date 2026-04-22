// game/g_globe.h
//

#pragma once

#include "cm_gtopo30.h"
#include "cm_color.h"
#include "cm_time.h"
#include "cm_vector.h"
#include "cm_gshhg.h"

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
    gshhg _gshhg;

#if 0
    gtopo30 _topo;
#endif

    std::vector<vec2> _vertices;
    std::vector<color4> _colors;
    std::vector<int> _indices;

    float _longitude;
    float _latitude;

    float _zoom;

    bool _is_dirty;
    bool _is_dragging;
    vec2 _cursor;

    static constexpr int X = 64 * 4;
    static constexpr int Y = 36 * 4;

protected:
    void resample();
};

} // namespace game

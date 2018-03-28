// r_model.cpp
//

#include "precompiled.h"
#pragma hdrstop

render::model tank_body_model({
    //  center        size          gamma
    {{  0.f,  8.0f}, {20.f,  2.f},  0.5f},  // left tread
    {{  0.f, -8.0f}, {20.f,  2.f},  0.5f},  // right tread
    {{  0.f,  0.0f}, {24.f, 16.f},  0.8f},  // main chassis
    {{- 8.f,  0.0f}, { 4.f, 12.f},  0.6f},  // whatever
    {{-12.f,  5.0f}, { 3.f,  6.f},  0.5f},  // left barrel
    {{-12.f, -5.0f}, { 3.f,  6.f},  0.5f},  // right barrel
    {{ 10.f,  0.0f}, { 2.f, 12.f},  0.6f},
    {{  7.f,  0.0f}, { 2.f, 12.f},  0.6f},
});

render::model tank_turret_model({
    //  center        size          gamma
    {{  0.f,  0.f},  { 8.f,  10.f}, 1.0f},  // turret
    {{  0.f,  0.f},  {10.f,   8.f}, 1.0f},  // turret
    {{ 13.f,  0.f},  {16.f,   2.f}, 1.0f},  // barrel
});

render::model ship_model({
    //  center          size        gamma
    {{  2.f,  0.f},  {18.f, 12.f}, 0.5f},
    {{  2.f,  0.f},  {16.f, 14.f}, 0.5f},
    {{  2.f,  0.f},  {12.f, 16.f}, 0.5f},
    {{ -2.f,  7.f},  {14.f,  4.f}, 0.5f},
    {{ -2.f, -7.f},  {14.f,  4.f}, 0.5f},
    {{-15.f,  8.f},  { 2.f,  4.f}, 0.8f},
    {{-15.f, -8.f},  { 2.f,  4.f}, 0.8f},
    {{-16.f,  8.f},  { 2.f,  2.f}, 0.7f},
    {{-16.f, -8.f},  { 2.f,  2.f}, 0.7f},
    {{ -8.f,  8.f},  {14.f,  4.f}, 0.6f},
    {{ -8.f, -8.f},  {14.f,  4.f}, 0.6f},
});

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
model::model(model::rect const* rects, std::size_t num_rects)
    : _mins(0,0)
    , _maxs(0,0)
{
    for (std::size_t ii = 0; ii < num_rects; ++ii) {
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 0));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 1));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 2));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 1));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 3));
        _indices.push_back(narrow_cast<uint16_t>(_vertices.size() + 2));

        vec2 mins = rects[ii].center - rects[ii].size * 0.5f;
        vec2 maxs = rects[ii].center + rects[ii].size * 0.5f;

        _vertices.emplace_back(mins.x, mins.y);
        _vertices.emplace_back(maxs.x, mins.y);
        _vertices.emplace_back(mins.x, maxs.y);
        _vertices.emplace_back(maxs.x, maxs.y);

        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);
        _colors.emplace_back(rects[ii].gamma, rects[ii].gamma, rects[ii].gamma);

        // calculate absolute mins/maxs
        for (int jj = 0; jj < 2; ++jj) {
            if (_mins[jj] > mins[jj]) {
                _mins[jj] = mins[jj];
            }
            if (_maxs[jj] < maxs[jj]) {
                _maxs[jj] = maxs[jj];
            }
        }
    }
}

} // namespace render

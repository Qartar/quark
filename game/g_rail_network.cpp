// g_rail_network.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_rail_network.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
rail_network::rail_network(world* w)
    : _world(w)
{
}

//------------------------------------------------------------------------------
rail_network::~rail_network()
{
}

//------------------------------------------------------------------------------
void rail_network::draw(render::system* renderer, time_value time) const
{
    (void)time;
    render::view const& view = renderer->view();
    //float diag = view.size.length() / float(view.viewport.size().length());
    float diag = 16.f * view.size.length() / float(renderer->window()->size().length());
    for (auto edge : _network.edges()) {
        // skip opposite edges
        if (edge & 1) {
            continue;
        }
        clothoid::segment s = _network.get_segment(edge);
        if (s.type() == clothoid::segment_type::line) {
            renderer->draw_line(
                s.initial_position(),
                s.final_position(),
                color4(1,1,1,1),
                color4(1,1,1,1));
        } else {
            float n = max(2.f, s.length() / diag);
            vec2 p0 = s.initial_position();
            for (float ii = 1.f; ii < n; ii += 1.f) {
                vec2 p1 = s.evaluate(ii / n * s.length());
                renderer->draw_line(p0, p1, color4(1,1,1,1), color4(1,1,1,1));
                p0 = p1;
            }
            renderer->draw_line(p0, s.final_position(), color4(1,1,1,1), color4(1,1,1,1));
        }
    }
}

//------------------------------------------------------------------------------
void rail_network::add_segment(clothoid::segment s)
{
    _network.insert_edge(s);
}

//------------------------------------------------------------------------------
handle<rail_signal> rail_network::add_signal(vec2 position)
{
    (void)position;
    return handle<rail_signal>();
}

//------------------------------------------------------------------------------
handle<rail_station> rail_network::add_station(vec2 position, string::view name)
{
    (void)position;
    (void)name;
    clothoid::network::edge_index edge;
    float dist;
    if (get_closest_segment(position, 1.f, edge, dist)) {
        //_world->spawn<rail_station>(edge, dist, name);
        return handle<rail_station>();
    } else {
        return handle<rail_station>();
    }
}

//------------------------------------------------------------------------------
clothoid::segment rail_network::get_segment(clothoid::network::edge_index edge) const
{
    return _network.get_segment(edge);
}

//------------------------------------------------------------------------------
bool rail_network::get_closest_segment(
    vec2 position,
    float max_distance,
    clothoid::network::edge_index& edge,
    float& length) const
{
    return _network.get_closest_segment(
        position,
        max_distance,
        edge,
        length);
}

} // namespace game

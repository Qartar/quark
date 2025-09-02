// g_rail_network.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_rail_network.h"
#include "g_rail_station.h"
#include "g_train.h"

#include <set>

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
void rail_network::clear()
{
    _signals.clear();
    _signal_block_index.clear();
    _stations.clear();
    _trains.clear();

    _network = {};

    _signal_blocks.clear();

    _clearance.clear();
}

//------------------------------------------------------------------------------
void rail_network::draw(render::system* renderer, time_value time) const
{
    (void)time;

    // draw clearance
    for (auto& kv : _clearance) {
        auto seg = _network.get_segment(kv.first.first);
        vec2 p = seg.evaluate(kv.second);
        vec2 t = seg.evaluate_tangent(kv.second);
        renderer->draw_line(p + t.cross(2), p - t.cross(2), color4(1,1,1,1), color4(1,1,1,1));
    }

    // draw edges
    for (auto edge : _network.edges()) {
        // skip opposite edges
        if (edge & 1) {
            continue;
        }
        draw_segment(renderer, _network.get_segment(edge));
    }

#if 0
    // draw nodes
    for (auto node : _network.nodes()) {
        vec2 p = _network.node_position(node);
        renderer->draw_arc(p, 1, 0, 0, 2.f * math::pi<float>, color4(1,.5f,0,1));
        renderer->draw_string(va("%d", node), p, color4(1,.5f,0,1));
    }
#endif
}

//------------------------------------------------------------------------------
void rail_network::draw_segment(render::system* renderer, clothoid::segment s) const
{
    render::view const& view = renderer->view();
    //float diag = view.size.length() / float(view.viewport.size().length());
    //float diag = 16.f * view.size.length() / float(renderer->window()->size().length());
    vec2 to_pixels = vec2(renderer->window()->size()) / view.size;

    auto curvature_color = [](float k) {
#if 1
        (void)k;
        return color4(.5f,.5f,.5f,1);
#else // color by curvature
        float vsqr = 4.f/*train::max_lateral_acceleration*/ / abs(k);
        if (vsqr > square(50.f/*train::max_speed*/)) {
            return color4(.5f,.5f,.5f,1);
        } else {
            float v = sqrt(vsqr);
            if (v > 35.f) {
                float t = (v - 35.f) / 15.f;
                return color4(.5f,.5f,.5f,1) * t + color4(.5f,.5f,0,1) * (1.f - t);
            } else {
                float t = v / 35.f;
                return color4(.5f,.5f,0,1) * t + color4(.5f,0,0,1) * (1.f - t);
            }
        }
#endif
    };

#if 0 // Draw segment bounding triangles
    if (s.type() == clothoid::segment_type::arc) {
        vec2 v1, v2, v3;
        s.bounding_triangle(v1, v2, v3);
        renderer->draw_line(v1, v2, color4(.6f,1,0,.5f), color4(.6f,1,0,.5f));
        renderer->draw_line(v2, v3, color4(.6f,1,0,.5f), color4(.6f,1,0,.5f));
        renderer->draw_line(v3, v1, color4(.6f,1,0,.5f), color4(.6f,1,0,.5f));
        renderer->draw_box(vec2(4.f), v1, color4(0,1,0,1));
        renderer->draw_box(vec2(4.f), v2, color4(0,1,0,1));
        renderer->draw_box(vec2(4.f), v3, color4(0,1,0,1));
    } else if (s.type() == clothoid::segment_type::transition) {
        vec2 v1, v2, v3;
        s.bounding_triangle(v1, v2, v3);
        renderer->draw_line(v1, v2, color4(0,1,.6f,.5f), color4(0,1,.6f,.5f));
        renderer->draw_line(v2, v3, color4(0,1,.6f,.5f), color4(0,1,.6f,.5f));
        renderer->draw_line(v3, v1, color4(0,1,.6f,.5f), color4(0,1,.6f,.5f));
        renderer->draw_box(vec2(4.f), v1, color4(0,1,0,1));
        renderer->draw_box(vec2(4.f), v2, color4(0,1,0,1));
        renderer->draw_box(vec2(4.f), v3, color4(0,1,0,1));
    }
#endif

    if (s.type() == clothoid::segment_type::line) {
        renderer->draw_line(
            s.initial_position() + s.initial_tangent().cross(.75f),
            s.final_position() + s.final_tangent().cross(.75f),
            curvature_color(0.f),
            curvature_color(0.f));
        renderer->draw_line(
            s.initial_position() - s.initial_tangent().cross(.75f),
            s.final_position() - s.final_tangent().cross(.75f),
            curvature_color(0.f),
            curvature_color(0.f));
    } else {
        vec2 p0 = s.initial_position();
        vec2 t0 = s.initial_tangent();
        vec2 p1 = s.final_position();
        vec2 t1 = s.final_tangent();
        float s0 = 0.f;
        float s1 = s.length();

        float k0 = s.initial_curvature();
        float k1 = s.final_curvature();

        while (true) {
            float s2 = .5f * (s0 + s1);
            vec2 p2 = s.evaluate(s2);
            vec2 t2 = s.evaluate_tangent(s2);
            float k2 = s.evaluate_curvature(s2);
            // projection of p2 onto (p1 - p0)
            vec2 p3 = p0 + (p1 - p0) * dot(p2 - p0, p1 - p0) / (p1 - p0).length_sqr();
            float dsqr = ((p2 - p3) * to_pixels).length_sqr();
            if (dsqr < 1.f) {
                renderer->draw_line(
                    p1 + t1.cross(.75f),
                    p2 + t2.cross(.75f),
                    curvature_color(k1),
                    curvature_color(k2));
                renderer->draw_line(
                    p1 - t1.cross(.75f),
                    p2 - t2.cross(.75f),
                    curvature_color(k1),
                    curvature_color(k2));
                p1 = p2;
                t1 = t2;
                s1 = s2;
                k1 = k2;
                if (s0 == 0.f) {
                    renderer->draw_line(
                        p0 + t0.cross(.75f),
                        p2 + t2.cross(.75f),
                        curvature_color(k0),
                        curvature_color(k2));
                    renderer->draw_line(
                        p0 - t0.cross(.75f),
                        p2 - t2.cross(.75f),
                        curvature_color(k0),
                        curvature_color(k2));
                    break;
                } else {
                    p0 = s.initial_position();
                    t0 = s.initial_tangent();
                    s0 = 0.f;
                    k0 = s.initial_curvature();
                }
            } else {
                p0 = p2;
                t0 = t2;
                s0 = s2;
                k0 = k2;
            }
        }
#if 0
        int n = int(ceil(.25f * s.length()));
        float f = s.length() / float(n);
        for (int ii = 0; ii < n + 1; ++ii) {
            float t = float(ii) * f;
            vec2 p = s.evaluate(t);
            vec2 d = s.evaluate_tangent(t).cross(-1);
            float k = s.evaluate_curvature(t);
            renderer->draw_line(p, p + d / k, color4(1,1,1,.1f), color4(1,1,1,.1f));
        }
#endif
        //float n = max(2.f, s.length() / diag);
        //vec2 p0 = s.initial_position();
        //for (float ii = 1.f; ii < n; ii += 1.f) {
        //    vec2 p1 = s.evaluate(ii / n * s.length());
        //    renderer->draw_line(p0, p1, color4(1,1,1,1), color4(1,1,1,1));
        //    p0 = p1;
        //}
        //renderer->draw_line(p0, s.final_position(), color4(1,1,1,1), color4(1,1,1,1));
    }
}

//------------------------------------------------------------------------------
void rail_network::add_segment(clothoid::segment s)
{
    if (s.type() == clothoid::segment_type::arc && abs(s.length() * s.initial_curvature()) > .5f * math::pi<float>) {
        clothoid::segment s1, s2;
        s.split(.5f * s.length(), s1, s2);
        add_segment(s1);
        add_segment(s2);
    } else {
    auto edge = _network.insert_edge(s);

    update_clearance(edge);
    update_clearance(edge ^ 1);
    }
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
    edge_index edge;
    float dist;
    if (get_closest_segment(position, 1.f, edge, dist)) {
        return _world->spawn<rail_station>(edge, dist, name);
    } else {
        return handle<rail_station>();
    }
}

//------------------------------------------------------------------------------
rail_network::node_index rail_network::insert_node(vec2 position)
{
    edge_index edge;
    float dist;

    // FIXME: epsilon
    if (!_network.get_closest_segment(position, 5.f, edge, dist)) {
        return invalid_node;
    } else if (dist < 5.f || dist > _network.edge_length(edge) - 5.f) {
        return invalid_node;
    }

    edge_index new_edge;
    node_index new_node;

    _network.split_edge(edge, dist, &new_edge, &new_node);

    // update clearance
    node_index node = _network.end_node(new_edge);
    for (auto e = _network.first_edge(node); e != invalid_edge; e = _network.next_edge(e)) {
        if (e == (edge ^ 1)) {
            continue;
        }

        auto it = _clearance.find(std::make_pair(edge ^ 1, e));
        if (it != _clearance.end()) {
            _clearance[std::make_pair(new_edge ^ 1, e)] = it->second;
            _clearance.erase(it);
        }

        it = _clearance.find(std::make_pair(e, edge ^ 1));
        if (it != _clearance.end()) {
            _clearance[std::make_pair(e, new_edge ^ 1)] = it->second;
            _clearance.erase(it);
        }
    }

    for (auto obj : _world->objects()) {
        if (obj->is_type<train>()) {
            obj->as_type<train>()->on_edge_split(edge, new_edge, new_node);
        }
    }

    return new_node;
}

//------------------------------------------------------------------------------
clothoid::segment rail_network::get_segment(edge_index edge) const
{
    return _network.get_segment(edge);
}

//------------------------------------------------------------------------------
rail_network::node_index rail_network::start_node(edge_index edge) const
{
    return _network.start_node(edge);
}

//------------------------------------------------------------------------------
bool rail_network::get_closest_segment(
    vec2 position,
    float max_distance,
    edge_index& edge,
    float& length) const
{
    return _network.get_closest_segment(
        position,
        max_distance,
        edge,
        length);
}

//------------------------------------------------------------------------------
bool rail_network::get_closest_node(
    vec2 position,
    float max_distance,
    node_index& node) const
{
    return _network.get_closest_node(
        position,
        max_distance,
        node);
}

//------------------------------------------------------------------------------
std::size_t rail_network::find_path(rail_position start, rail_position goal, edge_index* edges, std::size_t max_edges) const
{
    vec2 goal_pos;
    if (goal.is_edge) {
        goal_pos = _network.get_segment(goal.edge).evaluate(goal.dist);
    } else if (goal.is_node) {
        goal_pos = _network.node_position(goal.node);
    }

    struct search_state {
        float distance; //!< total distance along path, including this edge
        float heuristic; //!< estimated distance to destination
        std::size_t previous; //!< index of previous search node
        node_index node; //!< destination node
        edge_index edge; //!< traversed edge
    };

    std::vector<search_state> search;

    if (start.is_edge) {
        vec2 start_pos = _network.get_segment(start.edge).evaluate(start.dist);
        search.push_back({
            0.f,
            (start_pos - goal_pos).length(),
            SIZE_MAX,
            _network.end_node(start.edge),
            start.edge,
        });
    } else if (start.is_node) {
        // populate initial search state with all edges connected to start node
        for (edge_index edge = _network.first_edge(start.node)
            ; edge != invalid_edge
            ; edge = _network.next_edge(edge)) {

            node_index node = _network.end_node(edge);

            search.push_back({
                _network.edge_length(edge),
                (goal_pos - _network.node_position(node)).length(),
                SIZE_MAX,
                node,
                edge,
                });
        }
    }

    struct search_comparator {
        decltype (search) const& _search;
        bool operator()(std::size_t lhs, std::size_t rhs) const {
            float d1 = _search[lhs].distance + _search[lhs].heuristic;
            float d2 = _search[rhs].distance + _search[rhs].heuristic;
            return d1 < d2;
        }
    };

    std::priority_queue<std::size_t, std::vector<std::size_t>, search_comparator> queue{{search}};
    std::set<std::size_t> closedset;

    for (std::size_t ii = 0; ii < search.size(); ++ii) {
        closedset.insert(search[ii].edge);
        queue.push(ii);
    }

    // drain the queue until we reach the goal or run out of edges
    while (queue.size()) {
        std::size_t idx = queue.top(); queue.pop();

        // check if search reached the goal
        if (goal.is_edge && search[idx].edge == goal.edge
            || goal.is_node && search[idx].node == goal.node) {

            // determine the length of the path by backtracking through the search state
            std::size_t depth = 0;
            for (std::size_t ii = idx; search[ii].previous != SIZE_MAX; ii = search[ii].previous) {
                ++depth;
            }

            // if path buffer is too small then just return the path size
            if (depth > max_edges) {
                return depth;
            }

            // fill in the path buffer by backtracking through the search state
            edge_index* edge_ptr = edges + depth - 1;
            for (std::size_t ii = idx; search[ii].previous != SIZE_MAX; ii = search[ii].previous) {
                *edge_ptr-- = search[ii].edge;
            }
            return depth;
        }

        // add all suitable edges from the search node to the queue
        vec2 dir = _network.get_segment(search[idx].edge).final_tangent();
        for (edge_index edge = _network.first_edge(search[idx].node)
            ; edge != invalid_edge
            ; edge = _network.next_edge(edge)) {

            if (dot(dir, _network.edge_direction(edge)) < .999f) {
                continue;
            }

            if (closedset.find(edge) != closedset.end()) {
                continue;
            }

            node_index node = _network.end_node(edge);

            search.push_back({
                search[idx].distance + _network.edge_length(edge),
                (goal_pos - _network.node_position(node)).length(),
                idx,
                node,
                edge,
            });
            queue.push(search.size() - 1);
            closedset.insert(edge);
        }
    }

    // search failed
    return 0;
}

//------------------------------------------------------------------------------
bool rail_network::calculate_clearance(edge_index e0, edge_index e1, float clearance, float& c0, float& c1) const
{
    clothoid::segment s0 = _network.get_segment(e0);
    clothoid::segment s1 = _network.get_segment(e1);

    // initial guess
    c0 = clearance;

    for (int ii = 0; ii < 32; ++ii) {
        // point on segment 0 for current guess
        vec2 p0 = s0.evaluate(c0);
        vec2 t0 = s0.evaluate_tangent(c0);

        // closest point on segment 1 to point on segment 0
        vec3 p1 = s1.get_closest_point(p0);
        vec2 t1 = s1.evaluate_tangent(p1.z);

        c1 = p1.z;

        // refine guess until within epsilon or outside segment
        float d = length(p1.to_vec2() - p0);
        float ds = (clearance - d) / dot(t0, t1);
        if (abs(clearance - d) < 1e-6f) {
            break;
        } else if (c0 < 0.f && ds < 0.f) {
            return false;
        } else if (c0 > s0.length() && ds > 0.f) {
            return false;
        }

        c0 += ds;
    }

    return true;
}

//------------------------------------------------------------------------------
void rail_network::update_clearance(edge_index edge)
{
    clothoid::segment s = _network.get_segment(edge);
    node_index node = _network.start_node(edge);

    // iterate over all edges at the start node
    for (auto e = _network.first_edge(node); e != invalid_edge; e = _network.next_edge(e)) {
        if (e == edge) {
            continue;
        }
        // ignore edges that are not the same direction
        if (dot(s.initial_tangent(), _network.get_segment(e).initial_tangent()) < -.999f) {
            continue;
        } else if (dot(s.initial_tangent(), _network.get_segment(e).initial_tangent()) < 0) {
            _clearance[std::make_pair(edge, e)] = junction_clearance;
            _clearance[std::make_pair(e, edge)] = junction_clearance;
            continue;
        }
        // calculate and update clearance for both edges
        float c0, c1;
        if (calculate_clearance(edge, e, junction_clearance, c0, c1)) {
            //float c2, c3;
            //if (calculate_clearance(e, edge, junction_clearance, c3, c2)) {
            //    c0 = max(c0, c2);
            //    c1 = max(c1, c3);
            //}
            //if (c0 < junction_clearance || c1 < junction_clearance) {
            //    int breakme = 1; (void)breakme;
            //}
            _clearance[std::make_pair(edge, e)] = max(junction_clearance, c0);
            _clearance[std::make_pair(e, edge)] = max(junction_clearance, c1);
        }
    }
}

} // namespace game

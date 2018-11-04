// g_ship_layout.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship_layout.h"
#include <set>

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
ship_layout::ship_layout(vec2 const* vertices, int num_vertices,
                         compartment const* compartments, int num_compartments,
                         connection const* connections, int num_connections)
    : _vertices(vertices, vertices + num_vertices)
    , _compartments(compartments, compartments + num_compartments)
    , _connections(connections, connections + num_connections)
{
    for (auto& c : _compartments) {
        vec2 v0 = _vertices[c.first_vertex];
        c.area = 0.f;
        for (int ii = 2; ii < c.num_vertices; ++ii) {
            vec2 v1 = _vertices[c.first_vertex + ii - 1];
            vec2 v2 = _vertices[c.first_vertex + ii - 0];
            c.area += 0.5f * (v2 - v1).cross(v1 - v0);
        }
    }

    for (auto& c : _connections) {
        vec2 v0 = _vertices[c.vertices[1]] - _vertices[c.vertices[0]];
        vec2 v1 = _vertices[c.vertices[3]] - _vertices[c.vertices[2]];
        c.width = .5f * (v0.length() + v1.length());
    }
}

//------------------------------------------------------------------------------
uint16_t ship_layout::intersect_compartment(vec2 point) const
{
    // iterate over all compartments and return the first compartment that
    // contains the given point (if any)
    for (std::size_t ii = 0, jj, sz = _compartments.size(); ii < sz; ++ii) {
        uint16_t base = _compartments[ii].first_vertex;
        uint16_t size = _compartments[ii].num_vertices;
        for (jj = 0; jj < size; ++jj) {
            vec2 va = point - _vertices[base + jj];
            vec2 vb = point - _vertices[base + (jj + 1) % size];
            if (va.cross(vb) > 0.f) {
                break;
            }
        }
        if (jj == size) {
            return narrow_cast<uint16_t>(ii);
        }
    }
    // point is not contained by any compartment
    return invalid_compartment;
}

//------------------------------------------------------------------------------
int ship_layout::find_path(vec2 start, vec2 end, float radius, vec2* buffer, int buffer_size) const
{
    uint16_t start_idx = intersect_compartment(start);
    uint16_t end_idx = intersect_compartment(end);

    // if either point is invalid then return no path
    if (start_idx == invalid_compartment || end_idx == invalid_compartment) {
        return 0;
    // if start and end are in the same compartment then return a direct path
    } else if (start_idx == end_idx) {
        if (buffer_size >= 2) {
            buffer[0] = start;
            buffer[1] = end;
        }
        return 2;
    }

    struct search_state {
        vec2 position;
        float distance; //!< distance along path to this search node
        float heuristic; //!< estimated distance from this node to goal
        int previous; //!< index of previous search node
        uint16_t compartment; //!< index of the destination compartment
        uint16_t connection; //!< index of the traversed connection
    };

    std::array<search_state, 256> search;
    search[0] = {start, 0.f, (start - end).length(), -1, start_idx, invalid_connection};
    int search_size = 0;

    struct search_comparator {
        decltype(search) const& _search;
        bool operator()(int lhs, int rhs) const {
            float d1 = _search[lhs].distance + _search[lhs].heuristic;
            float d2 = _search[rhs].distance + _search[rhs].heuristic;
            return d1 > d2;
        };
    };

    std::priority_queue<int, std::vector<int>, search_comparator> queue{{search}};
    queue.push(search_size++);

    std::set<int> openset;
    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        openset.insert(narrow_cast<uint16_t>(ii));
    }

    while (queue.size() && search_size < search.size()) {
        int idx = queue.top(); queue.pop();

        // remove connection from the open set
        if (search[idx].connection != invalid_connection) {
            auto it = openset.find(search[idx].connection);
            if (it != openset.end()) {
                openset.erase(it);
            } else {
                continue;
            }
        }

        if (search[idx].compartment == end_idx) {
            // search succeeded
            int depth = 0;
            for (int ii = idx; search[ii].previous != -1; ii = search[ii].previous) {
                ++depth;
            }

            // if buffer doesn't have room just return the path length
            int num_vertices = depth * 2 + 2;
            if (num_vertices > buffer_size) {
                return num_vertices;
            }

            // copy path into buffer and return path length
            vec2* buffer_ptr = buffer + num_vertices - 1;
            *buffer_ptr-- = end;
            // insert two vertices for each connection, one on each side
            for (int ii = idx; search[ii].previous != -1; ii = search[ii].previous) {
                auto const& c = _connections[search[ii].connection];
                vec2 n = _vertices[c.vertices[0]] - _vertices[c.vertices[2]]
                        + _vertices[c.vertices[1]] - _vertices[c.vertices[3]];
                float l = .25f * n.normalize_length();
                // check direction of connection against direction of path
                if (n.dot(search[ii].position - buffer_ptr[1]) < .0f) {
                    n *= -1.f;
                }
                *buffer_ptr-- = search[ii].position - n * (l + radius);
                *buffer_ptr-- = search[ii].position + n * (l + radius);
            }
            *buffer_ptr-- = start;
            return num_vertices;
        }

        // iterate over all untraversed edges
        for (auto ii : openset) {
            if (search_size == search.size()) {
                break;
            }
            auto& s = search[search_size];

            // ensure connection is wide enough to be traversed
            if (_connections[ii].width < 2.f * radius) {
                continue;
            }

            if (_connections[ii].compartments[0] == search[idx].compartment) {
                s.compartment = _connections[ii].compartments[1];
            } else if (_connections[ii].compartments[1] == search[idx].compartment) {
                s.compartment = _connections[ii].compartments[0];
            } else {
                continue;
            }

            // calculate midpoint of the connection
            auto const& c = _connections[ii];
            s.position =  _vertices[c.vertices[0]];
            for (int jj = 1; jj < countof(c.vertices); ++jj) {
                s.position += _vertices[c.vertices[jj]];
            }
            s.position /= static_cast<float>(countof(c.vertices));
            // distance from previous path position
            float d = (s.position - search[idx].position).length();
            s.distance = search[idx].distance + d;
            s.heuristic = (s.position - end).length();
            s.previous = idx;
            s.connection = narrow_cast<uint16_t>(ii);
            queue.push(search_size++);
        }
    }

    // search failed
    return 0;
}

//------------------------------------------------------------------------------
ship_state::ship_state(ship_layout const& layout)
    : _layout(layout)
    , framenum(0)
{
    constexpr compartment_state default_compartment_state = {1.f, 0.f, {0.f, 0.f}};
    _compartments.resize(_layout.compartments().size(), default_compartment_state);

    constexpr connection_state default_connection_state = {false, 0.f, 0.f, 0.f};
    _connections.resize(_layout.connections().size(), default_connection_state);
}

//------------------------------------------------------------------------------
void ship_state::think()
{
#if 0
    struct simulation {
        float density;
        float static_pressure;
        float dynamic_pressure;
        float temperature;
        float velocity;
    };

    std::vector<simulation> s(_compartments.size());

    constexpr float R = 8.314459848; // J/mol/K
    constexpr float M = 28.97; // g/mol (dry air)
    constexpr float rho0 = 1.225e3f; // g/m^3 (dry air at STP)

    //
    //  initialization
    //

    for (uint16_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
        s[ii].density = _compartments[ii].atmosphere * rho0;
        s[ii].velocity = 0.f;
        s[ii].temperature = 295.f; // approx. room temperature in kelvin
        s[ii].static_pressure = s[ii].density * s[ii].temperature * (R / M);
        s[ii].dynamic_pressure = .5f * s[ii].density * square(s[ii].velocity);
    }

    //
    //  simulation
    //

    for (uint16_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        uint16_t c0 = _layout.connections()[ii].compartments[0];
        uint16_t c1 = _layout.connections()[ii].compartments[1];

        // calculate pressure gradient at each connection
        float gradient = 0.f;
        if (c0 != ship_layout::invalid_compartment) {
            gradient -= s[c0].static_pressure + s[c0].dynamic_pressure;
        }
        if (c1 != ship_layout::invalid_compartment) {
            gradient += s[c1].static_pressure + s[c1].dynamic_pressure;
        }

        // calculate acceleration at each connection
        // P = F / A

        // a = F / m
        //   = PA / m
        //   = PA / rho / V
        //   = P / rho / z

    }

    for (uint16_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
        s[ii].static_pressure = s[ii].density * s[ii].temperature * (R / M);
        s[ii].dynamic_pressure = .5f * s[ii].density * square(s[ii].velocity);
    }
#endif

    for (std::size_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
        _compartments[ii].flow[0] = 0.f;
        _compartments[ii].flow[1] = 0.f;

        // calculate atmosphere loss through hull damage separately
        float delta = -_compartments[ii].damage * FRAMETIME.to_seconds();
        if (delta > _compartments[ii].atmosphere) {
            _compartments[ii].atmosphere = 0.f;
        } else {
            _compartments[ii].atmosphere -= delta;
        }
    }

    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        uint16_t c0 = _layout.connections()[ii].compartments[0];
        uint16_t c1 = _layout.connections()[ii].compartments[1];

        // calculate pressure differential at each connection
        if (c0 == ship_layout::invalid_compartment) {
            assert(c1 != ship_layout::invalid_compartment);
            _connections[ii].gradient = _compartments[c1].atmosphere;
        } else if (c1 == ship_layout::invalid_compartment) {
            assert(c0 != ship_layout::invalid_compartment);
            _connections[ii].gradient = -_compartments[c0].atmosphere;
        } else {
            _connections[ii].gradient = _compartments[c1].atmosphere
                                      - _compartments[c0].atmosphere;
        }
    }

    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        uint16_t c0 = _layout.connections()[ii].compartments[0];
        uint16_t c1 = _layout.connections()[ii].compartments[1];

        float mass = 0.f;
        if (c0 != ship_layout::invalid_compartment) {
            mass += _compartments[c0].atmosphere * _layout.compartments()[c0].area;
        }
        if (c1 != ship_layout::invalid_compartment) {
            mass += _compartments[c1].atmosphere * _layout.compartments()[c1].area;
        }

        _connections[ii].velocity *= .95f;

        if (_connections[ii].opened || _connections[ii].opened_automatic) {
            _connections[ii].velocity += _connections[ii].gradient * mass * FRAMETIME.to_seconds();
        } else {
            _connections[ii].velocity = 0.f;
        }
    }

    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        if (_connections[ii].opened || _connections[ii].opened_automatic) {
            _connections[ii].flow = _connections[ii].velocity * _layout.connections()[ii].width * FRAMETIME.to_seconds();
        } else {
            _connections[ii].flow = 0.f;
        }
    }

    std::vector<std::vector<float>> flow;
    flow.resize(5);
    flow[0].resize(_connections.size());
    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        flow[0][ii] = _connections[ii].flow;
    }

    for (int iter = 0; iter < 32; ++iter) {
        bool clamped = false;

        for (std::size_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
            _compartments[ii].flow[0] = 0.f;
            _compartments[ii].flow[1] = 0.f;
        }

        for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
            uint16_t c0 = _layout.connections()[ii].compartments[0];
            uint16_t c1 = _layout.connections()[ii].compartments[1];
            // calculate flow in each compartment
            if (_connections[ii].opened || _connections[ii].opened_automatic) {
                float delta = _connections[ii].flow;
                if (c0 != ship_layout::invalid_compartment) {
                    _compartments[c0].flow[0] += min(0.f, delta); // outflow
                    _compartments[c0].flow[1] += max(0.f, delta); // inflow
                }
                if (c1 != ship_layout::invalid_compartment) {
                    _compartments[c1].flow[0] -= max(0.f, delta); // outflow
                    _compartments[c1].flow[1] -= min(0.f, delta); // inflow
                }
            }
        }

        // advect atmosphere between compartments
        for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
            uint16_t c0 = _layout.connections()[ii].compartments[0];
            uint16_t c1 = _layout.connections()[ii].compartments[1];

            if (_connections[ii].opened || _connections[ii].opened_automatic) {
                if ( _connections[ii].flow < 0.f && c0 != ship_layout::invalid_compartment) {
                    float fraction = -_connections[ii].flow / _compartments[c0].flow[0];
                    float limit = fraction * (_compartments[c0].flow[1] + _compartments[c0].atmosphere * _layout.compartments()[c0].area);
                    assert(limit <= 0.f);
                    if ( _connections[ii].flow < limit) {
                         _connections[ii].flow = limit;
                         clamped = true;
                    }
                }
                if ( _connections[ii].flow > 0.f && c1 != ship_layout::invalid_compartment) {
                    float fraction = -_connections[ii].flow / _compartments[c1].flow[0];
                    float limit = fraction * (_compartments[c1].flow[1] + _compartments[c1].atmosphere * _layout.compartments()[c1].area);
                    assert(limit >= 0.f);
                    if ( _connections[ii].flow > limit) {
                         _connections[ii].flow = limit;
                         clamped = true;
                    }
                }
                assert(!isnan(_connections[ii].flow));
            }
        }

        flow[iter + 1].resize(_connections.size());
        for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
            flow[iter + 1][ii] = _connections[ii].flow;
        }

        if (!clamped) {
            break;
        }
    }

    // advect atmosphere between compartments
    for (std::size_t ii = 0, sz = _connections.size(); ii < sz; ++ii) {
        uint16_t c0 = _layout.connections()[ii].compartments[0];
        uint16_t c1 = _layout.connections()[ii].compartments[1];

        if (_connections[ii].opened || _connections[ii].opened_automatic) {
            if (c0 != ship_layout::invalid_compartment) {
                float delta = _connections[ii].flow / _layout.compartments()[c0].area;
                _compartments[c0].atmosphere += delta;
                assert(_compartments[c0].atmosphere >= -1e-3f);
            }
            if (c1 != ship_layout::invalid_compartment) {
                float delta = _connections[ii].flow / _layout.compartments()[c1].area;
                _compartments[c1].atmosphere -= delta;
                assert(_compartments[c1].atmosphere >= -1e-3f);
            }
        }
    }

    int index = framenum++ % countof<decltype(compartment_state::history)>();
    for (std::size_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
        _compartments[ii].history[index] = _compartments[ii].atmosphere;
    }
}

//------------------------------------------------------------------------------
void ship_state::recharge(float atmosphere_per_second)
{
    float delta = atmosphere_per_second * FRAMETIME.to_seconds();
    for (std::size_t ii = 0, sz = _compartments.size(); ii < sz; ++ii) {
        _compartments[ii].atmosphere = clamp(_compartments[ii].atmosphere + delta, 0.f, 1.f);
    }
}

} // namespace game

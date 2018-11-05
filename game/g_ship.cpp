// g_ship.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_ship.h"
#include "g_character.h"
#include "g_shield.h"
#include "g_weapon.h"
#include "g_subsystem.h"
#include "r_model.h"

#include <intrin.h>
#include <random>

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type ship::_type(object::_type);
physics::material ship::_material(0.5f, 1.0f, 5.0f);

vec2 main_body_vertices[] = {
    {11.0000000, 6.00000000 },
    {10.0000000, 7.00000000 },
    {8.00000000, 8.00000000 },
    {5.00000000, 9.00000000 },
    {-1.00000000, 9.00000000 },
    {-7.00000000, 5.00000000 },
    {-7.00000000, -5.00000000 },
    {-1.00000000, -9.00000000 },
    {-1.00000000, -9.00000000 },
    {5.00000000, -9.00000000 },
    {8.00000000, -8.00000000 },
    {10.0000000, -7.00000000 },
    {11.0000000, -6.00000000 },
    {11.0000000, 6.00000000 },
};

vec2 left_engine_vertices[] = {
    vec2(8, -8) + vec2{-1.00000000, 9.0000000 },
    vec2(8, -8) + vec2{-1.00000000, 10.0000000 },
    vec2(8, -8) + vec2{-16.0000000, 10.0000000 },
    vec2(8, -8) + vec2{-17.0000000, 9.00000000 },
    vec2(8, -8) + vec2{-17.0000000, 7.00000000 },
    vec2(8, -8) + vec2{-16.0000000, 6.00000000 },
    vec2(8, -8) + vec2{-9.0000000, 5.00000000 },
    vec2(8, -8) + vec2{-7.0000000, 5.00000000 },
};

vec2 right_engine_vertices[] = {
    vec2(8, 8) + vec2{-1.00000000, -9.0000000 },
    vec2(8, 8) + vec2{-1.00000000, -10.0000000 },
    vec2(8, 8) + vec2{-16.0000000, -10.0000000 },
    vec2(8, 8) + vec2{-17.0000000, -9.00000000 },
    vec2(8, 8) + vec2{-17.0000000, -7.00000000 },
    vec2(8, 8) + vec2{-16.0000000, -6.00000000 },
    vec2(8, 8) + vec2{-9.0000000, -5.00000000 },
    vec2(8, 8) + vec2{-7.0000000, -5.00000000 },
};

const ship_layout default_layout(
#if 1
    {
        {7.f, 7.f}, {10.f, 5.f}, {10.f, -5.f}, {7.f, -7.f}, {7.f, -1.f}, {7.f, 1.f},
        {6.f, 1.f}, {6.f, -1.f}, {6.f, -2.f}, {4.f, -2.f}, {2.f, -2.f}, {0.f, -2.f}, {0.f, -1.f}, {0.f, 1.f}, {0.f, 2.f}, {2.f, 2.f}, {4.f, 2.f}, {6.f, 2.f},
        {0.f, -3.f}, {2.f, -3.f}, {4.f, -3.f}, {6.f, -3.f}, {6.f, -7.f}, {0.f, -8.f},
        {6.f, 3.f}, {4.f, 3.f}, {2.f, 3.f}, {0.f, 3.f}, {0.f, 8.f}, {6.f, 7.f},
        {-1.f, 1.f}, {-1.f, -1.f}, {-1.f, -3.f}, {-6.f, -3.f}, {-6.f, -1.f}, {-6.f, 1.f}, {-6.f, 3.f}, {-1.f, 3.f},
        {-7.f, 1.f}, {-7.f, -1.f},
        {7.f, -4.f}, {7.f, -6.f}, {6.f, -6.f}, {6.f, -4.f},
        {7.f, 6.f}, {7.f, 4.f}, {6.f, 4.f}, {6.f, 6.f},
        {-1.f, 4.f}, {-6.f, 4.f}, {-7.f, 7.f}, {-7.f, 9.f}, {-3.f, 9.f}, {-1.f, 8.f},
        {-1.f, -4.f}, {-1.f, -8.f}, {-3.f, -9.f}, {-7.f, -9.f}, {-7.f, -7.f}, {-6.f, -4.f},
        {-2.5f, 4.f}, {-4.5f, 4.f}, {-4.5f, 3.f}, {-2.5f, 3.f},
        {-4.5f, -4.f}, {-2.5f, -4.f}, {-2.5f, -3.f}, {-4.5f, -3.f},
        {-8.f, 7.f}, {-14.f, 7.f}, {-14.f, 9.f}, {-8.f, 9.f},
        {-14.f, -7.f}, {-8.f, -7.f}, {-8.f, -9.f}, {-14.f, -9.f},
    },
    {
        {0, 6},
        {6, 12},
        {18, 6},
        {24, 6},
        {30, 8},
        {48, 6},
        {54, 6},
        {68, 4},
        {72, 4},
    },
    {
        {{0, 1}, {4, 5, 6, 7}},
        {{1, 2}, {9, 10, 19, 20}},
        {{1, 3}, {15, 16, 25, 26}},
        {{1, 4}, {12, 13, 30, 31}},
        {{4, ship_layout::invalid_compartment}, {34, 35, 38, 39}},
        {{0, 2}, {40, 41, 42, 43}},
        {{0, 3}, {44, 45, 46, 47}},
        {{4, 5}, {60, 61, 62, 63}},
        {{4, 6}, {64, 65, 66, 67}},
        {{5, 7}, {50, 51, 71, 68}},
        {{6, 8}, {57, 58, 73, 74}},
    }
#else
    {
        {7.1f, 3.9f}, {9.5f, 3.9f}, {10.f, 3.f}, {10.5f, 1.f}, {10.5f, -1.f}, {10.f, -3.f}, {9.5f, -3.9f}, {7.1f, -3.9f},
        {9.f, 4.1f}, {5.f, 4.1f}, {3.f, 6.f}, {3.f, 7.f}, {6.5f, 7.f}, {8.f, 6.f},
        {5.1f, 3.9f}, {6.9f, 3.9f}, {6.9f, -3.9f}, {5.1f, -3.9f},
        {-.9f, .9f}, {4.9f, .9f}, {4.9f, -.9f}, {-.9f, -.9f},
        {-2.9f, 9.9f}, {-1.1f, 9.9f}, {-1.1f, -9.9f}, {-2.9f, -9.9f},
        {-6.9f, 2.9f}, {-3.1f, 2.9f}, {-3.1f, -2.9f}, {-6.9f, -2.9f},
            // connections
        {6.9f, .7f}, {7.1f, .7f}, {7.1f, -.7f}, {6.9f, -.7f},
        {4.9f, .7f}, {5.1f, .7f}, {5.1f, -.7f}, {4.9f, -.7f},
        {-1.1f, .7f}, {-.9f, .7f}, {-.9f, -.7f}, {-1.1f, -.7f},
        {-3.1f, .7f}, {-2.9f, .7f}, {-2.9f, -.7f}, {-3.1f, -.7f},
    },
    {
        {0, 8},
        {8, 6},
        {14, 4},
        {18, 4},
        {22, 4},
        {26, 4},
    },
    {
        {{0, 2}, {30, 31, 32, 33}},
        {{2, 3}, {34, 35, 36, 37}},
        {{3, 4}, {38, 39, 40, 41}},
        {{4, 5}, {42, 43, 44, 45}},
    }
#endif
);

//------------------------------------------------------------------------------
ship::ship()
    : _usercmd{}
    , _state(default_layout)
    , _shield(nullptr)
    , _dead_time(time_value::max)
    , _is_destroyed(false)
    , _shape({
        {std::make_unique<physics::convex_shape>(main_body_vertices)},
        {std::make_unique<physics::convex_shape>(left_engine_vertices), vec2(-8, 8)},
        {std::make_unique<physics::convex_shape>(right_engine_vertices), vec2(-8, -8)}})
{
    _rigid_body = physics::rigid_body(&_shape, &_material, 10.f);

    _model = &ship_model;
}

//------------------------------------------------------------------------------
ship::~ship()
{
    get_world()->remove_body(&_rigid_body);
}

//------------------------------------------------------------------------------
void ship::spawn()
{
    object::spawn();

    get_world()->add_body(this, &_rigid_body);

    for (int ii = 0; ii < 3; ++ii) {
        _crew.push_back(get_world()->spawn<character>());
    }

    std::vector<int> compartments(_state.compartments().size());
    for (std::size_t ii = 0, sz = compartments.size(); ii < sz; ++ii) {
        compartments[ii] = narrow_cast<int>(ii);
    }

    std::shuffle(compartments.begin(), compartments.end(), std::random_device());

    _reactor = get_world()->spawn<subsystem>(this, compartments[0], subsystem_info{subsystem_type::reactor, 10});
    _subsystems.push_back(_reactor);

    _shield = get_world()->spawn<game::shield>(&_shape, this, compartments[1]);
    _subsystems.push_back(_shield);

    _engines = get_world()->spawn<game::engines>(this, compartments[2], engines_info{16.f, .125f, 8.f, .0625f, .5f, .5f});
    _subsystems.push_back(_engines);

    for (int ii = 0; ii < 2; ++ii) {
        weapon_info info = weapon::by_random(_random);
        _weapons.push_back(get_world()->spawn<weapon>(this, compartments[3 + ii], info, vec2(11.f, ii ? 6.f : -6.f)));
        _subsystems.push_back(_weapons.back());
    }

    _subsystems.push_back(get_world()->spawn<subsystem>(this, compartments[5], subsystem_info{subsystem_type::life_support, 1}));
    _subsystems.push_back(get_world()->spawn<subsystem>(this, compartments[6], subsystem_info{subsystem_type::medical_bay, 1}));

    std::vector<handle<subsystem>> assignments(_subsystems.begin(), _subsystems.end());
    for (auto& ch : _crew) {
        if (assignments.size()) {
            std::size_t index = _random.uniform_int(assignments.size());
            ch->operate(assignments[index]);
            assignments.erase(assignments.begin() + index);
        }
    }
}

extern vec2 g_cursor;

//------------------------------------------------------------------------------
void ship::draw(render::system* renderer, time_value time) const
{
    if (!_is_destroyed) {
        renderer->draw_model(_model, get_transform(time), _color);

        constexpr color4 subsystem_colors[2][2] = {
            { color4(.4f, 1.f, .2f, .225f), color4(1.f, .2f, 0.f, .333f) },
            { color4(.4f, 1.f, .2f, 1.00f), color4(1.f, .2f, 0.f, 1.00f) },
        };

        float alpha = 1.f;
        if (time > _dead_time && time - _dead_time < destruction_time) {
            alpha = 1.f - (time - _dead_time) / destruction_time;
        }

        //
        // draw reactor ui
        //

        if (_reactor) {
            constexpr float mint = (3.f / 4.f) * math::pi<float>;
            constexpr float maxt = (5.f / 4.f) * math::pi<float>;
            constexpr float radius = 40.f;
            constexpr float edge_width = 1.f;
            constexpr float edge = edge_width / radius;

            vec2 pos = get_position(time);

            for (int ii = 0; ii < _reactor->maximum_power(); ++ii) {
                bool damaged = ii >= _reactor->maximum_power() - std::ceil(_reactor->damage() - .2f);
                bool powered = ii < _reactor->current_power();

                color4 c = subsystem_colors[powered][damaged]; c.a *= alpha;

                float t0 = maxt - (maxt - mint) * (float(ii + 1) / float(_reactor->maximum_power()));
                float t1 = maxt - (maxt - mint) * (float(ii + 0) / float(_reactor->maximum_power()));
                renderer->draw_arc(pos, radius, 3.f, t0 + .5f * edge, t1 - .5f * edge, c);
            }
        }

        //
        // draw shield ui
        //

        if (_shield) {
            constexpr float mint = (-1.f / 4.f) * math::pi<float>;
            constexpr float maxt = (1.f / 4.f) * math::pi<float>;
            constexpr float radius = 40.f;

            float midt =  mint + (maxt - mint) * (_shield->strength() / _shield->info().maximum_power);

            vec2 pos = get_position(time);

            color4 c0 = _shield->colors()[1]; c0.a *= 1.5f * alpha;
            color4 c1 = _shield->colors()[1]; c1.a = alpha;
            renderer->draw_arc(pos, radius, 3.f, mint, maxt, c0);
            renderer->draw_arc(pos, radius, 3.f, mint, midt, c1);
        }

        //
        // draw subsystems ui
        //

        vec2 position = get_position(time) - vec2(8.f * (_subsystems.size() - 2.f) * .5f, 40.f);

        for (auto const& subsystem : _subsystems) {
            // reactor subsystem ui is drawn explicitly
            if (subsystem->info().type == subsystem_type::reactor) {
                continue;
            }

            // draw power bar for each subsystem power level
            for (int ii = 0; ii < subsystem->maximum_power(); ++ii) {
                bool damaged = ii >= subsystem->maximum_power() - std::ceil(subsystem->damage() - .2f);
                bool powered = ii < subsystem->current_power();

                color4 c = subsystem_colors[powered][damaged]; c.a *= alpha;

                renderer->draw_box(vec2(7,3), position + vec2(0, 10.f + 4.f * ii), c);
            }

            position += vec2(8,0);
        }

        position = get_position(time) - vec2(4.f * (_crew.size() - 1), -24.f);

        for (auto const& ch : _crew) {
            color4 c = ch->health() ? color4(1,1,1,alpha) : color4(1,.2f,0,alpha);
            if (ch->health() == 0.f) {
                renderer->draw_box(vec2(7,3), position + vec2(0,10 - 4), color4(1,.2f,0,alpha));
            } else if (ch->health() == 1.f) {
                renderer->draw_box(vec2(7,3), position + vec2(0,10 - 4), color4(1,1,1,alpha));
            } else {
                float t = ch->health();
                renderer->draw_box(vec2(7*(1.f-t),3), position + vec2(3.5f*t,10 - 4), color4(1,.2f,0,alpha));
                renderer->draw_box(vec2(7*t,3), position + vec2(-3.5f*(1.f-t),10 - 4), color4(1,1,1,alpha));
            }
            position += vec2(8,0);
        }

        //
        // draw layout
        //

        ship_layout const& _layout = _state.layout();
        mat3 transform = get_transform(time);
        std::vector<vec2> positions;
        std::vector<color4> colors;
        std::vector<int> indices;

        for (auto const& v : _layout.vertices()) {
            positions.push_back(v * transform);
            colors.push_back(color4(1,1,1,1));
        }

        for (std::size_t ii = 0, sz = _layout.compartments().size(); ii < sz; ++ii) {
            auto const& c = _layout.compartments()[ii];
            auto const& s = _state.compartments()[ii];
            color4 color(1, s.atmosphere, s.atmosphere, 1);
            colors[c.first_vertex + 0] = color;
            colors[c.first_vertex + 1] = color;
            for (int jj = 2; jj < c.num_vertices; ++jj) {
                colors[c.first_vertex + jj] = color;
                indices.push_back(c.first_vertex);
                indices.push_back(c.first_vertex + jj - 1);
                indices.push_back(c.first_vertex + jj - 0);
            }
        }

        renderer->draw_triangles(positions.data(), colors.data(), indices.data(), indices.size());

        indices.clear();
        for (std::size_t ii = 0, sz = _layout.connections().size(); ii < sz; ++ii) {
            auto const& c = _layout.connections()[ii];
            auto const& s = _state.connections()[ii];
            if (s.opened) {
                colors[c.vertices[0]] = color4(1,1,0,1);
                colors[c.vertices[1]] = color4(1,1,0,1);
                colors[c.vertices[2]] = color4(1,1,0,1);
                colors[c.vertices[3]] = color4(1,1,0,1);
            } else if (s.opened_automatic) {
                colors[c.vertices[0]] = color4(0,1,0,1);
                colors[c.vertices[1]] = color4(0,1,0,1);
                colors[c.vertices[2]] = color4(0,1,0,1);
                colors[c.vertices[3]] = color4(0,1,0,1);
            } else {
                colors[c.vertices[0]] = color4(0,1,1,1);
                colors[c.vertices[1]] = color4(0,1,1,1);
                colors[c.vertices[2]] = color4(0,1,1,1);
                colors[c.vertices[3]] = color4(0,1,1,1);
            }
            indices.push_back(c.vertices[0]);
            indices.push_back(c.vertices[1]);
            indices.push_back(c.vertices[2]);
            indices.push_back(c.vertices[0]);
            indices.push_back(c.vertices[2]);
            indices.push_back(c.vertices[3]);
        }

        renderer->draw_triangles(positions.data(), colors.data(), indices.data(), indices.size());

#if 0
        for (std::size_t ii = 0, sz = _layout.compartments().size(); ii < sz; ++ii) {
            auto const& c = _layout.compartments()[ii];
            auto const& s = _state.compartments()[ii];
            vec2 p = vec2_zero;
            for (int jj = 0; jj < c.num_vertices; ++jj) {
                p += positions[c.first_vertex + jj];
            }
            p /= c.num_vertices;
            renderer->draw_string(va("%.2f - %.2f", s.atmosphere, s.atmosphere * c.area), p, color4(.4f,.4f,.4f,1));
        }

        for (std::size_t ii = 0, sz = _layout.connections().size(); ii < sz; ++ii) {
            auto const& c = _layout.connections()[ii];
            auto const& s = _state.connections()[ii];
            vec2 p = vec2_zero;
            for (int jj = 0; jj < 4; ++jj) {
                p += positions[c.vertices[jj]] * .25f;
            }
            renderer->draw_string(va("%.2f - %.2f", s.flow, s.velocity), p, color4(.4f,.3f,.2f,1));
        }
#endif

        //
        //  draw subsystem icons
        //

        {
            mat3 tx = get_transform(time);

            for (auto const& ss : _subsystems) {
                auto const& c = layout().compartments()[ss->compartment()];
                vec2 pt = vec2_zero;
                for (int ii = 0; ii < c.num_vertices; ++ii) {
                    pt += layout().vertices()[c.first_vertex + ii];
                }
                pt /= c.num_vertices;
                string::literal text = "";
                if (ss->info().type == subsystem_type::reactor) {
                    text = "R";
                } else if (ss->info().type == subsystem_type::engines) {
                    text = "E";
                } else if (ss->info().type == subsystem_type::shields) {
                    text = "S";
                } else if (ss->info().type == subsystem_type::weapons) {
                    text = "W";
                } else if (ss->info().type == subsystem_type::life_support) {
                    text = "L";
                } else if (ss->info().type == subsystem_type::medical_bay) {
                    text = "M";
                }

                vec2 sz = renderer->string_size(text);
                float t = sz.length_sqr() / (_model->bounds().maxs() - _model->bounds().mins()).length_sqr();
                float a = alpha * clamp(1.f - 16.f * t, 0.f, 1.f);
                color4 color = ss->damage() < ss->maximum_power() ? color4(.09f,.225f,.045f,a) : color4(.333f,.0666f,0,a);
                renderer->draw_string(text, pt * tx - sz * .5f, color);
            }
        }

        //
        //  draw crew
        //

        {
            struct text_layout {
                string::view text;
                color4 color;
                vec2 origin;
                vec2 offset;
                vec2 size;
            };

            std::vector<text_layout> layout;

            float t = renderer->view().size.length_sqr() / (_model->bounds().maxs() - _model->bounds().mins()).length_sqr();
            float a = alpha * clamp(1.f - t * (1.f / 16.f), 0.f, 1.f);

            mat3 tx = get_transform(time);
            for (auto const& ch : _crew) {
                renderer->draw_arc(ch->get_position(time) * tx, .25f, .15f, -math::pi<float>, math::pi<float>, color4(.4f,.5f,.6f,1.f));
                vec2 sz = renderer->string_size(ch->name());
                layout.push_back({
                    ch->name(),
                    ch->health() ? color4(0,0,1,a) : color4(1,0,0,a),
                    ch->get_position(time) * tx,
                    vec2(0, -.4f),
                    sz
                });
            }
#if 1
            for (int iter = 0; iter < 64; ++iter) {
                uint64_t overlap_mask = 0;
                bool has_overlap = false;
                float minscale = FLT_MAX;
                for (std::size_t ii = 0, sz = layout.size(); ii < sz; ++ii) {
                    bounds b0(layout[ii].origin + layout[ii].offset - layout[ii].size * .5f, layout[ii].origin + layout[ii].offset + layout[ii].size * .5f);
                    for (std::size_t jj = ii + 1; jj < sz; ++jj) {
                        bounds b1(layout[jj].origin + layout[jj].offset - layout[jj].size * .5f, layout[jj].origin + layout[jj].offset + layout[jj].size * .5f);
                        bounds overlap = b0 & b1;
                        if (!overlap.empty()) {
                            has_overlap = true;
                            overlap_mask |= (1LL << ii);
                            overlap_mask |= (1LL << jj);
                            vec2 s0 = overlap.size();
                            vec2 s1 = vec2(std::abs(layout[ii].origin.x + layout[ii].offset.x - layout[jj].origin.x - layout[jj].offset.x),
                                           std::abs(layout[ii].origin.y + layout[ii].offset.y - layout[jj].origin.y - layout[jj].offset.y));
                            float s = std::min((s0.x + s1.x) / s1.x, (s0.y + s1.y) / s1.y);
                            minscale = std::min(s * (1.f + 1e-3f), minscale);
                        }
                    }
                }
#if 0
                for (int ii = 0, sz = layout.size(); ii < sz; ++ii) {
                    text_layout const& ll = layout[ii];
                    color4 color = (overlap_mask & (1LL << ii)) ? color4(1,0,0,.1f) : color4(0,0,1,.1f);

                    vec2 v[4] = {
                        ll.origin + ll.offset + ll.size * vec2(-.5f, -.5f),
                        ll.origin + ll.offset + ll.size * vec2(+.5f, -.5f),
                        ll.origin + ll.offset + ll.size * vec2(+.5f, +.5f),
                        ll.origin + ll.offset + ll.size * vec2(-.5f, +.5f),
                    };
                    renderer->draw_line(v[0], v[1], color, color);
                    renderer->draw_line(v[1], v[2], color, color);
                    renderer->draw_line(v[2], v[3], color, color);
                    renderer->draw_line(v[3], v[0], color, color);
                }
#endif
                if (!has_overlap) {
                    break;
                }
                vec2 midpoint = vec2_zero;
                for (std::size_t ii = 0, sz = layout.size(); ii < sz; ++ii) {
                    if (overlap_mask & (1LL << ii)) {
                        midpoint += layout[ii].origin + layout[ii].offset;
                    }
                }
                midpoint /= static_cast<float>(__popcnt64(overlap_mask));
                for (std::size_t ii = 0, sz = layout.size(); ii < sz; ++ii) {
                    if (overlap_mask & (1LL << ii)) {
                        layout[ii].offset = (layout[ii].origin + layout[ii].offset - midpoint) * minscale + midpoint - layout[ii].origin;
                    }
                }
            }
#endif
            for (auto const& elem : layout) {
                renderer->draw_string(elem.text, elem.origin + elem.offset - elem.size * .5f, elem.color);
            }
        }

        //
        //  draw cursor and highlight compartment
        //

        {
            vec2 scale = renderer->view().size / vec2(renderer->window()->size());
            vec2 s = vec2(scale.x, -scale.y), b = vec2(0.f, static_cast<float>(renderer->window()->size().y)) * scale;
            // cursor position in world space
            vec2 world_cursor = g_cursor * s + b + renderer->view().origin - renderer->view().size * 0.5f;

            float d = renderer->view().size.y * (1.f / 50.f);
            renderer->draw_line(world_cursor - vec2(d,0), world_cursor + vec2(d,0), color4(1,0,0,1), color4(1,0,0,1));
            renderer->draw_line(world_cursor - vec2(0,d), world_cursor + vec2(0,d), color4(1,0,0,1), color4(1,0,0,1));

            // cursor position in ship-local space
            vec2 local_cursor = world_cursor * get_inverse_transform(time);

            uint16_t idx = _state.layout().intersect_compartment(local_cursor);
            if (idx != ship_layout::invalid_compartment) {
                auto const& c = _layout.compartments()[idx];
                for (int jj = 0; jj < c.num_vertices; ++jj) {
                    positions[jj] = _state.layout().vertices()[c.first_vertex + jj] * transform;
                }
                for (int jj = 1; jj < c.num_vertices; ++jj) {
                    renderer->draw_line(positions[jj-1], positions[jj], color4(0,0,0,1), color4(0,0,0,1));
                }
                renderer->draw_line(positions[c.num_vertices - 1], positions[0], color4(0,0,0,1), color4(0,0,0,1));
            }

            //
            //  draw path from cursor to random point in ship
            //

#if 0
            {
                static random r;
                mat3 tx = get_transform(time);
                static vec2 goal = vec2_zero;
                if (get_world()->framenum() % 100 == 0) {
                    do {
                        goal = vec2(r.uniform_real(), r.uniform_real()) * (_model->bounds().maxs() - _model->bounds().mins()) + _model->bounds().mins();
                    } while (_layout.intersect_compartment(goal) == ship_layout::invalid_compartment);
                }

                vec2 buffer[64];
                std::size_t len = _layout.find_path(local_cursor, goal, .3f, buffer, narrow_cast<int>(countof(buffer)));
                for (std::size_t ii = 1; ii < len; ++ii) {
                    renderer->draw_line(buffer[ii - 1] * tx, buffer[ii] * tx, color4(1,0,0,1), color4(1,0,0,1));
                }

                {
                    vec2 end = goal * tx;
                    vec2 v1 = vec2( d, d) * (1.f / math::sqrt2<float>);
                    vec2 v2 = vec2(-d, d) * (1.f / math::sqrt2<float>);
                    renderer->draw_line(end - v1, end + v1, color4(1,0,0,1), color4(1,0,0,1));
                    renderer->draw_line(end - v2, end + v2, color4(1,0,0,1), color4(1,0,0,1));
                }
            }
#endif
        }

        //
        //  draw compartment state line graph
        //

#if 0
        {
            constexpr std::size_t count = countof<decltype(ship_state::compartment_state::history)>();
            for (std::size_t ii = 0, sz = _layout.compartments().size(); ii < sz; ++ii) {
                auto const& s = _state.compartments()[ii];
                vec2 scale = renderer->view().size * .95f;
                vec2 bias = renderer->view().origin - scale * .5f;
                for (int jj = 1; jj < _state.framenum && jj < count; ++jj) {
                    float x0 = (count - jj - 1) / float(count);
                    float x1 = (count - jj - 0) / float(count);
                    float y0 = s.history[(_state.framenum - jj - 1) % count];
                    float y1 = s.history[(_state.framenum - jj - 0) % count];
                    renderer->draw_line(vec2(x0, y0) * scale + bias, vec2(x1, y1) * scale + bias, color4(1,1,1,1), color4(1,1,1,1));
                }
            }
        }
#endif
#if 0
        {
            mat3 tx = get_transform(lerp);
            mat2 rot = mat2::rotate(get_rotation(lerp));
            renderer->draw_line(get_position(lerp), get_position(lerp) + get_linear_velocity() * 2.f, color4(1,0,0,1), color4(1,0,0,1));
            renderer->draw_line(get_position(lerp), get_position(lerp) + _engines->get_move_target().normalize() * 32.f, color4(0,1,0,1), color4(0,1,0,1));

            mat2 lt = mat2::rotate(get_rotation(lerp) + _engines->get_look_target());
            renderer->draw_line(get_position(lerp), get_position(lerp) + vec2(32,0) * lt, color4(1,1,0,1), color4(1,1,0,1));
            renderer->draw_line(get_position(lerp), get_position(lerp) + vec2(32,0) * rot, color4(1,.5,0,1), color4(1,.5,0,1));
            renderer->draw_line(get_position(lerp) + vec2(32,0) * rot, get_position(lerp) + vec2(32,get_angular_velocity()*32) * rot, color4(1,.5,0,1), color4(1,.5,0,1));
        }
#endif
    }
}

//------------------------------------------------------------------------------
bool ship::touch(object* /*other*/, physics::collision const* /*collision*/)
{
    return !_is_destroyed;
}

//------------------------------------------------------------------------------
void ship::think()
{
    time_value time = get_world()->frametime();

    if (_dead_time > time && _reactor && _reactor->damage() == _reactor->maximum_power()) {
        _dead_time = time;
    }

    _state.think();
#if 0
    for (std::size_t ii = 0, sz = _state.connections().size(); ii < sz; ++ii) {
        if (_random.uniform_real() < .005f) {
            _state.set_connection(narrow_cast<uint16_t>(ii), !_state.connections()[ii].opened);
        }
    }
#endif

    if (_dead_time > time) {
        for (auto& subsystem : _subsystems) {
            if (subsystem->damage()) {
                subsystem->repair(1.f / 15.f);
                break;
            }
        }
        _shield->recharge(1.f / 5.f);
    }

    //
    // Update doors
    //

    {
        std::vector<bool> open(_state.connections().size(), false);
        for (std::size_t ii = 0; ii < _state.connections().size(); ++ii) {
            auto const& conn = _state.layout().connections()[ii];
            for (auto const& ch : _crew) {
                if (!ch->get_path_length()) {
                    continue;
                }
                uint16_t c0 = _state.layout().intersect_compartment(ch->get_path(-1));
                uint16_t c1 = _state.layout().intersect_compartment(ch->get_path(0));
                if (conn.compartments[0] == c0 && conn.compartments[1] == c1
                        || conn.compartments[0] == c1 && conn.compartments[1] == c0) {
                    open[ii] = true;
                }
            }
        }
        for (std::size_t ii = 0; ii < _state.connections().size(); ++ii) {
            _state.set_connection_automatic(narrow_cast<uint16_t>(ii), open[ii]);
        }
    }

    //
    // Death sequence
    //

    if (time > _dead_time) {
        if (time - _dead_time < destruction_time) {
            float t = min(.8f, (time - _dead_time) / destruction_time);
            float s = powf(_random.uniform_real(), 6.f * (1.f - t));

            // random explosion at a random point on the ship
            if (s > .2f) {
                // find a random point on the ship's model
                vec2 v;
                do {
                    v = _model->bounds().mins() + _model->bounds().size() * vec2(_random.uniform_real(), _random.uniform_real());
                } while (!_model->contains(v));

                get_world()->add_effect(time, effect_type::explosion, v * get_transform(), vec2_zero, .2f * s);
                if (s * s > t) {
                    sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
                    get_world()->add_sound(_sound_explosion, get_position(), .2f * s);
                }
            }
        } else if (!_is_destroyed) {
            // add final explosion effect
            get_world()->add_effect(time, effect_type::explosion, get_position(), vec2_zero);
            sound::asset _sound_explosion = pSound->load_sound("assets/sound/cannon_impact.wav");
            get_world()->add_sound(_sound_explosion, get_position());

            // remove all subsystems
            _crew.clear();
            _subsystems.clear();
            _weapons.clear();

            _is_destroyed = true;
        }
    }
}

//------------------------------------------------------------------------------
void ship::read_snapshot(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void ship::write_snapshot(network::message& /*message*/) const
{
}

//------------------------------------------------------------------------------
void ship::damage(object* inflictor, vec2 /*point*/, float amount)
{
    // get list of subsystems that can take additional damage
    std::vector<subsystem*> subsystems;
    for (auto& subsystem : _subsystems) {
        if (subsystem->damage() < subsystem->maximum_power()) {
            subsystems.push_back(subsystem.get());
        }
    }

    // apply damage to a random subsystem
    if (subsystems.size()) {
        std::size_t idx = _random.uniform_int(subsystems.size());
        subsystems[idx]->damage(inflictor, amount * 6.f);
        for (auto& ch : _crew) {
            if (ch->compartment() == subsystems[idx]->compartment()) {
                ch->damage(inflictor, amount);
            }
        }
    }
}

//------------------------------------------------------------------------------
void ship::update_usercmd(game::usercmd usercmd)
{
    _usercmd = usercmd;
}

} // namespace game

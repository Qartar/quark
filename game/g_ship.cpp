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
    {
        /*  0 */ {{7.f, 7.f}, {10.f, 5.f}, {10.f, -5.f}, {7.f, -7.f}, {7.f, -1.f}, {7.f, 1.f}},
        /*  1 */ {{6.f, 1.f}, {6.f, -1.f}, {6.f, -2.f}, {4.f, -2.f}, {2.f, -2.f}, {0.f, -2.f}, {0.f, -1.f}, {0.f, 1.f}, {0.f, 2.f}, {2.f, 2.f}, {4.f, 2.f}, {6.f, 2.f}},
        /*  2 */ {{0.f, -3.f}, {2.f, -3.f}, {4.f, -3.f}, {6.f, -3.f}, {6.f, -7.f}, {0.f, -8.f}},
        /*  3 */ {{6.f, 3.f}, {4.f, 3.f}, {2.f, 3.f}, {0.f, 3.f}, {0.f, 8.f}, {6.f, 7.f}},
        /*  4 */ {{-1.f, 1.f}, {-1.f, -1.f}, {-1.f, -3.f}, {-6.f, -3.f}, {-6.f, -1.f}, {-6.f, 1.f}, {-6.f, 3.f}, {-1.f, 3.f}},
        /*  5 */ {{-1.f, 4.f}, {-6.f, 4.f}, {-7.f, 7.f}, {-7.f, 9.f}, {-3.f, 9.f}, {-1.f, 8.f}},
        /*  6 */ {{-1.f, -4.f}, {-1.f, -8.f}, {-3.f, -9.f}, {-7.f, -9.f}, {-7.f, -7.f}, {-6.f, -4.f}},
        /*  7 */ {{-8.f, 7.f}, {-14.f, 7.f}, {-14.f, 9.f}, {-8.f, 9.f}},
        /*  8 */ {{-14.f, -7.f}, {-8.f, -7.f}, {-8.f, -9.f}, {-14.f, -9.f}},

        /*  9 */ {{7.f, 1.f}, {7.f, -1.f}, {6.f, -1.f}, {6.f, 1.f}},
        /* 10 */ {{7.f, -4.f}, {7.f, -6.f}, {6.f, -6.f}, {6.f, -4.f}},
        /* 11 */ {{7.f, 6.f}, {7.f, 4.f}, {6.f, 4.f}, {6.f, 6.f}},
        /* 12 */ {{-2.5f, 4.f}, {-2.5f, 3.f}, {-4.5f, 3.f}, {-4.5f, 4.f}},
        /* 13 */ {{-4.5f, -4.f}, {-4.5f, -3.f}, {-2.5f, -3.f}, {-2.5f, -4.f}},
        /* 14 */ {{0.f, 1.f}, {0.f, -1.f}, {-1.f, -1.f}, {-1.f, 1.f}},
        /* 15 */ {{4.f, -2.f}, {4.f, -3.f}, {2.f, -3.f}, {2.f, -2.f}},
        /* 16 */ {{4.f, 3.f}, {4.f, 2.f}, {2.f, 2.f}, {2.f, 3.f}},
        /* 17 */ {{-7.f, 9.f}, {-7.f, 7.f}, {-8.f, 7.f}, {-8.f, 9.f}},
        /* 18 */ {{-7.f, -7.f}, {-7.f, -9.f}, {-8.f, -9.f}, {-8.f, -7.f}},
        /* 19 */ {{-6.f, 1.f}, {-6.f, -1.f}, {-7.f, -1.f}, {-7.f, 1.f}},
    },
    {
        /* X - 4 */ {{ ship_layout::invalid_compartment, 19}}, {{4, 19}},
        /* 0 - 1 */ {{ 0,  9}}, {{ 1,  9}},
        /* 0 - 2 */ {{ 0, 10}}, {{ 2, 10}},
        /* 0 - 3 */ {{ 0, 11}}, {{ 3, 11}},
        /* 1 - 2 */ {{ 1, 15}}, {{ 2, 15}},
        /* 1 - 3 */ {{ 1, 16}}, {{ 3, 16}},
        /* 1 - 4 */ {{ 1, 14}}, {{ 4, 14}},
        /* 4 - 5 */ {{ 4, 12}}, {{ 5, 12}},
        /* 4 - 6 */ {{ 4, 13}}, {{ 6, 13}},
        /* 5 - 7 */ {{ 5, 17}}, {{ 7, 17}},
        /* 6 - 8 */ {{ 6, 18}}, {{ 8, 18}},
    }
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

    enum style_t {
        extra_crewmember,
        extra_shields,
        extra_weapon,
    } style = (style_t)(_random.uniform_int(3));

    for (int ii = 0; ii < 3; ++ii) {
        _crew.push_back(get_world()->spawn<character>());
        _crew.back()->set_position(this, vec2_zero, true);
    }
    if (style == extra_crewmember) {
        _crew.push_back(get_world()->spawn<character>());
        _crew.back()->set_position(this, vec2_zero, true);
    }

    std::vector<uint16_t> compartments;
    compartments.reserve(_state.compartments().size());
    for (std::size_t ii = 0, sz = _state.compartments().size(); ii < sz; ++ii) {
        if (_state.layout().compartments()[ii].area < 3.f) {
            continue;
        }
        compartments.push_back(narrow_cast<uint16_t>(ii));
    }

    std::shuffle(compartments.begin(), compartments.end(), _random.engine());
    std::size_t num_subsystems = 0;

    _reactor = get_world()->spawn<subsystem>(this, compartments[num_subsystems++], subsystem_info{subsystem_type::reactor, 11});
    _subsystems.push_back(_reactor);

    _shield = get_world()->spawn<game::shield>(&_shape, this, compartments[num_subsystems++], style == extra_shields ? 3 : 2);
    _subsystems.push_back(_shield);

    _engines = get_world()->spawn<game::engines>(this, compartments[num_subsystems++], engines_info{16.f, .125f, 8.f, .0625f, .5f, .5f});
    _subsystems.push_back(_engines);

    int num_weapons = style == extra_weapon ? 3 : 2;
    for (int ii = 0; ii < num_weapons; ++ii) {
        weapon_info info = weapon::by_random(_random);
        _weapons.push_back(get_world()->spawn<weapon>(this, compartments[num_subsystems++], info, vec2(11.f, ii ? 6.f : -6.f)));
        _subsystems.push_back(_weapons.back());
    }

    _subsystems.push_back(get_world()->spawn<subsystem>(this, compartments[num_subsystems++], subsystem_info{subsystem_type::life_support, 1}));
    if (style != extra_weapon) {
        _subsystems.push_back(get_world()->spawn<subsystem>(this, compartments[num_subsystems++], subsystem_info{subsystem_type::medical_bay, 1}));
    }

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

        constexpr color4 subsystem_colors[2][3] = {
            { color4(.4f, 1.f, .2f, .225f), color4(1.f, .2f, 0.f, .333f), color4(1.f, .8f, .1f, .300f), },
            { color4(.4f, 1.f, .2f, 1.00f), color4(1.f, .2f, 0.f, 1.00f), color4(1.f, .8f, .1f, 1.00f), },
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
                int repaired = 0;
                for (auto const& ch : _crew) {
                    if (ch->is_repairing(_reactor)) {
                        ++repaired;
                    }
                }
                int damaged = static_cast<int>(_reactor->maximum_power() - std::ceil(_reactor->damage() - .2f));
                int powered = ii < _reactor->current_power() ? 1 : 0;
                int status = ii - repaired >= damaged ? 1 : ii >= damaged ? 2 : 0;
                if (status == 2 && ii < _reactor->desired_power()) {
                    powered = true;
                }

                color4 c = subsystem_colors[powered][status]; c.a *= alpha;

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
                int repaired = 0;
                for (auto const& ch : _crew) {
                    if (ch->is_repairing(subsystem)) {
                        ++repaired;
                    }
                }
                int damaged = static_cast<int>(subsystem->maximum_power() - std::ceil(subsystem->damage() - .2f));
                int powered = ii < subsystem->current_power() ? 1 : 0;
                int status = ii - repaired >= damaged ? 1 : ii >= damaged ? 2 : 0;
                if (status == 2 && ii < subsystem->desired_power()) {
                    powered = true;
                }

                color4 c = subsystem_colors[powered][status]; c.a *= alpha;

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
#if 1
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

#endif
#if 1
        for (std::size_t ii = 0, sz = _layout.compartments().size(); ii < sz; ++ii) {
            auto const& c = _layout.compartments()[ii];
            std::vector<vec2> vtx;
            std::vector<color4> col;
            std::vector<int> idx;
            vtx.push_back(c.shape.vertices()[0] * transform);
            col.push_back(color4(1,1,1,1));
            for (int jj = 1; jj < c.shape.num_vertices(); ++jj) {
                vtx.push_back(c.shape.vertices()[jj] * transform);
                col.push_back(color4(1,1,1,1));
                idx.push_back(0);
                idx.push_back(jj);
                idx.push_back(jj+1);
            }
            vtx.push_back(c.shape.vertices()[0] * transform);
            col.push_back(color4(1,1,1,1));
            //renderer->draw_triangles(vtx.data(), col.data(), idx.data(), (c.shape.num_vertices() - 2) * 3);
            vtx[0] = c.inner_shape.vertices()[0] * transform;
            col[0] = color4(0.f,0.f,0.f,.1f);
            for (int jj = 1; jj < c.inner_shape.num_vertices(); ++jj) {
                vtx[jj] = c.inner_shape.vertices()[jj] * transform;
                col[jj] = color4(0.f,0.f,0.f,.1f);
            }
            vtx[c.inner_shape.num_vertices()] = c.inner_shape.vertices()[0] * transform;
            col[c.inner_shape.num_vertices()] = color4(0.f,0.f,0.f,.1f);
            renderer->draw_triangles(vtx.data(), col.data(), idx.data(), (c.inner_shape.num_vertices() - 2) * 3);
        }
#endif
#if 1
        positions.clear();
        colors.clear();
        indices.clear();
        for (std::size_t ii = 0, sz = _layout.connections().size(); ii < sz; ++ii) {
            auto const& c = _layout.connections()[ii];
            auto const& s = _state.connections()[ii];
            indices.push_back(narrow_cast<int>(ii * 4 + 0));
            indices.push_back(narrow_cast<int>(ii * 4 + 1));
            indices.push_back(narrow_cast<int>(ii * 4 + 2));
            indices.push_back(narrow_cast<int>(ii * 4 + 1));
            indices.push_back(narrow_cast<int>(ii * 4 + 3));
            indices.push_back(narrow_cast<int>(ii * 4 + 2));
            vec2 v0 = _layout.vertices()[c.vertices[0]] * transform;
            vec2 v1 = _layout.vertices()[c.vertices[1]] * transform;
            vec2 n = (v1 - v0).cross(1.f).normalize();
            positions.push_back(v0 - n * .1f);
            positions.push_back(v0 + n * .1f);
            positions.push_back(v1 - n * .1f);
            positions.push_back(v1 + n * .1f);
            if (s.opened) {
                colors.push_back(color4(1,1,0,1));
                colors.push_back(color4(1,1,0,1));
                colors.push_back(color4(1,1,0,1));
                colors.push_back(color4(1,1,0,1));
            } else if (s.opened_automatic) {
                colors.push_back(color4(0,1,0,1));
                colors.push_back(color4(0,1,0,1));
                colors.push_back(color4(0,1,0,1));
                colors.push_back(color4(0,1,0,1));
            } else {
                colors.push_back(color4(0,1,1,1));
                colors.push_back(color4(0,1,1,1));
                colors.push_back(color4(0,1,1,1));
                colors.push_back(color4(0,1,1,1));
            }
        }

        renderer->draw_triangles(positions.data(), colors.data(), indices.data(), indices.size());
#endif

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

        if (1) {
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
        } else if (1) {
            mat3 tx = get_transform(time);

            for (std::size_t ii = 0, sz = _layout.compartments().size(); ii < sz; ++ii) {
                auto const& c = layout().compartments()[ii];
                vec2 pt = vec2_zero;
                for (int jj = 0; jj < c.num_vertices; ++jj) {
                    pt += layout().vertices()[c.first_vertex + jj];
                }
                pt /= c.num_vertices;
                string::view text = va("%d", ii);

                vec2 txsz = renderer->string_size(text);
                float t = txsz.length_sqr() / (_model->bounds().maxs() - _model->bounds().mins()).length_sqr();
                float a = alpha * clamp(1.f - 16.f * t, 0.f, 1.f);
                renderer->draw_string(text, pt * tx - txsz * .5f, color4(.09f,.225f,.045f,a));
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

// g_world.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_aicontroller.h"
#include "g_projectile.h"
#include "g_ship.h"
#include "g_player.h"
#include "p_collide.h"
#include "p_trace.h"

#include <algorithm>
#include <set>

////////////////////////////////////////////////////////////////////////////////
namespace game {

std::array<world*, world::max_worlds> world::_singletons{};

//------------------------------------------------------------------------------
world::world()
    : _sequence(0)
    , _physics(
        std::bind(&world::physics_filter_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&world::physics_collide_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
    for (_index = 0; _index < max_worlds; ++_index) {
        if (!_singletons[_index]) {
            _singletons[_index] = this;
            break;
        }
    }

    assert(_index < max_worlds);
}

//------------------------------------------------------------------------------
world::~world()
{
    assert(_singletons[_index] == this);
    _singletons[_index] = nullptr;
}

//------------------------------------------------------------------------------
void world::init()
{
    reset();
}

//------------------------------------------------------------------------------
void world::shutdown()
{
    clear();
}

//------------------------------------------------------------------------------
void world::reset()
{
    clear();

    _sequence = 0;
    _framenum = 0;

    _tiles.clear();
    _boundary_tiles.clear();

    insert_boundary_tile(vec2i(0,0));
    for (int jj = 0; jj < 64; ++jj) {
        for (int ii = 0; ii < 64; ++ii) {
            vec2i n[4] = {{-ii, -jj}, {ii, -jj}, {-ii, jj}, {ii, jj}};
            for (int kk = 0; kk < 4; ++kk) {
                vec2 r = hextile::grid_to_world(n[kk]);
                if (length_sqr(r) < 48.f * 48.f) {
                    insert_tile(n[kk], {});
                    //insert_boundary_tile({ii, jj});
                }
            }
        }
    }

    std::vector<hextile::index> river_tiles = _boundary_tiles;
    while (river_tiles.size()) {
        float f = _random.uniform_real();

        std::size_t r = std::size_t(powf(f, .9f) * float(river_tiles.size()));
        hextile::index idx = river_tiles[r];
        int dir[36];
        hextile::index adj[36];
        int n = 0;

        for (int ii = 0; ii < 6; ++ii) {
            adj[n] = _tiles[idx].neighbors[ii];
            dir[n] = ii;
            if (adj[n] == hextile::invalid_index) {
                continue;
            }
            if (_tiles[adj[n]].is_boundary) {
                continue;
            }
            if (_tiles[adj[n]].contents[6] != 0) {
                continue;
            }
            ++n;
            if (_tiles[idx].contents[(ii + 3) % 6] != 0) {
                adj[n] = adj[n - 1];
                dir[n] = dir[n - 1];
                ++n;
                adj[n] = adj[n - 1];
                dir[n] = dir[n - 1];
                ++n;
                adj[n] = adj[n - 1];
                dir[n] = dir[n - 1];
                ++n;
            }
            if (_tiles[idx].contents[(ii + 2) % 6] != 0) {
                adj[n] = adj[n - 1];
                dir[n] = dir[n - 1];
                ++n;
            }
            if (_tiles[idx].contents[(ii + 4) % 6] != 0) {
                adj[n] = adj[n - 1];
                dir[n] = dir[n - 1];
                ++n;
            }
        }

        int d = -1;
        if (n > 1) {
            d = dir[_random.uniform_int(n)];
        } else {
            if (n > 0) {
                d = dir[0];
            }
            river_tiles.erase(river_tiles.begin() + r);
        }
        if (d != -1) {
            int c = _tiles[idx].contents[6];
            if (c == 0) {
                c = int(idx + 1);
            }
            hextile::index opp = _tiles[idx].neighbors[d];
            _tiles[opp].contents[6] = c;
            _tiles[idx].contents[d] = c;
            _tiles[opp].contents[(d + 3) % 6] = c;
            river_tiles.push_back(opp);
        }
    }

    _tile_scale = .05f;
    _tile_offset = vec2_zero;

    _tile_cursor = vec2i_zero;
    _tile_rotation = 0;
}

//------------------------------------------------------------------------------
void world::clear()
{
    _objects.clear();
    // assign with empty queue because std::queue has no clear method
    _removed = std::queue<handle<game::object>>{};

    _physics_objects.clear();
    _particles.clear();
}

//------------------------------------------------------------------------------
object_range<object const> world::objects() const
{
    return object_range<object const>(
        _objects.data(),
        _objects.data() + _objects.size()
    );
}

//------------------------------------------------------------------------------
object_range<object> world::objects()
{
    return object_range<object>(
        _objects.data(),
        _objects.data() + _objects.size()
    );
}

//------------------------------------------------------------------------------
void world::remove(handle<object> object)
{
    _removed.push(object);
}

//------------------------------------------------------------------------------
hextile::index world::insert_tile(vec2i position, hextile const& tile)
{
    hextile::index index = hextile::invalid_index;

    for (hextile::index ii = 0, sz = _tiles.size(); ii < sz; ++ii) {
        // tile already exists at this position
        if (_tiles[ii].position == position) {
            if (!_tiles[ii].is_boundary) {
                return hextile::invalid_index;
            }
            index = ii;
            break;
        }
    }

    if (index == hextile::invalid_index) {
        return hextile::invalid_index;
    } else {
        _tiles[index].position = position;
        _tiles[index].contents = tile.contents;
        _tiles[index].is_boundary = false;

        for (std::size_t ii = 0, sz = _boundary_tiles.size(); ii < sz; ++ii) {
            if (_boundary_tiles[ii] == index) {
                _boundary_tiles[ii] = _boundary_tiles.back();
                _boundary_tiles.pop_back();
                break;
            }
        }
    }

    for (std::size_t jj = 0; jj < 6; ++jj) {
        hextile::index other = _tiles[index].neighbors[jj];
        if (other != hextile::invalid_index) {
            _tiles[other].neighbors[(jj + 3) % 6] = index;
        } else {
            insert_boundary_tile(_tiles[index].position + hextile::neighbor_offsets[jj]);
        }
    }

    return index;
}

//------------------------------------------------------------------------------
hextile::index world::insert_boundary_tile(vec2i position)
{
    for (hextile::index ii = 0, sz = _tiles.size(); ii < sz; ++ii) {
        // tile already exists at this position
        if (_tiles[ii].position == position) {
            return hextile::invalid_index;
        }
    }

    hextile::index index = _tiles.size();
    _tiles.push_back({position, {}, true, false, {
        hextile::invalid_index, hextile::invalid_index, hextile::invalid_index,
        hextile::invalid_index, hextile::invalid_index, hextile::invalid_index,
    }});

    for (std::size_t ii = 0, sz = _tiles.size(); ii < sz; ++ii) {
        for (std::size_t jj = 0; jj < 6; ++jj) {
            if (_tiles[ii].position == position + hextile::neighbor_offsets[jj]) {
                _tiles[index].neighbors[jj] = ii;
                _tiles[ii].neighbors[(jj + 3) % 6] = index;
            }
        }
    }

    _boundary_tiles.push_back(index);
    return index;
}

//------------------------------------------------------------------------------
bool world::match_tile(hextile::index index, hextile const& tile, int rotation) const
{
    for (std::size_t ii = 0; ii < 6; ++ii) {
        hextile::index neighbor = _tiles[index].neighbors[ii];
        if (neighbor == hextile::invalid_index || _tiles[neighbor].is_boundary) {
            continue;
        }

        if (_tiles[neighbor].contents[(ii + 3) % 6] != tile.contents[(ii + rotation) % 6]) {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
void draw_tile(render::system* renderer, hextile const& tile)
{
    vec2 origin = hextile::grid_to_world(tile.position);

    float alpha = (tile.is_boundary || tile.is_candidate) ? .25f : 1.f;

    config::boolean draw_grid("draw_grid", false, 0, "");

    if (draw_grid) {
        renderer->draw_line(origin + hextile::vertices[0], origin + hextile::vertices[1], color4(1,1,1,alpha), color4(1,1,1,alpha));
        renderer->draw_line(origin + hextile::vertices[1], origin + hextile::vertices[2], color4(1,1,1,alpha), color4(1,1,1,alpha));
        renderer->draw_line(origin + hextile::vertices[2], origin + hextile::vertices[3], color4(1,1,1,alpha), color4(1,1,1,alpha));
        renderer->draw_line(origin + hextile::vertices[3], origin + hextile::vertices[4], color4(1,1,1,alpha), color4(1,1,1,alpha));
        renderer->draw_line(origin + hextile::vertices[4], origin + hextile::vertices[5], color4(1,1,1,alpha), color4(1,1,1,alpha));
        renderer->draw_line(origin + hextile::vertices[5], origin + hextile::vertices[0], color4(1,1,1,alpha), color4(1,1,1,alpha));
    }

    if (tile.is_boundary) {
        return;
    } else if (tile.is_candidate) {
        vec2 const verts[6] = {
            origin + hextile::vertices[0],
            origin + hextile::vertices[1],
            origin + hextile::vertices[2],
            origin + hextile::vertices[3],
            origin + hextile::vertices[4],
            origin + hextile::vertices[5],
        };
        color4 const colors[6] = {
            color4(1.f, 1.f, 1.f, .05f),
            color4(1.f, 1.f, 1.f, .05f),
            color4(1.f, 1.f, 1.f, .05f),
            color4(1.f, 1.f, 1.f, .05f),
            color4(1.f, 1.f, 1.f, .05f),
            color4(1.f, 1.f, 1.f, .05f),
        };
        int const indices[12] = {
            0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5,
        };
        renderer->draw_triangles(verts, colors, indices, 12);
    } else {
        vec2 const verts[12] = {
            origin + hextile::vertices[0],
            origin + hextile::vertices[1],
            origin + hextile::vertices[2],
            origin + hextile::vertices[3],
            origin + hextile::vertices[4],
            origin + hextile::vertices[5],
            origin + hextile::vertices[0] * .5f,
            origin + hextile::vertices[1] * .5f,
            origin + hextile::vertices[2] * .5f,
            origin + hextile::vertices[3] * .5f,
            origin + hextile::vertices[4] * .5f,
            origin + hextile::vertices[5] * .5f,
        };

        int const indices[] = {
            0, 6, 7, 0, 7, 1,
            1, 7, 8, 1, 8, 2,
            2, 8, 9, 2, 9, 3,
            3, 9, 10, 3, 10, 4,
            4, 10, 11, 4, 11, 5,
            5, 11, 6, 5, 6, 0,
            6, 7, 8, 6, 8, 9, 6, 9, 10, 6, 10, 11,
        };
#if 0
        constexpr color4 contents_color[7] = {
            color4(1.f, 1.f, 1.f, .2f),
            color4(1.f, 0.f, 0.f, .3f),
            color4(.2f, .4f, 1.f, .3f),
            color4(.4f, 1.f, 0.f, .3f),
            color4(1.f, 1.f, 0.f, .3f),
            color4(.2f, 1.f, 1.f, .3f),
            color4(1.f, .2f, 1.f, .3f),
        };
#else
        auto contents_color = [](int i) -> color4 {
            if (i == 0) {
                return color4(1.f, 1.f, 1.f, .2f);
            }
            return color4(
                sin(float(i)/3.f) * .5f + .5f,
                sin(float(i)/5.f) * .5f + .5f,
                sin(float(i)/7.f) * .5f + .5f,
                .3f).normalize();
        };
#endif
        color4 colors[12] = {};

        for (int ii = 0; ii < 6; ++ii) {
            int const* iptr = indices + ii * 6;
            for (int jj = 0; jj < 6; ++jj) {
                colors[iptr[jj]] = contents_color(tile.contents[ii]);
            }

            renderer->draw_triangles(verts, colors, iptr, 6);
        }
        {
            int const* iptr = indices + 36;
            for (int jj = 0; jj < 12; ++jj) {
                colors[iptr[jj]] = contents_color(tile.contents[6]);
            }
            renderer->draw_triangles(verts, colors, iptr, 12);
        }
    }
}

//------------------------------------------------------------------------------
void world::draw_tiles(render::system* renderer) const
{
    render::view view{};

    view.origin = _tile_offset;
    view.angle = 0.f;
    view.raster = false;
    view.size = vec2(renderer->window()->size()) * _tile_scale;

    renderer->set_view(view);

    for (auto const& tile : _tiles) {
        draw_tile(renderer, tile);
    }

    //
    //  draw candidates
    //

    for (std::size_t ii = 0, sz = _candidates.size(); ii < sz; ++ii) {
        if (ii > 0 && _candidates[ii].first == _candidates[ii - 1].first) {
            continue;
        }
        hextile tile{_tiles[_candidates[ii].first].position, {}, false, true};
        draw_tile(renderer, tile);
    }

    {
        vec2 world_cursor = _usercmd.cursor * view.size - view.size * .5f + _tile_offset;
        _tile_cursor = hextile::world_to_grid(world_cursor);

        std::size_t first_candidate, last_candidate;
        for (first_candidate = 0; first_candidate < _candidates.size(); ++first_candidate) {
            if (_tiles[_candidates[first_candidate].first].position == _tile_cursor) {
                break;
            }
        }
        for (last_candidate = first_candidate; last_candidate < _candidates.size(); ++last_candidate) {
            if (_tiles[_candidates[last_candidate].first].position != _tile_cursor) {
                break;
            }
        }

        if (first_candidate < last_candidate) {
            int n = _tile_rotation % (last_candidate - first_candidate);
            std::size_t c = n < 0 ? last_candidate - n : first_candidate + n;
            int r = _candidates[c].second;

            hextile tile{_tile_cursor, _next.contents, false, false};

            std::rotate(tile.contents.begin(), tile.contents.begin() + r, tile.contents.begin() + 6);
            draw_tile(renderer, tile);
        } else {
            hextile tile{_tile_cursor, {}, false, true};
            draw_tile(renderer, tile);
        }
    }
}

//------------------------------------------------------------------------------
void world::draw(render::system* renderer, time_value /*time*/) const
{
    draw_tiles(renderer);
}

//------------------------------------------------------------------------------
void world::run_frame()
{
    _message.reset();

    ++_framenum;

    while (_removed.size()) {
        if (_removed.front()) {
            _objects[_removed.front().get_index()] = nullptr;
        }
        _removed.pop();
    }

    for (std::size_t ii = 0; ii < _objects.size(); ++ii) {
        // objects array is sparse
        if (!_objects[ii].get()) {
            continue;
        }

        // objects can spawn other objects, do not think this frame
        if (_objects[ii]->_spawn_time == frametime()) {
            continue;
        }

        _objects[ii]->think();

        _objects[ii]->_old_position = _objects[ii]->get_position();
        _objects[ii]->_old_rotation = _objects[ii]->get_rotation();
    }

    _physics.step(FRAMETIME.to_seconds());

#if 0
    if (_framenum % 120 == 0 && _candidates.size()) {
        std::vector<std::pair<hextile::index, int>> candidates[7];

        for (std::size_t ii = 0, sz = _candidates.size(); ii < sz; ++ii) {
            hextile::index jj = _candidates[ii].first;
            int n = 0;
            for (auto other : _tiles[jj].neighbors) {
                if (other != hextile::invalid_index) {
                    ++n;
                }
            }
            candidates[n].push_back(_candidates[ii]);
        }

        for (int jj = 0; jj < 7; ++jj) {
            if (candidates[6 - jj].size()) {
                std::size_t ii = _random.uniform_int(candidates[6 - jj].size());
                vec2i position = _tiles[candidates[6 - jj][ii].first].position;
                int rotation = candidates[6 - jj][ii].second;
                std::rotate(_next.contents.begin(), _next.contents.begin() + rotation, _next.contents.begin() + 6);
                insert_tile(position, _next);
                break;
            }
        }

        _candidates.resize(0);
    }
#endif

    if (!_candidates.size()) {
        for (std::size_t kk = 0; kk < 6; ++kk) {
            if (_random.uniform_real() < .5f) {
                _next.contents[kk] = 0;
            } else {
                _next.contents[kk] = 3 + _random.uniform_int(4);
            }
        }
        // randomly select center contents from edge contents
        _next.contents[6] = _next.contents[_random.uniform_int(6)];

        int r = _random.uniform_int(12);
        if (r < 2) {
            _next.contents[0] = r + 1;
            _next.contents[6] = r + 1;
            int r1 = _random.uniform_int(10);
            if (r1 < 5) {
                _next.contents[3] = r + 1;
            } else if (r1 < 7) {
                _next.contents[2] = r + 1;
            } else if (r1 < 9) {
                _next.contents[4] = r + 1;
            } else {
                _next.contents[2] = r + 1;
                _next.contents[4] = r + 1;
            }
        }

        // find all boundary tiles where this tile could fit
        for (std::size_t ii = 0, sz = _boundary_tiles.size(); ii < sz; ++ii) {
            for (int rotation = 0; rotation < 6; ++rotation) {
                if (match_tile(_boundary_tiles[ii], _next, rotation)) {
                    _candidates.emplace_back(_boundary_tiles[ii], rotation);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void world::update_usercmd(usercmd cmd, time_value /*time*/)
{
    _usercmd = cmd;

    bool is_cursor_active = false;
    for (auto const& candidate : _candidates) {
        if (_tiles[candidate.first].position == _tile_cursor) {
            is_cursor_active = true;
            break;
        }
    }

    switch (_usercmd.action) {
        case usercmd::action::zoom_in:
            if (is_cursor_active) {
                ++_tile_rotation;
            } else {
                _tile_scale /= 1.1f;
            }
            break;

        case usercmd::action::zoom_out:
            if (is_cursor_active) {
                --_tile_rotation;
            } else {
                _tile_scale *= 1.1f;
            }
            break;

        default:
            break;
    }

    if ((_usercmd.buttons & usercmd::button::select) == usercmd::button::select) {
        std::size_t first_candidate, last_candidate;
        for (first_candidate = 0; first_candidate < _candidates.size(); ++first_candidate) {
            if (_tiles[_candidates[first_candidate].first].position == _tile_cursor) {
                break;
            }
        }
        for (last_candidate = first_candidate; last_candidate < _candidates.size(); ++last_candidate) {
            if (_tiles[_candidates[last_candidate].first].position != _tile_cursor) {
                break;
            }
        }

        if (first_candidate < last_candidate) {
            int n = _tile_rotation % (last_candidate - first_candidate);
            std::size_t c = n < 0 ? last_candidate - n : first_candidate + n;
            int r = _candidates[c].second;

            hextile tile{_tile_cursor, _next.contents, false, false};

            std::rotate(tile.contents.begin(), tile.contents.begin() + r, tile.contents.begin() + 6);
            insert_tile(_tile_cursor, tile);

            _candidates.clear();
        }
    }
}

//------------------------------------------------------------------------------
void world::read_snapshot(network::message& message)
{
    while (_removed.size()) {
        if (_removed.front()) {
            _objects[_removed.front().get_index()] = nullptr;
        }
        _removed.pop();
    }

    while (message.bytes_remaining()) {
        message_type type = static_cast<message_type>(message.read_byte());
        if (type == message_type::none) {
            break;
        }

        switch (type) {
            case message_type::frame:
                read_frame(message);
                break;

            case message_type::sound:
                read_sound(message);
                break;

            case message_type::effect:
                read_effect(message);
                break;

            default:
                break;
        }
    }
}

//------------------------------------------------------------------------------
void world::read_frame(network::message const& /*message*/)
{
}

//------------------------------------------------------------------------------
void world::read_sound(network::message const& message)
{
    int asset = message.read_long();
    vec2 position = message.read_vector();
    float volume = message.read_float();

    add_sound(static_cast<sound::asset>(asset), position, volume);
}

//------------------------------------------------------------------------------
void world::read_effect(network::message const& message)
{
    float time = message.read_float();
    int type = message.read_byte();
    vec2 pos = message.read_vector();
    vec2 vel = message.read_vector();
    float strength = message.read_float();

    add_effect(time_value::from_seconds(time), static_cast<game::effect_type>(type), pos, vel, strength);
}

//------------------------------------------------------------------------------
void world::write_snapshot(network::message& message) const
{
    message.write_byte(svc_snapshot);

    // write frame
    message.write_byte(narrow_cast<uint8_t>(message_type::frame));
    message.write_long(_framenum);

    // write active objects
    // ...

    // write sounds and effects
    message.write(_message);
    message.write_byte(narrow_cast<uint8_t>(message_type::none));

    _message.rewind();
}

//------------------------------------------------------------------------------
void world::write_sound(sound::asset sound_asset, vec2 position, float volume)
{
    _message.write_byte(narrow_cast<uint8_t>(message_type::sound));
    _message.write_long(narrow_cast<int>(sound_asset));
    _message.write_vector(position);
    _message.write_float(volume);
}

//------------------------------------------------------------------------------
void world::write_effect(time_value time, effect_type type, vec2 position, vec2 direction, float strength)
{
    _message.write_byte(narrow_cast<uint8_t>(message_type::effect));
    _message.write_float(time.to_seconds());
    _message.write_byte(narrow_cast<uint8_t>(type));
    _message.write_vector(position);
    _message.write_vector(direction);
    _message.write_float(strength);
}

//------------------------------------------------------------------------------
game::object* world::trace(physics::contact& contact, vec2 start, vec2 end, game::object const* ignore) const
{
    struct candidate {
        float fraction;
        physics::contact contact;
        game::object* object;

        bool operator<(candidate const& other) const {
            return fraction < other.fraction;
        }
    };

    std::set<candidate> candidates;
    for (auto& other : _physics_objects) {
        if (other.second == ignore || other.second->_owner == ignore) {
            continue;
        }

        auto tr = physics::trace(other.first, start, end);
        if (tr.get_fraction() < 1.0f) {
            candidates.insert(candidate{
                tr.get_fraction(),
                tr.get_contact(),
                other.second}
            );
        }
    }

    if (candidates.size()) {
        contact = candidates.begin()->contact;
        return candidates.begin()->object;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void world::add_sound(sound::asset sound_asset, vec2 position, float volume)
{
    write_sound(sound_asset, position, volume);
    pSound->play(sound_asset, vec3(position), volume, 1.0f);
}

//------------------------------------------------------------------------------
void world::add_body(game::object* owner, physics::rigid_body* body)
{
    _physics.add_body(body);
    _physics_objects[body] = owner;
}

//------------------------------------------------------------------------------
void world::remove_body(physics::rigid_body* body)
{
    _physics.remove_body(body);
    _physics_objects.erase(body);
}

//------------------------------------------------------------------------------
bool world::physics_filter_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b)
{
    game::object* obj_a = _physics_objects[body_a];
    game::object* obj_b = _physics_objects[body_b];

    if (obj_b->is_type<projectile>()) {
        return false;
    }

    game::object const* owner_a = obj_a->_owner ? obj_a->_owner.get() : obj_a;
    game::object const* owner_b = obj_b->_owner ? obj_b->_owner.get() : obj_b;

    if (owner_a == owner_b) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool world::physics_collide_callback(physics::rigid_body const* body_a, physics::rigid_body const* body_b, physics::collision const& collision)
{
    game::object* obj_a = _physics_objects[body_a];
    game::object* obj_b = _physics_objects[body_b];

    return obj_a->touch(obj_b, &collision);
}

} // namespace game

// g_player.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_player.h"
#include "g_train.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type player::_type(object::_type);

//------------------------------------------------------------------------------
float determinant(vec2 a, vec2 b)
{
#if 0
    float w = a.y * b.x;
    float c = fma(-a.y, b.x, w);
    float d = fma(a.x, b.y, -w);
    return c + d;
#else
    double c = a.x * b.y;
    double d = a.y * b.x;
    return float(c - d);
#endif
}

//------------------------------------------------------------------------------
std::vector<clothoid::segment> connect_linear_segments(vec2 p0, vec2 t0, vec2 p1, vec2 t1)
{
    float theta = acos(dot(t0, t1));
    mat2 m(t0.x, t0.y, -t0.y, t0.x);

    // normalized clothoid coordinates for the required angle
    vec2 s;
    float t = sqrt(theta / math::pi<float>);
    clothoid::segment::fresnel_integral(t, s.x, s.y);
    s *= math::pi<float>;
    if (cross(t0, t1) < 0) {
        s.y *= -1.f;
        theta *= -1.f;
    }
    // transformed coordinates of the combined normalized curves
    vec2 r = (s + vec2(s.x, -s.y) * mat2::rotate(theta)) * m;

    // calculate the maximum size of the transition curve
    float scale = min(cross(p1 - p0, t1) / cross(r, t1),
                      cross(p1 - p0, t0) / cross(r, t0));
    // length and curvature of the curves based on scaling parameter
    float length = scale * sqrt(abs(theta) * math::pi<float>);
    float curvature = theta / length;

    if (isnan(length) || length < 1.f) {
        return {};
    }

    // midpoint of the transition curves
    vec2 mid = p0 + s * m * scale;

    float den = cross(t0, t1);
    float u = cross(p1 - p0, t0) / den;
    float v = cross(p1 - p0, -t1) / den;

    if (u - v > 1e-3f) {
        // linear extension from p0
        vec2 dp = t0 * (u - v);
        return {
            clothoid::segment::from_line(p0, t0, u - v),
            clothoid::segment::from_transition(dp + p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(dp + mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
        };
    } else if (v - u > 1e-3f) {
        // linear extension from p1
        return {
            clothoid::segment::from_transition(p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
            clothoid::segment::from_line(p1 - t1 * (v - u), t1, v - u),
        };
    } else {
        // no linear extension
        return {
            clothoid::segment::from_transition(p0, t0, length, 0.f, curvature),
            clothoid::segment::from_transition(mid, t0 * mat2::rotate(.5f * theta), length, curvature, 0),
        };
    }
}

//------------------------------------------------------------------------------
std::vector<clothoid::segment> extend_linear_segment(vec2 p0, vec2 t0, vec2 p1)
{
    mat2 m(t0.x, t0.y, -t0.y, t0.x);
    float theta = .5f * math::pi<float>;

    // normalized clothoid coordinates for the maximum angle
    vec2 s;
    float t = sqrt(2.f * theta / math::pi<float>);
    clothoid::segment::fresnel_integral(t, s.x, s.y);
    s *= math::pi<float>;
    if (cross(t0, p1 - p0) < 0) {
        s.y *= -1.f;
        theta *= -1.f;
    }
    // transformed coordinates of the maximal curve
    vec2 r = s * m;

    if (cross(r, p1 - p0) * theta < 0.f) {
        float phi = atan2(cross(t0, p1 - p0), dot(t0, p1 - p0));
        // iterate to find t with the approximation:
        //  phi(t) ~= 0.519 t^2
        t = sqrt((1.f / 0.519f) * abs(phi));
        clothoid::segment::fresnel_integral(t, s.x, s.y);
        for (int ii = 0; ii < 4; ++ii) {
            float dphi = atan2(s.y, s.x) - abs(phi);
            t -= dphi * (.5f / 0.519f);
            clothoid::segment::fresnel_integral(t, s.x, s.y);
        }
        if (theta < 0.f) {
            s.y *= -1.f;
            theta = -.5f * math::pi<float> * t * t;
        } else {
            theta = .5f * math::pi<float> * t * t;
        }
        s *= math::pi<float>;
        r = s * m;
    } else {
        // project p1 onto r
        p1 = p0 + r * dot(p1 - p0, r) / r.length_sqr();
    }

    // calculate the maximum size of the transition curve
    float scale = sqrt((p1 - p0).length_sqr() / r.length_sqr());
    float length = scale * sqrt(2.f * abs(theta) * math::pi<float>);
    float curvature = 2.f * theta / length;

    if (isnan(length) || length < 1.f) {
        return {};
    }

    return {
        clothoid::segment::from_transition(p0, t0, length, 0.f, curvature),
    };
}

//------------------------------------------------------------------------------
std::vector<clothoid::segment> extend_curved_segment(vec2 p0, vec2 t0, float k0, vec2 p1)
{
    static float T = 0.f;
    float t = square(cos(T));
    float s = 1.f;//square(cos(T * .0539f));
    T += 0.01f;
    (void)p1;

    float B = t / k0;//(p1 - p0).length();
    float t2 = t * (1.f - s);
    float t1 = (1.f - t) * s + t;
    float L2 = math::pi<float> * B * (t - t2);
    float L1 = math::pi<float> * B * (t1 - t);
    float k2 = t2 / B;
    float k1 = t1 / B;

    if (L2 < 1.f && L1 < 1.f) {
        return {};
    } else if (L2 < 1.f) {
        return {
            clothoid::segment::from_transition(p0, t0, L1, k0, k1),
            clothoid::segment::from_arc(p0, t0, L1, k0),
        };
    } else if (L1 < 1.f) {
        return {
            clothoid::segment::from_transition(p0, t0, L2, k0, k2),
            clothoid::segment::from_arc(p0, t0, L2, k0),
        };
    } else {
        return {
            clothoid::segment::from_transition(p0, t0, L2, k0, k2),
            clothoid::segment::from_transition(p0, t0, L1, k0, k1),
            clothoid::segment::from_arc(p0, t0, max(L2, L1), k0),
        };
    }

}

//------------------------------------------------------------------------------
player::player()
    : _usercmd{}
    , _usercmd_time(time_value::zero)
    , _timescale_time(time_value::zero)
    , _input_state(input_state::none)
{
    _view.origin = vec2_zero;
    _view.size = vec2(640.f, 480.f);
    _view.angle = 0.f;
}

//------------------------------------------------------------------------------
player::~player()
{
}

//------------------------------------------------------------------------------
void player::spawn()
{
}

//------------------------------------------------------------------------------
void player::draw(render::system* renderer, time_value time) const
{
    (void)renderer;
    (void)time;

    mat2 rotation = mat2::rotate(renderer->view().angle);

    vec2 cursor = (_usercmd.cursor - vec2(.5f)) * renderer->view().size * rotation + renderer->view().origin;
    float sz = renderer->view().size.length() * (1.f / 512.f);

    clothoid::network::edge_index edge;
    float dist;

    if (get_world()->rail_network().get_closest_segment(cursor, 1.5f * rail_network::track_clearance, edge, dist)) {
        renderer->draw_box(vec2(sz), cursor, color4(0,1,0,1));
        vec2 spos = get_world()->rail_network().get_segment(edge).evaluate(dist);
        if (length(cursor - get_world()->rail_network().get_segment(edge).initial_position()) < .5f * rail_network::track_clearance) {
            renderer->draw_box(vec2(sz), get_world()->rail_network().get_segment(edge).initial_position(), color4(1,0,1,1));
        } else if (length(cursor - get_world()->rail_network().get_segment(edge).final_position()) < .5f * rail_network::track_clearance) {
            renderer->draw_box(vec2(sz), get_world()->rail_network().get_segment(edge).final_position(), color4(1,0,1,1));
        } else if (length(cursor - spos) >= .5f * rail_network::track_clearance) {
            vec2 tangent = get_world()->rail_network().get_segment(edge).evaluate_tangent(dist).cross(1);
            if (dot(cursor - spos, tangent) < 0.f) {
                renderer->draw_box(vec2(sz), spos - tangent * rail_network::track_clearance, color4(1,1,0,1));
            } else {
                renderer->draw_box(vec2(sz), spos + tangent * rail_network::track_clearance, color4(1,1,0,1));
            }
        } else {
            renderer->draw_box(vec2(sz), spos, color4(1,0,0,1));
            renderer->draw_arc(spos, rail_network::junction_clearance, 0, 0, 2.f * math::pi<float>, color4(1,0,0,.5f));
        }
    } else {
        renderer->draw_box(vec2(sz), cursor, color4(1,1,0,1));
    }

#if 0

#if 0
    auto connection = connect_linear_segments(
        vec2(300, 0), normalize(vec2(2, 1)), cursor, normalize(vec2(-1, 1)));

    get_world()->rail_network().draw_segment(renderer,
        clothoid::segment::from_line(vec2(300, 0), -normalize(vec2(2, 1)), 25));
    get_world()->rail_network().draw_segment(renderer,
        clothoid::segment::from_line(cursor, normalize(vec2(-1, 1)), 25));
#elif 0
    auto connection = extend_linear_segment(
        vec2(300, 0), normalize(vec2(2, 1)), cursor);

    get_world()->rail_network().draw_segment(renderer,
        clothoid::segment::from_line(vec2(300, 0), -normalize(vec2(2, 1)), 25));
#else
    auto connection = extend_curved_segment(
        vec2(500, 100), normalize(vec2(2, 1)), 1.f / 200.f, cursor);

    get_world()->rail_network().draw_segment(renderer,
        clothoid::segment::from_arc(vec2(500, 100), -normalize(vec2(2, 1)), 50, -1.f / 200.f));
#endif
    for (auto& s : connection) {
        get_world()->rail_network().draw_segment(renderer, s);
    }
#endif

#if 1
    {
        clothoid::segment l1 = clothoid::segment::from_arc(vec2(0, -800), vec2(0, -1), 400.f, 1.f / 400.f);
        clothoid::segment l2 = clothoid::segment::from_arc(vec2(300, -800), vec2(-1, 0), 400.f, 1.f / 400.f);
        clothoid::segment l3 = clothoid::segment::from_transition(vec2(200, -800), vec2(0, -1), 400.f, -1.f / 400.f, 0.f);
        get_world()->rail_network().draw_segment(renderer, l1);
        get_world()->rail_network().draw_segment(renderer, l2);
        get_world()->rail_network().draw_segment(renderer, l3);

        float s1, s2;
        do {
            float r1 = 1.f / l1.initial_curvature();
            float r2 = 1.f / l2.initial_curvature();
            vec2 c1 = l1.initial_position() - l1.initial_tangent().cross(r1);
            vec2 c2 = l2.initial_position() - l2.initial_tangent().cross(r2);
            vec2 dr = c2 - c1;
            float d = dr.length();
            if (d > abs(r1) + abs(r2)) {
                break;//return false;
            }
            float x = (d * d - r2 * r2 + r1 * r1) / (2.f * d);
            float y = sqrt(max(0.f, r1 * r1 - x * x));

            vec2 vx = dr.normalize();
            vec2 vy = vx.cross(1.f);

            vec2 p0 = c1 + vx * x + vy * y;
            s1 = atan2f(dot(p0 - c1, l1.initial_tangent() * r1),
                        dot(p0 - c1, l1.initial_tangent().cross(r1))) * r1;
            s2 = atan2f(dot(p0 - c2, l2.initial_tangent() * r2),
                        dot(p0 - c2, l2.initial_tangent().cross(r2))) * r2;

            if (s1 >= 0 && s1 <= l1.length() && s2 >= 0 && s2 <= l2.length()) {
                renderer->draw_box(vec2(4), p0, color4(0,1,0,1));
                //renderer->draw_box(vec2(4), l1.evaluate(s1), color4(1,0,0,1));
                //renderer->draw_box(vec2(4), l2.evaluate(s2), color4(1,1,0,1));
            } else {
                renderer->draw_box(vec2(4), p0, color4(1,0,0,1));
            }

            if (y > 1e-6f) {
                vec2 p1 = c1 + vx * x - vy * y;
                s1 = atan2f(dot(p1 - c1, l1.initial_tangent() * r1),
                            dot(p1 - c1, l1.initial_tangent().cross(r1))) * r1;
                s2 = atan2f(dot(p1 - c2, l2.initial_tangent() * r2),
                            dot(p1 - c2, l2.initial_tangent().cross(r2))) * r2;

                if (s1 >= 0 && s1 <= l1.length() && s2 >= 0 && s2 <= l2.length()) {
                    renderer->draw_box(vec2(4), p1, color4(0,1,0,1));
                    //renderer->draw_box(vec2(4), l1.evaluate(s1), color4(0,1,0,1));
                    //renderer->draw_box(vec2(4), l2.evaluate(s2), color4(0,1,1,1));
                } else {
                    renderer->draw_box(vec2(4), p1, color4(1,0,0,1));
                }
            }
        } while (false);

        if (l1.intersect(l2, s1, s2)) {
            vec2 p1 = l1.evaluate(s1);
            renderer->draw_box(vec2(4), p1, color4(1,1,1,1));
        }

        if (l1.intersect(l3, s1, s2)) {
            vec2 p1 = l1.evaluate(s1);
            renderer->draw_box(vec2(4), p1, color4(1,1,1,1));
        }

        if (l2.intersect(l3, s1, s2)) {
            vec2 p1 = l2.evaluate(s1);
            renderer->draw_box(vec2(4), p1, color4(1,1,1,1));
        }
    }

#endif

#if 1 // parallel transition curve experiments
    {
        clothoid::segment l1 = clothoid::segment::from_transition(vec2(-400, -800), vec2(0, -1), 400.f, -1.f / 400.f, -1.f / 200.f);
        get_world()->rail_network().draw_segment(renderer, l1);

        float k0 = 1.f / ((1.f / l1.initial_curvature() + 5.f));
        float k1 = 1.f / ((1.f / l1.final_curvature() + 5.f));
        float kmid = .5f * (k0 + k1);// * (1.f + .1f * cos(_usercmd_time.to_seconds()));
        //float kmid = 1.f / (.5f * (1.f / k0 + 1.f / k1));

        clothoid::segment l2 = clothoid::segment::from_transition(vec2(-405, -800), l1.initial_tangent(), 400.f, k0, k1);
        get_world()->rail_network().draw_segment(renderer, l2);

        float len = 200.f;
        clothoid::segment l3 = clothoid::segment::from_transition(vec2(-405, -800), l1.initial_tangent(), len, k0, kmid);
        clothoid::segment l4 = clothoid::segment::from_transition(l3.final_position(), l3.final_tangent(), len, kmid, k1);

        //get_world()->rail_network().draw_segment(renderer, l3);
        //get_world()->rail_network().draw_segment(renderer, l4);

        vec2 t1 = l1.final_tangent();
        vec2 p1 = l1.final_position() + t1.cross(5);
        renderer->draw_box(vec2(2), p1, color4(1,1,1,1));

        for (int ii = 0; ii < 64; ++ii) {
            vec2 p2 = l4.final_position();
            kmid *= exp(-1e-2f * cross(p2 - p1, t1));
            len -= dot(p2 - p1, t1);

            l3 = clothoid::segment::from_transition(vec2(-405, -800), l1.initial_tangent(), len, k0, kmid);
            l4 = clothoid::segment::from_transition(l3.final_position(), l3.final_tangent(), len, kmid, k1);
        }

        get_world()->rail_network().draw_segment(renderer, l3);
        get_world()->rail_network().draw_segment(renderer, l4);

        renderer->draw_line(l1.final_position(), l1.final_position() - l1.final_tangent().cross(1.f / l1.final_curvature()), color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_line(l4.final_position(), l4.final_position() - l4.final_tangent().cross(1.f / l4.final_curvature()), color4(1,1,0,1), color4(1,1,0,1));

        clothoid::segment l5 = clothoid::segment::from_arc(l1.final_position(), l1.final_tangent(), abs(1.f / l1.final_curvature()), l1.final_curvature());
        clothoid::segment l6 = clothoid::segment::from_arc(l4.final_position(), l4.final_tangent(), abs(1.f / l4.final_curvature()), l4.final_curvature());

        get_world()->rail_network().draw_segment(renderer, l5);
        get_world()->rail_network().draw_segment(renderer, l6);

        renderer->draw_line(l5.final_position(), l5.final_position() - l5.final_tangent().cross(1.f / l5.final_curvature()), color4(1,1,1,1), color4(1,1,1,1));
        renderer->draw_line(l6.final_position(), l6.final_position() - l6.final_tangent().cross(1.f / l6.final_curvature()), color4(1,1,0,1), color4(1,1,0,1));

        auto draw_radius = [](render::system* renderer, clothoid::segment const& s, color4 c) {
            vec2 p0 = s.initial_position() - s.initial_tangent().cross(1.f / s.initial_curvature());
            for (int ii = 1; ii <= 128; ++ii) {
                float t = s.length() * ii / 128.f;
                float k1 = s.evaluate_curvature(t);
                vec2 t1 = s.evaluate_tangent(t);
                vec2 p1 = s.evaluate(t) - t1.cross(1.f / k1);

                renderer->draw_line(p0, p1, c, c);
                p0 = p1;
            }
        };

        draw_radius(renderer, l1, color4(1,1,1,1));
        draw_radius(renderer, l2, color4(0,1,1,1));
        draw_radius(renderer, l3, color4(1,1,0,1));
        draw_radius(renderer, l4, color4(1,1,0,1));
    }
#endif

    if (_follow) {
        //_follow->as_type<train>()->draw_debug(renderer, time);
        //_follow->as_type<train>()->draw_path(renderer, time);
    }

    constexpr time_delta fade_time = time_delta::from_seconds(1.5f);
    // FIXME: using _usercmd_time as proxy for realtime
    if (_usercmd_time - _timescale_time < fade_time) {
        string::view str = "";
        if (get_world()->timescale() == 0.f) {
            str = "||";
        } else if (get_world()->timescale() == 1.f) {
            str = ">";
        } else if (get_world()->timescale() == 3.f) {
            str = ">>";
        } else if (get_world()->timescale() == 9.f) {
            str = ">>>";
        }

        float t = 1.f - (_usercmd_time - _timescale_time) / fade_time;

        // Text orientation is fixed in worldspace, create a new view with no
        // rotation so that the text always appears in the top right corner.
        render::view old_view = renderer->view();
        render::view view = old_view;
        view.angle = 0.f;
        renderer->set_view(view);

        vec2 offset = renderer->string_size(">>>");
        vec2 size = renderer->string_size(str);
        renderer->draw_string(
            str,
            view.origin - offset + .5f * (view.size - size),
            color4(.9f * t * t, 1, 1, t));

        renderer->set_view(old_view);
    }
}

//------------------------------------------------------------------------------
void player::think()
{
}

//------------------------------------------------------------------------------
vec2 player::get_position(time_value time) const
{
    (void)time;
    return vec2_zero;
}

//------------------------------------------------------------------------------
float player::get_rotation(time_value time) const
{
    (void)time;
    return 0.f;
}

//------------------------------------------------------------------------------
mat3 player::get_transform(time_value time) const
{
    (void)time;
    return mat3_identity;
}

//------------------------------------------------------------------------------
player_view player::view(time_value time, time_value realtime) const
{
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
    constexpr float scroll_speed = 1.f;

    player_view view = _view;

    if (_follow) {
        view.origin = _follow->get_position(time);
        view.angle = _follow->get_rotation(time);
    }

    if (realtime > _usercmd_time) {
        float delta_time = (realtime - _usercmd_time).to_seconds();

        if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
            view.origin.y += scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
            view.origin.y -= scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
            view.origin.x -= scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
            view.origin.x += scroll_speed * _view.size.x * delta_time;
        }
        if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
            view.size *= exp(-zoom_speed * delta_time);
        }
        if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
            view.size *= exp(zoom_speed * delta_time);
        }
    }

    return view;
}

//------------------------------------------------------------------------------
void player::set_aspect(float aspect)
{
    _view.size.x = _view.size.y * aspect;
}

//------------------------------------------------------------------------------
void player::update_usercmd(usercmd cmd, time_value realtime)
{
    constexpr float zoom_speed = 1.f + (1.f / 32.f);
    constexpr float scroll_speed = 1.f;

    float delta_time = (realtime - _usercmd_time).to_seconds();

    if (!!(_usercmd.buttons & usercmd::button::scroll_up)) {
        _view.origin.y += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_down)) {
        _view.origin.y -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_left)) {
        _view.origin.x -= scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::scroll_right)) {
        _view.origin.x += scroll_speed * _view.size.x * delta_time;
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_in)) {
        _view.size *= exp(-zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::zoom_out)) {
        _view.size *= exp(zoom_speed * delta_time);
    }
    if (!!(_usercmd.buttons & usercmd::button::pan)) {
        _view.origin -= (cmd.cursor - _usercmd.cursor) * _view.size;
    }

    _usercmd = cmd;
    _usercmd_time = realtime;

    if (_usercmd.action == usercmd::action::zoom_in) {
        _view.size *= (1.f / zoom_speed);
    } else if (_usercmd.action == usercmd::action::zoom_out) {
        _view.size *= zoom_speed;
    } else if (_usercmd.action == usercmd::action::follow) {
        on_follow();
    } else if (_usercmd.action == usercmd::action::speed_up) {
        get_world()->on_speed_up();
        _timescale_time = realtime;
    } else if (_usercmd.action == usercmd::action::speed_down) {
        get_world()->on_speed_down();
        _timescale_time = realtime;
    } else if (_usercmd.action == usercmd::action::pause) {
        get_world()->on_pause();
        _timescale_time = realtime;
    }
}

//------------------------------------------------------------------------------
void player::on_follow()
{
    handle<object> prev = nullptr;

    // cycle through all train objects
    for (auto obj : get_world()->objects()) {
        if (obj->is_type<train>()) {
            if (prev == _follow) {
                _follow = obj;
                break;
            }
            prev = obj;
        }
    }

    // stop following if no more trains
    if (prev == _follow) {
        //_view = view(_usercmd_time);
        _view.angle = 0.f;
        _follow = nullptr;
    }
}

} // namespace game

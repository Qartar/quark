// cm_ballistics.cpp
//

#include "cm_ballistics.h"
#include "cm_shared.h"
#include "cm_string.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////
namespace ballistics {

//------------------------------------------------------------------------------
struct ballistic_data
{
    double initial_velocity;
    double initial_angle;
    double range;
    double time;
    double impact_angle;
    double impact_velocity;
};

//------------------------------------------------------------------------------
struct ballistic_dataset
{
    string::literal name;
    float caliber;
    float mass;
    std::vector<ballistic_data> data;
};

//------------------------------------------------------------------------------
void simulate_ballistic_coefficient(ballistic_data& data, ballistics::curve curve, time_delta dt, double bc)
{
    vec3 r = vec3_zero;
    vec3 v = vec3(cos(math::deg2rad(data.initial_angle)),
        0,
        sin(math::deg2rad(data.initial_angle))) * data.initial_velocity;

    data.time = ballistics::simulate(r, v, curve, bc, dt).to_seconds();
    data.range = r.x;
    data.impact_angle = math::rad2deg(atan2(-v.z, v.x));
    data.impact_velocity = length(v);
}

//------------------------------------------------------------------------------
double compare_ballistic_data(ballistic_data const& b1, ballistic_data const& b2)
{
    double num = 0.f;
    double den = 0.f;

    if (b1.range && b2.range) {
        num += square(b1.range - b2.range) / (b1.range * b2.range);
        den += 1.f;
    }

    if (b1.time && b2.time) {
        num += square(b1.time - b2.time) / (b1.time * b2.time);
        den += 1.f;
    }

    if (b1.impact_angle && b2.impact_angle) {
        num += square(b1.impact_angle - b2.impact_angle) / (b1.impact_angle * b2.impact_angle);
        den += 1.f;
    }

    if (b1.impact_velocity && b2.impact_velocity) {
        num += square(b1.impact_velocity - b2.impact_velocity) / (b1.impact_velocity * b2.impact_velocity);
        den += 1.f;
    }

    return den ? num / den : 0.f;
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient(ballistic_data const& b, ballistics::curve c, time_delta dt, double& bc, double& rms)
{
    ballistic_data d;

    bc = 1e3;
    rms = 0.f;
    double den = 0.f;

    if (b.range) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.range / d.range;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.time) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.time / d.time;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.impact_angle) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= d.impact_angle / b.impact_angle;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (b.impact_velocity) {
        for (std::size_t ii = 0; ii < 64; ++ii) {
            d = b;
            simulate_ballistic_coefficient(d, c, dt, bc);
            bc *= b.impact_velocity / d.impact_velocity;
        }
        rms += compare_ballistic_data(b, d);
        den += 1.f;
    }

    if (den) {
        rms /= den;
    }
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient(ballistic_data const* b, std::size_t n, ballistics::curve c, time_delta dt, double& bc, double& rms)
{
    rms = 0.f;
    for (std::size_t ii = 0; ii < n; ++ii) {
        double tmp = 0.f;
        solve_ballistic_coefficient(b[ii], c, dt, bc, tmp);
        rms += tmp;
    }
    rms /= n;
}

//------------------------------------------------------------------------------
void solve_ballistic_coefficient_cmd(parser::text const& /*args*/)
{
    ballistic_dataset const sets[] = {
        {
            "46 cm/45 Type 94 (Yamato)",
            .46f,
            1460.f,
            {
                { 780.f,  2.4f,  5000.f, 0.f,  3.3f, 690.f },
                { 780.f,  5.4f, 10000.f, 0.f,  7.2f, 620.f },
                { 780.f,  8.6f, 15000.f, 0.f, 11.5f, 562.f },
                { 780.f, 12.6f, 20000.f, 0.f, 16.5f, 521.f },
                { 780.f, 17.2f, 25000.f, 0.f, 23.0f, 490.f },
                { 780.f, 23.2f, 30000.f, 0.f, 31.4f, 475.f },
            }
        },
        {
            "16-inch/50 Mark 7 (Iowa)",
            .406f,
            1225.f,
            {
                { 762.f,  2.36f,  4572.f,  6.29f,  2.50f, 695.f },
                { 762.f,  5.05f,  9140.f, 13.24f,  5.01f, 632.f },
                { 762.f,  8.16f, 13716.f, 20.98f,  9.78f, 577.f },
                { 762.f, 11.77f, 18290.f, 29.59f, 14.92f, 530.f },
                { 762.f, 16.03f, 22860.f, 39.30f, 21.12f, 497.f },
                { 762.f, 21.11f, 27430.f, 50.32f, 28.25f, 478.f },
                { 762.f, 27.37f, 32000.f, 63.22f, 36.27f, 474.f },
                { 762.f, 36.08f, 36580.f, 79.92f, 47.73f, 490.f },
                { 762.f, 45.08f, 38720.f, 95.32f, 51.23f, 514.f },
            }
        },
        {
            "16-inch/45 Mark 6 (North Carolina)",
            .406f,
            1225.f,
            {
                { 701.f,  2.79f,  4572.f,  6.83f,  2.93f, 637.f },
                { 701.f,  5.99f,  9140.f, 14.45f,  6.80f, 579.f },
                { 701.f,  9.73f, 13716.f, 22.94f, 11.72f, 528.f },
                { 701.f, 14.16f, 18290.f, 32.55f, 17.93f, 489.f },
                { 701.f, 19.50f, 22860.f, 43.61f, 25.38f, 463.f },
                { 701.f, 26.23f, 27430.f, 56.64f, 34.07f, 454.f },
                { 701.f, 36.18f, 32000.f, 74.42f, 44.88f, 466.f },
            }
        },
        {
            "38 cm SK C/34 (Bismarck)",
            .38f,
            800.f,
            {
                { 820.f,  2.2f,  5000.f,  6.5f,  2.4f, 727.f },
                { 820.f,  4.9f, 10000.f, 13.9f,  5.8f, 641.f },
                { 820.f,  8.1f, 15000.f, 22.3f, 10.4f, 568.f },
                { 820.f, 12.1f, 20000.f, 32.0f, 16.4f, 511.f },
                { 820.f, 16.8f, 25000.f, 43.0f, 23.8f, 473.f },
                { 820.f, 22.4f, 30000.f, 55.5f, 31.9f, 457.f },
                { 820.f, 29.1f, 35000.f, 69.9f, 40.3f, 462.f },
            }
        },
        {
            "380 mm/45 Model 1935 (Richelieu)",
            .38f,
            884.f,
            {
                { 830.f,  4.4f, 10000.f, 0.f,  5.2f, 675.f },
                { 830.f,  7.4f, 15000.f, 0.f,  9.0f, 608.f },
                { 830.f, 10.9f, 20000.f, 0.f, 14.0f, 544.f },
                { 830.f, 14.9f, 25000.f, 0.f, 20.2f, 514.f },
                { 830.f, 19.8f, 30000.f, 0.f, 27.2f, 490.f },
                { 830.f, 25.5f, 35000.f, 0.f, 40.3f, 479.f },
            }
        },
        {
            "381 mm/50 Model 1934 (Littorio)",
            .381f,
            884.8f,
            {
                { 850.f,  4.3f, 10000.f, 0.f,  5.0f, 687.f },
                { 850.f,  7.2f, 15000.f, 0.f,  8.7f, 620.f },
                { 850.f, 10.6f, 20000.f, 0.f, 13.4f, 563.f },
                { 850.f, 14.5f, 25000.f, 0.f, 19.3f, 524.f },
                { 850.f, 19.2f, 30000.f, 0.f, 26.1f, 498.f },
                { 850.f, 24.7f, 35000.f, 0.f, 37.6f, 483.f },
            }
        },
        {
            "36 cm 41st Year Type (Fuso)",
            .356f,
            635.f,
            {
                { 775.f,  2.6f,  5000.f, 0.f,  3.5f, 670.f },
                { 775.f,  6.1f, 10000.f, 0.f,  7.5f, 576.f },
                { 775.f,  9.9f, 15000.f, 0.f, 12.4f, 510.f },
                { 775.f, 14.4f, 20000.f, 0.f, 18.9f, 459.f },
                { 775.f, 20.0f, 25000.f, 0.f, 27.5f, 420.f },
                { 775.f, 28.7f, 30000.f, 0.f, 35.9f, 414.f },
            }
        },
        {
            "14-inch BL Mark VII (King George V)",
            .3556f,
            721.2f,
            {
                { 732.f,  2.5f,   4570.f,  6.59f,  2.8f, 658.f },
                { 732.f,  5.5f,   9140.f, 14.06f,  6.5f, 587.f },
                { 732.f,  9.25f, 13720.f, 22.57f, 11.5f, 526.f },
                { 732.f, 13.75f, 18290.f, 32.41f, 18.2f, 476.f },
                { 732.f, 19.25f, 22400.f, 43.86f, 26.4f, 445.f },
                { 732.f, 26.2f,  27430.f, 57.43f, 35.6f, 436.f },
                { 732.f, 36.0f,  32000.f, 74.97f, 46.1f, 452.f },
                { 732.f, 40.7f,  33380.f, 82.42f, 50.3f, 464.f },
            }
        },
        {
            "28 cm/52 SK C/28 (Deutschland)",
            .283f,
            300.f,
            {
                { 910.f,  1.9f,  5000.f, 0.f,  2.4f, 752.f },
                { 910.f,  4.5f, 10000.f, 0.f,  6.0f, 611.f },
                { 910.f,  8.0f, 15000.f, 0.f, 11.8f, 493.f },
                { 910.f, 12.5f, 20000.f, 0.f, 21.4f, 407.f },
                { 910.f, 18.6f, 25000.f, 0.f, 34.2f, 360.f },
                { 910.f, 26.3f, 30000.f, 0.f, 46.4f, 353.f },
                { 910.f, 36.4f, 35000.f, 0.f, 56.0f, 380.f },
            }
        },
        {
            "15.5 cm/60 Type 3rd Year Type (Yamato)",
            .155f,
            55.87f,
            {
                { 925.f,  2.3f,  5000.f, 0.f,  3.0f, 663.f },
                { 925.f,  5.4f, 10000.f, 0.f,  7.7f, 465.f },
                { 925.f, 11.2f, 15000.f, 0.f, 16.5f, 340.f },
                { 925.f, 20.0f, 20000.f, 0.f, 33.0f, 320.f },
                { 925.f, 35.0f, 25000.f, 0.f, 56.0f, 350.f },
            }
        },
        {
            "152 mm/55 Model 1934 (Littorio)",
            .152f,
            49.6f,
            {
                { 910.f,  45.f, 25740.f,  0.f,  0.f, 0.f },
            }
        },
        {
            "152 mm/55 Model 1930 (Richelieu)",
            .152f,
            54.5f,
            {
                { 870.f,  45.f, 26474.f,  0.f,  0.f, 0.f },
            }
        },
        {
            "15 cm/55 SK C/28 (Bismarck)",
            .15f,
            45.3f,
            {
                { 875.f,  35.f, 22000.f,  0.f,  0.f, 0.f },
                { 875.f,  40.f, 23000.f,  0.f,  0.f, 0.f },
            }
        },
        {
            "QF 5.25-inch Mark 1 (King George V)",
            .133f,
            36.3f,
            {
                { 814.f,  45.f, 21397.f,  0.f,  0.f, 0.f },
            }
        },
        {
            "5-inch/38 Mark 12 (North Carolina)",
            .127f,
            25.f,
            {
                { 792.f,  0.44f,  1829.f,  2.55f,  0.93f, 653.f },
                { 792.f,  2.18f,  3658.f,  5.71f,  2.85f, 526.f },
                { 792.f,  3.89f,  5486.f,  9.67f,  5.97f, 418.f },
                { 792.f,  6.34f,  7315.f, 14.73f, 11.10f, 344.f },
                { 792.f,  9.79f,  9114.f, 20.81f, 18.33f, 306.f },
                { 792.f, 14.33f, 10972.f, 27.78f, 26.63f, 290.f },
                { 792.f, 20.06f, 12802.f, 35.81f, 35.38f, 287.f },
                { 792.f, 27.47f, 14630.f, 45.44f, 44.60f, 294.f },
                { 792.f, 43.30f, 16642.f, 64.40f, 59.37f, 314.f },
                { 792.f, 64.23f, 12802.f, 84.20f, 73.40f, 352.f },
            }
        },
        {
            "6-inch/50 BL Mark XXIII (Town)",
            .152f,
            45.36f,
            {
                { 823.f,  2.3f,  4570.f,  6.6f,  3.0f, 591.f },
                { 823.f,  6.2f,  9140.f, 15.9f, 10.0f, 418.f },
                { 823.f, 13.1f, 13720.f, 29.4f, 23.6f, 335.f },
                { 823.f, 24.1f, 18290.f, 47.2f, 39.9f, 331.f },
                { 823.f, 41.1f, 22400.f, 71.4f, 56.5f, 353.f },
            }
        },
        {
            "QF 4.7-inch Mk IX (Tribal)",
            .12f,
            22.68f,
            {
                { 808.f,  30.f, 14450.f,  0.f,  0.f, 0.f },
                { 808.f,  40.f, 15545.f,  0.f,  0.f, 0.f },
            }
        },
    };

    const char* curves[] = { "G1", "G2", "G5", "G6", "G7", "G8" };

    for (auto const& s : sets) {
        log::message("%s\n", s.name.c_str());

        double best_bc = 0.f, best_rms = DBL_MAX;
        ballistics::curve best_curve = ballistics::curve::G1;

        double bc[6];
        double rms[6];

        for (std::size_t kk = 0; kk < 6; ++kk) {
            ballistics::curve c = static_cast<ballistics::curve>(static_cast<std::size_t>(ballistics::curve::G1) + kk);
            solve_ballistic_coefficient(s.data.data(), s.data.size(), c, time_delta::from_hertz(20.f), bc[kk], rms[kk]);
            if (rms[kk] < best_rms) {
                best_bc = bc[kk];
                best_rms = rms[kk];
                best_curve = c;
            }
        }

        for (std::size_t kk = 0; kk < 6; ++kk) {
            ballistics::curve c = static_cast<ballistics::curve>(static_cast<std::size_t>(ballistics::curve::G1) + kk);
            if (c == best_curve) {
                log::message("^fff %s %.16f %.16f %.16f^xxx\n",
                    curves[static_cast<std::size_t>(c)],
                    rms[kk], bc[kk], bc[kk] / s.caliber);
            } else {
                log::message(" %s %.16f %.16f %.16f\n",
                    curves[static_cast<std::size_t>(c)],
                    rms[kk], bc[kk], bc[kk] / s.caliber);
            }
        }

        for (std::size_t kk = 0; kk < s.data.size(); ++kk) {
            ballistic_data d = s.data[kk];
            simulate_ballistic_coefficient(d, best_curve, time_delta::from_hertz(20.f), best_bc);

            log::message(" %zu %.16f %.1fs (%g vs %g m) (%g vs %g m/s) (%2.1f vs %2.1f)\n", kk, best_bc,
                d.time,
                d.range, s.data[kk].range,
                d.impact_velocity, s.data[kk].impact_velocity,
                d.impact_angle, s.data[kk].impact_angle);
        }
        log::message("\n");
    }
}

//------------------------------------------------------------------------------
// https://jbmballistics.com/ballistics/downloads/downloads.shtml
// Believe these are in ft/lbs but a change in units just results in a constant
// scaling factor for the ballistic coefficient. Note that not all tables are the
// same length and so are padded with zeroes at the end.
constexpr float drag_tables[][170] = {
    // G1
    {
        0.00f,  0.2629f,
        0.05f,  0.2558f,
        0.10f,  0.2487f,
        0.15f,  0.2413f,
        0.20f,  0.2344f,
        0.25f,  0.2278f,
        0.30f,  0.2214f,
        0.35f,  0.2155f,
        0.40f,  0.2104f,
        0.45f,  0.2061f,
        0.50f,  0.2032f,
        0.55f,  0.2020f,
        0.60f,  0.2034f,
        0.70f,  0.2165f,
        0.725f, 0.2230f,
        0.75f,  0.2313f,
        0.775f, 0.2417f,
        0.80f,  0.2546f,
        0.825f, 0.2706f,
        0.85f,  0.2901f,
        0.875f, 0.3136f,
        0.90f,  0.3415f,
        0.925f, 0.3734f,
        0.95f,  0.4084f,
        0.975f, 0.4448f,
        1.0f,   0.4805f,
        1.025f, 0.5136f,
        1.05f,  0.5427f,
        1.075f, 0.5677f,
        1.10f,  0.5883f,
        1.125f, 0.6053f,
        1.15f,  0.6191f,
        1.20f,  0.6393f,
        1.25f,  0.6518f,
        1.30f,  0.6589f,
        1.35f,  0.6621f,
        1.40f,  0.6625f,
        1.45f,  0.6607f,
        1.50f,  0.6573f,
        1.55f,  0.6528f,
        1.60f,  0.6474f,
        1.65f,  0.6413f,
        1.70f,  0.6347f,
        1.75f,  0.6280f,
        1.80f,  0.6210f,
        1.85f,  0.6141f,
        1.90f,  0.6072f,
        1.95f,  0.6003f,
        2.00f,  0.5934f,
        2.05f,  0.5867f,
        2.10f,  0.5804f,
        2.15f,  0.5743f,
        2.20f,  0.5685f,
        2.25f,  0.5630f,
        2.30f,  0.5577f,
        2.35f,  0.5527f,
        2.40f,  0.5481f,
        2.45f,  0.5438f,
        2.50f,  0.5397f,
        2.60f,  0.5325f,
        2.70f,  0.5264f,
        2.80f,  0.5211f,
        2.90f,  0.5168f,
        3.00f,  0.5133f,
        3.10f,  0.5105f,
        3.20f,  0.5084f,
        3.30f,  0.5067f,
        3.40f,  0.5054f,
        3.50f,  0.5040f,
        3.60f,  0.5030f,
        3.70f,  0.5022f,
        3.80f,  0.5016f,
        3.90f,  0.5010f,
        4.00f,  0.5006f,
        4.20f,  0.4998f,
        4.40f,  0.4995f,
        4.60f,  0.4992f,
        4.80f,  0.4990f,
        5.00f,  0.4988f,
    },
    // G2
    {
        0.00f,  0.2303f,
        0.05f,  0.2298f,
        0.10f,  0.2287f,
        0.15f,  0.2271f,
        0.20f,  0.2251f,
        0.25f,  0.2227f,
        0.30f,  0.2196f,
        0.35f,  0.2156f,
        0.40f,  0.2107f,
        0.45f,  0.2048f,
        0.50f,  0.1980f,
        0.55f,  0.1905f,
        0.60f,  0.1828f,
        0.65f,  0.1758f,
        0.70f,  0.1702f,
        0.75f,  0.1669f,
        0.775f, 0.1664f,
        0.80f,  0.1667f,
        0.825f, 0.1682f,
        0.85f,  0.1711f,
        0.875f, 0.1761f,
        0.90f,  0.1831f,
        0.925f, 0.2004f,
        0.95f,  0.2589f,
        0.975f, 0.3492f,
        1.0f,   0.3983f,
        1.025f, 0.4075f,
        1.05f,  0.4103f,
        1.075f, 0.4114f,
        1.10f,  0.4106f,
        1.125f, 0.4089f,
        1.15f,  0.4068f,
        1.175f, 0.4046f,
        1.20f,  0.4021f,
        1.25f,  0.3966f,
        1.30f,  0.3904f,
        1.35f,  0.3835f,
        1.40f,  0.3759f,
        1.45f,  0.3678f,
        1.50f,  0.3594f,
        1.55f,  0.3512f,
        1.60f,  0.3432f,
        1.65f,  0.3356f,
        1.70f,  0.3282f,
        1.75f,  0.3213f,
        1.80f,  0.3149f,
        1.85f,  0.3089f,
        1.90f,  0.3033f,
        1.95f,  0.2982f,
        2.00f,  0.2933f,
        2.05f,  0.2889f,
        2.10f,  0.2846f,
        2.15f,  0.2806f,
        2.20f,  0.2768f,
        2.25f,  0.2731f,
        2.30f,  0.2696f,
        2.35f,  0.2663f,
        2.40f,  0.2632f,
        2.45f,  0.2602f,
        2.50f,  0.2572f,
        2.55f,  0.2543f,
        2.60f,  0.2515f,
        2.65f,  0.2487f,
        2.70f,  0.2460f,
        2.75f,  0.2433f,
        2.80f,  0.2408f,
        2.85f,  0.2382f,
        2.90f,  0.2357f,
        2.95f,  0.2333f,
        3.00f,  0.2309f,
        3.10f,  0.2262f,
        3.20f,  0.2217f,
        3.30f,  0.2173f,
        3.40f,  0.2132f,
        3.50f,  0.2091f,
        3.60f,  0.2052f,
        3.70f,  0.2014f,
        3.80f,  0.1978f,
        3.90f,  0.1944f,
        4.00f,  0.1912f,
        4.20f,  0.1851f,
        4.40f,  0.1794f,
        4.60f,  0.1741f,
        4.80f,  0.1693f,
        5.00f,  0.1648f,
    },
    // G5
    {
        0.00f,  0.1710f,
        0.05f,  0.1719f,
        0.10f,  0.1727f,
        0.15f,  0.1732f,
        0.20f,  0.1734f,
        0.25f,  0.1730f,
        0.30f,  0.1718f,
        0.35f,  0.1696f,
        0.40f,  0.1668f,
        0.45f,  0.1637f,
        0.50f,  0.1603f,
        0.55f,  0.1566f,
        0.60f,  0.1529f,
        0.65f,  0.1497f,
        0.70f,  0.1473f,
        0.75f,  0.1463f,
        0.80f,  0.1489f,
        0.85f,  0.1583f,
        0.875f, 0.1672f,
        0.90f,  0.1815f,
        0.925f, 0.2051f,
        0.95f,  0.2413f,
        0.975f, 0.2884f,
        1.0f,   0.3379f,
        1.025f, 0.3785f,
        1.05f,  0.4032f,
        1.075f, 0.4147f,
        1.10f,  0.4201f,
        1.15f,  0.4278f,
        1.20f,  0.4338f,
        1.25f,  0.4373f,
        1.30f,  0.4392f,
        1.35f,  0.4403f,
        1.40f,  0.4406f,
        1.45f,  0.4401f,
        1.50f,  0.4386f,
        1.55f,  0.4362f,
        1.60f,  0.4328f,
        1.65f,  0.4286f,
        1.70f,  0.4237f,
        1.75f,  0.4182f,
        1.80f,  0.4121f,
        1.85f,  0.4057f,
        1.90f,  0.3991f,
        1.95f,  0.3926f,
        2.00f,  0.3861f,
        2.05f,  0.3800f,
        2.10f,  0.3741f,
        2.15f,  0.3684f,
        2.20f,  0.3630f,
        2.25f,  0.3578f,
        2.30f,  0.3529f,
        2.35f,  0.3481f,
        2.40f,  0.3435f,
        2.45f,  0.3391f,
        2.50f,  0.3349f,
        2.60f,  0.3269f,
        2.70f,  0.3194f,
        2.80f,  0.3125f,
        2.90f,  0.3060f,
        3.00f,  0.2999f,
        3.10f,  0.2942f,
        3.20f,  0.2889f,
        3.30f,  0.2838f,
        3.40f,  0.2790f,
        3.50f,  0.2745f,
        3.60f,  0.2703f,
        3.70f,  0.2662f,
        3.80f,  0.2624f,
        3.90f,  0.2588f,
        4.00f,  0.2553f,
        4.20f,  0.2488f,
        4.40f,  0.2429f,
        4.60f,  0.2376f,
        4.80f,  0.2326f,
        5.00f,  0.2280f,
    },
    // G6
    {
        0.00f,  0.2617f,
        0.05f,  0.2553f,
        0.10f,  0.2491f,
        0.15f,  0.2432f,
        0.20f,  0.2376f,
        0.25f,  0.2324f,
        0.30f,  0.2278f,
        0.35f,  0.2238f,
        0.40f,  0.2205f,
        0.45f,  0.2177f,
        0.50f,  0.2155f,
        0.55f,  0.2138f,
        0.60f,  0.2126f,
        0.65f,  0.2121f,
        0.70f,  0.2122f,
        0.75f,  0.2132f,
        0.80f,  0.2154f,
        0.85f,  0.2194f,
        0.875f, 0.2229f,
        0.90f,  0.2297f,
        0.925f, 0.2449f,
        0.95f,  0.2732f,
        0.975f, 0.3141f,
        1.0f,   0.3597f,
        1.025f, 0.3994f,
        1.05f,  0.4261f,
        1.075f, 0.4402f,
        1.10f,  0.4465f,
        1.125f, 0.4490f,
        1.15f,  0.4497f,
        1.175f, 0.4494f,
        1.20f,  0.4482f,
        1.225f, 0.4464f,
        1.25f,  0.4441f,
        1.30f,  0.4390f,
        1.35f,  0.4336f,
        1.40f,  0.4279f,
        1.45f,  0.4221f,
        1.50f,  0.4162f,
        1.55f,  0.4102f,
        1.60f,  0.4042f,
        1.65f,  0.3981f,
        1.70f,  0.3919f,
        1.75f,  0.3855f,
        1.80f,  0.3788f,
        1.85f,  0.3721f,
        1.90f,  0.3652f,
        1.95f,  0.3583f,
        2.00f,  0.3515f,
        2.05f,  0.3447f,
        2.10f,  0.3381f,
        2.15f,  0.3314f,
        2.20f,  0.3249f,
        2.25f,  0.3185f,
        2.30f,  0.3122f,
        2.35f,  0.3060f,
        2.40f,  0.3000f,
        2.45f,  0.2941f,
        2.50f,  0.2883f,
        2.60f,  0.2772f,
        2.70f,  0.2668f,
        2.80f,  0.2574f,
        2.90f,  0.2487f,
        3.00f,  0.2407f,
        3.10f,  0.2333f,
        3.20f,  0.2265f,
        3.30f,  0.2202f,
        3.40f,  0.2144f,
        3.50f,  0.2089f,
        3.60f,  0.2039f,
        3.70f,  0.1991f,
        3.80f,  0.1947f,
        3.90f,  0.1905f,
        4.00f,  0.1866f,
        4.20f,  0.1794f,
        4.40f,  0.1730f,
        4.60f,  0.1673f,
        4.80f,  0.1621f,
        5.00f,  0.1574f,
    },
    // G7
    {
        0.00f,  0.1198f,
        0.05f,  0.1197f,
        0.10f,  0.1196f,
        0.15f,  0.1194f,
        0.20f,  0.1193f,
        0.25f,  0.1194f,
        0.30f,  0.1194f,
        0.35f,  0.1194f,
        0.40f,  0.1193f,
        0.45f,  0.1193f,
        0.50f,  0.1194f,
        0.55f,  0.1193f,
        0.60f,  0.1194f,
        0.65f,  0.1197f,
        0.70f,  0.1202f,
        0.725f, 0.1207f,
        0.75f,  0.1215f,
        0.775f, 0.1226f,
        0.80f,  0.1242f,
        0.825f, 0.1266f,
        0.85f,  0.1306f,
        0.875f, 0.1368f,
        0.90f,  0.1464f,
        0.925f, 0.1660f,
        0.95f,  0.2054f,
        0.975f, 0.2993f,
        1.0f,   0.3803f,
        1.025f, 0.4015f,
        1.05f,  0.4043f,
        1.075f, 0.4034f,
        1.10f,  0.4014f,
        1.125f, 0.3987f,
        1.15f,  0.3955f,
        1.20f,  0.3884f,
        1.25f,  0.3810f,
        1.30f,  0.3732f,
        1.35f,  0.3657f,
        1.40f,  0.3580f,
        1.50f,  0.3440f,
        1.55f,  0.3376f,
        1.60f,  0.3315f,
        1.65f,  0.3260f,
        1.70f,  0.3209f,
        1.75f,  0.3160f,
        1.80f,  0.3117f,
        1.85f,  0.3078f,
        1.90f,  0.3042f,
        1.95f,  0.3010f,
        2.00f,  0.2980f,
        2.05f,  0.2951f,
        2.10f,  0.2922f,
        2.15f,  0.2892f,
        2.20f,  0.2864f,
        2.25f,  0.2835f,
        2.30f,  0.2807f,
        2.35f,  0.2779f,
        2.40f,  0.2752f,
        2.45f,  0.2725f,
        2.50f,  0.2697f,
        2.55f,  0.2670f,
        2.60f,  0.2643f,
        2.65f,  0.2615f,
        2.70f,  0.2588f,
        2.75f,  0.2561f,
        2.80f,  0.2533f,
        2.85f,  0.2506f,
        2.90f,  0.2479f,
        2.95f,  0.2451f,
        3.00f,  0.2424f,
        3.10f,  0.2368f,
        3.20f,  0.2313f,
        3.30f,  0.2258f,
        3.40f,  0.2205f,
        3.50f,  0.2154f,
        3.60f,  0.2106f,
        3.70f,  0.2060f,
        3.80f,  0.2017f,
        3.90f,  0.1975f,
        4.00f,  0.1935f,
        4.20f,  0.1861f,
        4.40f,  0.1793f,
        4.60f,  0.1730f,
        4.80f,  0.1672f,
        5.00f,  0.1618f,
    },
    // G8
    {
        0.00f,  0.2105f,
        0.05f,  0.2105f,
        0.10f,  0.2104f,
        0.15f,  0.2104f,
        0.20f,  0.2103f,
        0.25f,  0.2103f,
        0.30f,  0.2103f,
        0.35f,  0.2103f,
        0.40f,  0.2103f,
        0.45f,  0.2102f,
        0.50f,  0.2102f,
        0.55f,  0.2102f,
        0.60f,  0.2102f,
        0.65f,  0.2102f,
        0.70f,  0.2103f,
        0.75f,  0.2103f,
        0.80f,  0.2104f,
        0.825f, 0.2104f,
        0.85f,  0.2105f,
        0.875f, 0.2106f,
        0.90f,  0.2109f,
        0.925f, 0.2183f,
        0.95f,  0.2571f,
        0.975f, 0.3358f,
        1.0f,   0.4068f,
        1.025f, 0.4378f,
        1.05f,  0.4476f,
        1.075f, 0.4493f,
        1.10f,  0.4477f,
        1.125f, 0.4450f,
        1.15f,  0.4419f,
        1.20f,  0.4353f,
        1.25f,  0.4283f,
        1.30f,  0.4208f,
        1.35f,  0.4133f,
        1.40f,  0.4059f,
        1.45f,  0.3986f,
        1.50f,  0.3915f,
        1.55f,  0.3845f,
        1.60f,  0.3777f,
        1.65f,  0.3710f,
        1.70f,  0.3645f,
        1.75f,  0.3581f,
        1.80f,  0.3519f,
        1.85f,  0.3458f,
        1.90f,  0.3400f,
        1.95f,  0.3343f,
        2.00f,  0.3288f,
        2.05f,  0.3234f,
        2.10f,  0.3182f,
        2.15f,  0.3131f,
        2.20f,  0.3081f,
        2.25f,  0.3032f,
        2.30f,  0.2983f,
        2.35f,  0.2937f,
        2.40f,  0.2891f,
        2.45f,  0.2845f,
        2.50f,  0.2802f,
        2.60f,  0.2720f,
        2.70f,  0.2642f,
        2.80f,  0.2569f,
        2.90f,  0.2499f,
        3.00f,  0.2432f,
        3.10f,  0.2368f,
        3.20f,  0.2308f,
        3.30f,  0.2251f,
        3.40f,  0.2197f,
        3.50f,  0.2147f,
        3.60f,  0.2101f,
        3.70f,  0.2058f,
        3.80f,  0.2019f,
        3.90f,  0.1983f,
        4.00f,  0.1950f,
        4.20f,  0.1890f,
        4.40f,  0.1837f,
        4.60f,  0.1791f,
        4.80f,  0.1750f,
        5.00f,  0.1713f,
    },
};

//------------------------------------------------------------------------------
template<std::size_t sz> double drag_table_lookup(const float (&table)[sz], double mach_number)
{
    if (mach_number <= table[0]) {
        return table[1];
    }
    for (std::size_t ii = 2; ii < sz; ii += 2) {
        if (table[ii] > mach_number) {
            double t = (table[ii] - mach_number) / (table[ii] - table[ii - 2]);
            return table[ii - 1] * t + table[ii + 1] * (1.f - t);
        }
    }
    assert(false);
    return table[sz - 1];
}

//------------------------------------------------------------------------------
double speed_of_sound(double altitude)
{
    // Speed of sound decreases nearly linearly up to 11km in altitude
    return 343.0 + altitude * ((295.0 - 343.0) / 11000.0);
}

//------------------------------------------------------------------------------
double normalized_atmospheric_density(double altitude)
{
    // Typical scale height of 8500m assumes constant temperature
    return exp(-altitude * (1.0 / 10400.0));
}

//------------------------------------------------------------------------------
void step(vec3& position, vec3& velocity, ballistics::curve curve, double ballistic_coefficient, time_delta dt)
{
    // Gravity varies with altitude by less than a percent at relevant altitudes.
    constexpr vec3 gravity(0, 0, -9.80665);

    double rho = normalized_atmospheric_density(position.z);
    double vlen = length(velocity);
    double mach = vlen / speed_of_sound(position.z);
    double Cd = drag_table_lookup(drag_tables[static_cast<int>(curve)], mach);
    vec3 vsqr = vlen * velocity;
    vec3 acceleration = gravity - 0.5 * Cd * rho * vsqr / ballistic_coefficient;

    position += velocity * dt.to_seconds();
    velocity += acceleration * dt.to_seconds();
}

//------------------------------------------------------------------------------
time_delta simulate(vec3& position, vec3& velocity, ballistics::curve curve, double ballistic_coefficient, time_delta timestep)
{
    time_delta dt = time_delta::zero;

    do {
        dt += timestep;
        step(position, velocity, curve, ballistic_coefficient, timestep);
    } while (position.z > 0.f);

    // backstep to impact
    double t = position.z / velocity.z;
    position -= velocity * t;
    return dt - time_delta::from_seconds(t);
}

} // namespace ballistics

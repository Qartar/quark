// g_system.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_system.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
system::system(random r)
    : _random(r)
    , _bodies(std::move(generate(r)))
{
}

//------------------------------------------------------------------------------
std::vector<system::body> system::generate(random r) const
{
    std::vector<body> bodies;

    // generate stars
    {
        float mass = generate_stellar_mass(r);
        float radius = calculate_stellar_radius(mass);
        bodies.emplace_back(body{invalid_body, 0, {}, mass, radius});
    }

    // generate planets
    {
        // calculate habitable zone
        //  T = sqrt( T_s^2 * R_s / D / 2)
        //  T^2 = T_s^2 * R_s / D / 2
        // D = T_s^2 * R_s / T^2 / 2
        float T_s = calculate_stellar_temperature(bodies[0]);
        float hz_min = .5f * bodies[0].radius * square(T_s) / square(373.f);
        float hz_max = .5f * bodies[0].radius * square(T_s) / square(273.f);

        float r0 = std::sqrt(hz_min * hz_max) / (1.5f * 1.5f * 1.5f);
        float semimajor_axis = r0;

        for (int ii = 0; ii < 8; ++ii) {
            float mass = generate_planetary_mass(r);
            float radius = calculate_planetary_radius(mass);

            float eccentricity = r.uniform_real(0.0f, 0.1f);
            float longitude_at_periapsis = r.uniform_real(2.f * math::pi<float>);
            float mean_anomaly_at_epoch = r.uniform_real(2.f * math::pi<float>);
            time_delta period = calculate_orbital_period(mass + bodies[0].mass, semimajor_axis);

            bodies.emplace_back(body{0, 0, orbit{eccentricity, semimajor_axis, longitude_at_periapsis, mean_anomaly_at_epoch, period}, mass, radius});
            bodies[0].num_descendants++;

            semimajor_axis *= 1.5f;
        }

        semimajor_axis = hz_max * 2.f;
        for (int ii = 0; ii < 256; ++ii) {
            float mass = r.uniform_real(1e10f, 1e15f);
            float radius = calculate_planetary_radius(mass);

            float eccentricity = r.uniform_real(0.01f, 0.07f);
            float longitude_at_periapsis = r.uniform_real(2.f * math::pi<float>);
            float mean_anomaly_at_epoch = r.uniform_real(2.f * math::pi<float>);
            time_delta period = calculate_orbital_period(mass + bodies[0].mass, semimajor_axis);
            float scale = 1.f + r.normal_real(0.05f);

            bodies.emplace_back(body{0, 0, orbit{eccentricity, semimajor_axis * scale, longitude_at_periapsis, mean_anomaly_at_epoch, period}, mass, radius});
            bodies[0].num_descendants++;
        }
    }

    // 
    return bodies;
}

//------------------------------------------------------------------------------
float system::generate_stellar_mass(random& r)
{
    constexpr float max_mass = 10.f;
    constexpr float min_mass = 0.5f;

    // One-part Kroupa generating function (cf. Cambody 2006)
    constexpr float a = 2.35f;
    float Kp = (1.f - a) / (pow(max_mass, 1.f - a) - pow(min_mass, 1.f - a));
    float X = r.uniform_real();

    float M = pow(X * (1.f - a) / Kp + pow(.5f, 1.f - a), 1.f / (1.f - a));
    return M * 1.98892e24f; // convert solar masses to gigagrams
}

//------------------------------------------------------------------------------
float system::generate_planetary_mass(random& r)
{
    // Minimum mass of a brown dwarf is ~13 Jupiter masses, clamp to ~5
    constexpr double max_mass = 5.f * 1.8987e21;
    constexpr double min_mass = 0.1f * 5.97237e18;
#if 0
    // Log-normal distribution centered around the geometric mean of min and max mass
    const double mu_x = std::sqrt(double(min_mass) * double(max_mass));
    const double sigma_x = mu_x - min_mass;
    const double mu = std::log(mu_x * mu_x / (mu_x * mu_x + sigma_x * sigma_x));
    const double sigma = std::sqrt(std::log(1.f + (sigma_x * sigma_x) / (mu_x * mu_x)));

    return float(std::exp(mu + sigma * r.normal_real()));
#else
    return float(r.uniform_real(max_mass - min_mass) + min_mass);
#endif
}

//------------------------------------------------------------------------------
float system::calculate_stellar_radius(float mass)
{
    return 7.756892e-20f * std::pow(mass, 0.78f);
}

//------------------------------------------------------------------------------
float system::calculate_stellar_luminosity(body const& b)
{
    const float Mv = b.mass * (1.f / 1.98892e24f); // Solar masses (from gigagrams)

    if (Mv < 0.43f) {
        return 3.828e17f * 0.2375f * std::pow(Mv, 2.3f);
    } else if (Mv < 2.f) {
        return 3.828e17f * std::pow(Mv, 4.f);
    } else if (Mv < 20.f) {
        return 3.828e17f * 1.415f * std::pow(Mv, 3.5f);
    } else {
        return 3.828e17f * 2550.f * Mv;
    }
}

//------------------------------------------------------------------------------
float system::calculate_stellar_temperature(body const& b)
{
    constexpr float stefan_boltzmann = 5.670374419e-8f;
    float luminosity = calculate_stellar_luminosity(b) * 1e9f; // watts from gigawatts
    float denominator = 4.f * math::pi<float> * square(b.radius * 1e9f) * stefan_boltzmann;
    return std::pow(luminosity / denominator, 0.25f);
}

//------------------------------------------------------------------------------
float system::calculate_planetary_radius(float mass)
{
    return 7.756892e-20f * std::pow(mass, 0.78f);
}

//------------------------------------------------------------------------------
time_delta system::calculate_orbital_period(float mass, float semimajor_axis)
{
    constexpr float gravitational_constant = 6.673848e-32f;
    const float mu = mass * gravitational_constant;
    const float a3 = semimajor_axis * semimajor_axis * semimajor_axis;
    return time_delta::from_seconds(2.0 * math::pi<double>) * std::sqrt(a3 / mu);
}

//------------------------------------------------------------------------------
vec2 system::calculate_orbit(time_value time, std::size_t index) const
{
    assert(index < _bodies.size());

    orbit const& orbit = _bodies[index].orbit;

    // Calculate mean anomaly
    float mean_anomaly = orbit.mean_anomaly_at_epoch + 2.f * math::pi<float> * (time % orbit.period) / orbit.period;

    // Calculate eccentric anomaly
    float eccentric_anomaly = mean_anomaly;
    for (int ii = 0; ii < 4; ++ii) {
        // M = E - e sin E
        // M' = 1 - e cos E
        float sinE = sin(eccentric_anomaly);
        float cosE = cos(eccentric_anomaly);
        eccentric_anomaly -= (eccentric_anomaly - orbit.eccentricity * sinE - mean_anomaly)
                           / (1.f - orbit.eccentricity * cosE);
    }

    // Calculate true anomaly
    float cosE = cos(eccentric_anomaly);
    //float true_anomaly = atan2(1.f - orbit.eccentricity * cosE, cosE - orbit.eccentricity);
    float anomaly_coeff = std::sqrt((1.f + orbit.eccentricity) / (1.f - orbit.eccentricity));
    float true_anomaly = 2.f * atan(anomaly_coeff * tan(.5f * eccentric_anomaly));

    // Calculate position
    float radius = orbit.semimajor_axis * (1.f - orbit.eccentricity * cosE);
    return radius * vec2(cos(true_anomaly + orbit.longitude_of_periapsis), sin(true_anomaly + orbit.longitude_of_periapsis));
}

//------------------------------------------------------------------------------
void system::calculate_positions(time_value time, std::size_t origin, vec2* positions) const
{
    assert(origin < _bodies.size());

    // Set the position of the origin body to the origin and calculate
    // relative positions of all descendants in the orbital hierarchy.
    std::size_t index = origin;
    positions[index] = vec2_zero;
    calculate_positions_subtree(time, index, invalid_body, positions);

    std::size_t parent_index = _bodies[index].parent;
    while (parent_index < _bodies.size()) {
        // Calculate relative position of the parent body to the current body
        // and relative positions of all siblings and their descendants.
        positions[parent_index] = positions[index] - calculate_orbit(time, index);
        calculate_positions_subtree(time, parent_index, index, positions);
        // Move up the hierarchy
        index = parent_index;
        parent_index = _bodies[index].parent;
    }
}

//------------------------------------------------------------------------------
void system::calculate_positions_subtree(time_value time, std::size_t index, std::size_t exclude_index, vec2* positions) const
{
    std::size_t first_index = index + 1;
    std::size_t last_index = index + _bodies[index].num_descendants;
    for (std::size_t ii = first_index; ii <= last_index; ++ii) {
        // Exclude the given body and all of its descendants
        if (ii == exclude_index) {
            ii += _bodies[ii].num_descendants;
            continue;
        }
        // Calculate the position of the current body based on the position of its parent
        positions[ii] = positions[_bodies[ii].parent] + calculate_orbit(time, ii);
    }
}

} // namespace game

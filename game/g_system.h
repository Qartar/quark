// g_system.h
//

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
class system
{
public:

    struct orbit
    {
        float eccentricity;
        float semimajor_axis; //!< semimajor axis in gigameters
        float longitude_of_periapsis; //!< longitude of periapsis in radians
        float mean_anomaly_at_epoch; //!< mean anomaly at epoch in radians
        time_delta period;
    };

    struct body
    {
        std::size_t parent;
        std::size_t num_descendants;
        orbit orbit;
        float mass; //!< mass in gigagrams
        float radius; //!< radius in gigameters
    };

    static constexpr std::size_t invalid_body = SIZE_MAX;

    body const* bodies() const { return _bodies.data(); }
    std::size_t num_bodies() const { return _bodies.size(); }
    void calculate_all_positions(time_value time, vec2* positions) const { calculate_positions(time, 0, positions); }

public:
    system(random r);

protected:
    random _random;
    std::vector<body> _bodies;

protected:
    std::vector<body> generate(random r) const;

    //!< Generate a random stellar mass using an initial mass function in gigagrams
    static float generate_stellar_mass(random& r);
    //!< Generate a random planetary mass in gigagrams
    static float generate_planetary_mass(random& r);

    //!< Calculate the radius of the stellar body in gigameters
    static float calculate_stellar_radius(float mass);
    //!< Calculate the luminosity of the stellar body in gigawatts
    static float calculate_stellar_luminosity(body const& b);
    //!< Calculate the surface temperature of the stellar body in kelvin
    static float calculate_stellar_temperature(body const& b);

    //!< Calculate the radius of the planetary body in gigameters
    static float calculate_planetary_radius(float mass);

    //!< Calculate orbital period
    static time_delta calculate_orbital_period(float mass, float semimajor_axis);

    //! Calculate the relative offset of the given body to its parent in the orbital hierarchy.
    vec2 calculate_orbit(time_value time, std::size_t index) const;
    //! Calculate the positions of all bodies relative to the given origin body.
    void calculate_positions(time_value time, std::size_t origin, vec2* positions) const;
    //! Calculate the positions of all bodies descending from the given body index except for
    //! the excluded body index and its descendants. Assumes positions[index] has been initialized.
    void calculate_positions_subtree(time_value time, std::size_t index, std::size_t exclude_index, vec2* positions) const;
};

} // namespace game

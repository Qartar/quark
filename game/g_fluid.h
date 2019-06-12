// g_fluid.h
//

#pragma once

#include "g_object.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

//------------------------------------------------------------------------------
struct equilibrium_point
{
    // K kelvin
    float temperature;

    // Pa pascals
    float pressure;

    // kg·m⁻³ kilograms per meter cubed
    float density;
};

//------------------------------------------------------------------------------
struct thermal_properties
{
    // W·m⁻¹·K⁻¹ watts per meter-kelvin
    float thermal_conductivity;

    // J·kg⁻¹·K⁻¹ joules per kilogram kelvin
    float specific_heat_capacity;

    // J·kg⁻¹ joules per kilogram
    float enthalpy_of_fusion;

    // J·kg⁻¹ joules per kilogram
    float enthalpy_of_vaporization;

    // J·kg⁻¹ joules per kilogram
    float enthalpy_of_sublimation;

    // <K, Pa, kg·m⁻³>
    equilibrium_point triple_point;

    // <K, Pa, kg·m⁻³>
    equilibrium_point critical_point;
};

//------------------------------------------------------------------------------
struct gas_properties : thermal_properties
{
    // m²·s⁻¹ meters squared per second
    float mass_diffusivity;

    // J·kg⁻¹·K⁻¹ joules per kilogram kelvin
    float specific_gas_constant;
};

//------------------------------------------------------------------------------
constexpr gas_properties air_properties = {
    0.0262f, 1.012e-3f/*wtf*/*1e3f, 0.f, 0.f, 0.f,
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    2e-9f, 287.058f,
};

constexpr gas_properties helium_properties = {
    0.f, 5.19e-3f, 0.f, 0.f, 0.f,
    {2.177f, 5043.f, 0.f},
    {5.1953f, 227460.f, 0.f},
    0.f, 0.f,
};

constexpr gas_properties carbon_dioxide_properties = {
    0.015f, 0.844e-3f, 0.f, 0.f, 0.f,
    {216.55f, 10132000.f, 0.f},
    {304.1f, 7380000.f, .000469f},
    0.f, 0.f,
};

constexpr gas_properties nitrogen_properties = {
    0.026f, 1.04e-3f, 0.f, 0.f, 0.f,
    {63.18f, 12600.f, 0.f},
    {126.2f, 3390000.f, 0.f},
    0.f, 0.f,
};

constexpr gas_properties oxygen_properties = {
    0.026f, 1.04e-3f, 0.f, 0.f, 0.f,
    {54.36f, 152.f, 0.f},
    {154.6f, 5050000.f, 0.f},
    0.f, 0.f,
};

constexpr gas_properties hydrogen_properties = {
    0.1819f, 14.32e-3f, 0.f, 0.f, 0.f,
    {13.84f, 7040.f, 0.f},
    {33.2f, 1300000.f, 0.f},
    0.f, 0.f,
};

constexpr gas_properties methane_properties = {
    0.f, 0.f, 0.f, 480600.f, 0.f,
    {90.68f, 11700.f, 0.f},
    {190.4f, 4600000.f, .000162f},
    0.f, 0.f,
};

constexpr gas_properties chlorine_properties = {
    0.f, .48e-3f, 0.f, 0.f, 0.f,
    {172.17f, 1392.f, 0.f},
    {416.9f, 7991000000.f, 0.f},
    0.f, 0.f,
};

constexpr gas_properties water_vapor_properties = {
    0.025f, 1.67e-3f, 333550.f, 22257000.f, 0.f,
    {273.16f, 611.657f, 0.f},
    {647.f, 22064000.f, .000322f},
    0.f, 0.f,
};

//------------------------------------------------------------------------------
class fluid : public object
{
public:
    static const object_type _type;

public:
    fluid();

    virtual object_type const& type() const override { return _type; }
    virtual void draw(render::system* renderer, time_value time) const override;
    virtual void think() override;

protected:
    static constexpr int kSize = 128;

    float _density[kSize]; // kgm⁻³ kilograms per meter cubed
    float _velocity[kSize]; // m¹s⁻¹ meters per second
    float _temperature[kSize]; // K¹ kelvin

    static constexpr gas_properties properties = air_properties;

    static constexpr float dx = 1.f;
    static constexpr float invdx = 1.f / dx;

    struct tracer {
        float position;
    };

    static constexpr int kNumTracers = kSize / 4;
    tracer _tracers[kNumTracers];

protected:
    float interpolate_density(float x) const;
    float interpolate_velocity(float x) const;
    float interpolate_pressure(float x) const;
    float interpolate_thermal_energy(float x) const;
    float interpolate_temperature(float x) const;

protected:
    void solve_pressure(float* pressure) const;

    void advect_pic(time_delta delta_time);

    void advect_tracers(time_delta delta_time, tracer* tracers) const;

    void diffuse(time_delta delta_time, float nu, float* quantity) const;
};

} // namespace game

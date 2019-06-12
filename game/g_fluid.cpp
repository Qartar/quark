// g_fluid.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "g_fluid.h"

////////////////////////////////////////////////////////////////////////////////
namespace game {

const object_type fluid::_type(object::_type);

//------------------------------------------------------------------------------
fluid::fluid()
    : _density{}
    , _velocity{}
    , _temperature{}
{
    for (int ii = 0; ii < kSize; ++ii) {
        //_density[ii] = sin(3.f / 2.f * math::pi<float> * ii / kSize) * 0.5f + 0.5f;
        _density[ii] = 1.2041f;
        _velocity[ii] = 0.f;
        //_temperature[ii] = (cos(3.f / 2.f * math::pi<float> * ii / kSize) * 0.25f + 0.75f) * 373.f;
        _temperature[ii] = 293.15f;
    }

    for (int ii = 0; ii < kNumTracers; ++ii) {
        _tracers[ii].position = float(ii * kSize / kNumTracers);
    }
}

//------------------------------------------------------------------------------
void fluid::draw(render::system* renderer, time_value /*time*/) const
{
    vec2 pos[kSize * 4];
    color4 col[kSize * 4];
    int idx[kSize * 6];

    float mass = 0.f;
    float thermal_energy = 0.f;
    float kinetic_energy = 0.f;
    for (int ii = 0; ii < kSize; ++ii) {
        mass += _density[ii];
        thermal_energy += _temperature[ii] * mass * properties.specific_heat_capacity;
        kinetic_energy += .5f * _density[ii] * _velocity[ii] * _velocity[ii];

        pos[ii * 4 + 0] = vec2(-kSize + ii * 2.f,        1.f);
        pos[ii * 4 + 1] = vec2(-kSize + ii * 2.f,       -1.f);
        pos[ii * 4 + 2] = vec2(-kSize + ii * 2.f + 2.f,  1.f);
        pos[ii * 4 + 3] = vec2(-kSize + ii * 2.f + 2.f, -1.f);

        col[ii * 4 + 0] =
        col[ii * 4 + 1] =
        col[ii * 4 + 2] =
        col[ii * 4 + 3] = color4(1, 0, 0, 1.f - exp(-.5f * _density[ii]));

        idx[ii * 6 + 0] = ii * 4 + 0;
        idx[ii * 6 + 1] = ii * 4 + 1;
        idx[ii * 6 + 2] = ii * 4 + 2;
        idx[ii * 6 + 3] = ii * 4 + 1;
        idx[ii * 6 + 4] = ii * 4 + 3;
        idx[ii * 6 + 5] = ii * 4 + 2;
    }

    renderer->draw_triangles(pos, col, idx, countof(idx));
    renderer->draw_string(va("%.2f kg", mass), vec2(kSize + 16,0), color4(1,1,1,1));
    renderer->draw_string(va("%.2f kJ", thermal_energy * 1e-3), vec2(kSize + 16,16), color4(1,1,1,1));
    renderer->draw_string(va("%.2f kJ", kinetic_energy * 1e-3), vec2(kSize + 16,32), color4(1,1,1,1));

    renderer->draw_line(vec2(-kSize, 4.f), vec2(kSize - 1, 4.f), color4(1,0,0,.1f), color4(1,0,0,.1f));

    for (int ii = 0; ii < kSize * 4; ++ii) {
        float x0 = float(ii) / 4.f;
        float x1 = x0 + 1.f / 4.f;

        // draw density
        float d0 = interpolate_density(x0);
        float d1 = interpolate_density(x1);

        renderer->draw_line(vec2(-kSize + x0 * 2.f, 4.f + d0 * 96.f),
                            vec2(-kSize + x1 * 2.f, 4.f + d1 * 96.f),
                            color4(0,1,1,.4f), color4(0,1,1,.4f));

        // draw velocity
        float v0 = interpolate_velocity(x0);
        float v1 = interpolate_velocity(x1);

        renderer->draw_line(vec2(-kSize + x0 * 2.f, 4.f + v0 * 8.f),
                            vec2(-kSize + x1 * 2.f, 4.f + v1 * 8.f),
                            color4(0,1,0,.4f), color4(0,1,0,.4f));

        // draw thermal energy
        float e0 = interpolate_thermal_energy(x0);
        float e1 = interpolate_thermal_energy(x1);

        renderer->draw_line(vec2(-kSize + x0 * 2.f, 4.f + e0),
                            vec2(-kSize + x1 * 2.f, 4.f + e1),
                            color4(1,1,0,.4f), color4(1,1,0,.4f));

        // draw kinetic energy
        float k0 = .5f * d0 * v0 * v0;
        float k1 = .5f * d1 * v1 * v1;

        renderer->draw_line(vec2(-kSize + x0 * 2.f, 4.f + k0),
                            vec2(-kSize + x1 * 2.f, 4.f + k1),
                            color4(1,0,1,.4f), color4(1,0,1,.4f));

        // draw temperature
        float t0 = interpolate_temperature(x0);
        float t1 = interpolate_temperature(x1);

        renderer->draw_line(vec2(-kSize + x0 * 2.f, 4.f + t0 * 1.f),
                            vec2(-kSize + x1 * 2.f, 4.f + t1 * 1.f),
                            color4(1,0,0,.4f), color4(1,0,0,.4f));
    }

    for (int ii = 0; ii < kNumTracers; ++ii) {
        renderer->draw_line(vec2(-kSize + _tracers[ii].position * 2.f,  1.f),
                            vec2(-kSize + _tracers[ii].position * 2.f, -1.f),
                            color4(1,1,1,.6f), color4(1,1,1,.6f));
    }
}

//------------------------------------------------------------------------------
float fluid::interpolate_density(float x) const
{
    float x_floor = std::floor(x - .5f);
    int index = int(x_floor);
    float t = x - .5f - x_floor;

    // linear interpolation from x0 to x1
    float x0 = _density[clamp(index + 0, 0, kSize - 1)];
    float x1 = _density[clamp(index + 1, 0, kSize - 1)];

    return x0 * (1.f - t) + x1 * t;
}

//------------------------------------------------------------------------------
float fluid::interpolate_velocity(float x) const
{
    float x_floor = std::floor(x - .5f);
    int index = int(x_floor);
    float t = x - .5f - x_floor;

    // linear interpolation from x0 to x1
    float x0 = _velocity[clamp(index + 0, 0, kSize - 1)];
    float x1 = _velocity[clamp(index + 1, 0, kSize - 1)];

    return x0 * (1.f - t) + x1 * t;
}

//------------------------------------------------------------------------------
float fluid::interpolate_pressure(float x) const
{
    return interpolate_temperature(x) * interpolate_density(x) * properties.specific_gas_constant;
}

//------------------------------------------------------------------------------
float fluid::interpolate_thermal_energy(float x) const
{
    return interpolate_temperature(x) * interpolate_density(x) * properties.specific_heat_capacity;
}

//------------------------------------------------------------------------------
float fluid::interpolate_temperature(float x) const
{
    float x_floor = std::floor(x - .5f);
    int index = int(x_floor);
    float t = x - .5f - x_floor;

    // linear interpolation from q0 to q1
    float t0 = _temperature[clamp(index + 0, 0, kSize - 1)];
    float t1 = _temperature[clamp(index + 1, 0, kSize - 1)];
    return t0 * (1.f - t) + t1 * t;
}

//------------------------------------------------------------------------------
void fluid::think()
{
    constexpr float dt = FRAMETIME.to_seconds();

    advect_pic(FRAMETIME);

    float mass_flux = 5.1f * dt;

    if (_density[0] < 1.f || true) {
        _velocity[0] *= _density[0] / (_density[0] + mass_flux);
        _temperature[0] = (_temperature[0] * _density[0] + 173.f * mass_flux) / (_density[0] + mass_flux);
        _density[0] += mass_flux;
    }

    if (_density[kSize - 1] > mass_flux) {
        _density[kSize - 1] -= mass_flux;
    } else {
        _density[kSize - 1] = 1e-1f;
    }

    float pressure[kSize] = {};
    solve_pressure(pressure);

    float pressure_gradient[kSize + 1] = {};
    for (int ii = 1; ii < kSize; ++ii) {
        pressure_gradient[ii] = pressure[ii] - pressure[ii - 1];
    }

    // boundary conditions
    pressure_gradient[0] = pressure_gradient[1];
    pressure_gradient[kSize] = pressure_gradient[kSize - 1];

    for (int ii = 0; ii < kSize; ++ii) {
        if (ii > 0) {
            _velocity[ii] -= _velocity[ii] * dt * invdx * (_velocity[ii] - _velocity[ii - 1]);
        }
        _velocity[ii] -= 1e-3f * dt * (pressure_gradient[ii + 1] + pressure_gradient[ii]) / max(_density[ii], 1e-3f);
    }

    // diffuse temperature
    diffuse(FRAMETIME, properties.thermal_conductivity / properties.specific_heat_capacity, _temperature);

    // diffuse velocity
    diffuse(FRAMETIME, properties.mass_diffusivity, _velocity);

    advect_tracers(FRAMETIME, _tracers);
}

//------------------------------------------------------------------------------
void fluid::solve_pressure(float* pressure) const
{
    for (int ii = 0; ii < kSize; ++ii) {
        // Ideal gas law: PV = nRT = mRT/M
        pressure[ii] = _density[ii] * invdx * _temperature[ii] * properties.specific_gas_constant;
    }
}

//------------------------------------------------------------------------------
void fluid::advect_pic(time_delta delta_time)
{
    const float dt = delta_time.to_seconds();

    struct particle {
        float position;
        float velocity;
        float mass;
        float specific_energy; // joules per kilogram
    };

    particle particles[kSize * 4];

    for (int ii = 0, jj = 0; ii < kSize; ++ii) {
        for (int kk = 0; kk < 4; ++kk, ++jj) {
            particles[jj].position = (float(jj) + 0.5f) * (invdx / 4.f);
            particles[jj].velocity = interpolate_velocity(particles[jj].position);
            particles[jj].mass = interpolate_density(particles[jj].position) * (dx / 4.f);
            particles[jj].specific_energy = interpolate_temperature(particles[jj].position) * properties.specific_heat_capacity;
        }
    }

    for (int ii = 0; ii < kSize * 4; ++ii) {
        particles[ii].position += particles[ii].velocity * dt;
    }

    float mass[kSize] = {};
    float momentum[kSize] = {};
    float energy[kSize] = {};

    for (int ii = 0; ii < kSize * 4; ++ii) {
        float x_floor = std::floor(particles[ii].position - .5f);
        int index = int(x_floor);
        float t = particles[ii].position - .5f - x_floor;

        int i0 = clamp(index + 0, 0, kSize - 1);
        int i1 = clamp(index + 1, 0, kSize - 1);

        mass[i0] += particles[ii].mass * (1.f - t);
        momentum[i0] += particles[ii].mass * particles[ii].velocity * (1.f - t);
        energy[i0] += particles[ii].mass * particles[ii].specific_energy * (1.f - t);

        mass[i1] += particles[ii].mass * t;
        momentum[i1] += particles[ii].mass * particles[ii].velocity * t;
        energy[i1] += particles[ii].mass * particles[ii].specific_energy * t;
    }

    for (int ii = 0; ii < kSize; ++ii) {
        _density[ii] = mass[ii] * invdx;
        _velocity[ii] = momentum[ii] / mass[ii];
        _temperature[ii] = energy[ii] / (mass[ii] * properties.specific_heat_capacity);
    }
}

//------------------------------------------------------------------------------
void fluid::advect_tracers(time_delta delta_time, tracer* tracers) const
{
    const float dt = delta_time.to_seconds();

    // super dumb advect tracers
    {
        int best_index = -1;
        float best_distance = 0.f;
        //float min_distance = 1e30f;
        int gap_index = 0;
        float gap_distance = 0.f;

        for (int ii = 0; ii < kNumTracers; ++ii) {
            tracers[ii].position += interpolate_velocity(tracers[ii].position) * dt;
        }

        std::sort(tracers, tracers + kNumTracers,
            [](tracer const& lhs, tracer const& rhs) {
                return lhs.position < rhs.position;
            }
        );

        for (int ii = 0; ii < kNumTracers; ++ii) {
            float x0 = tracers[max(0,ii-1)].position;
            float x1 = tracers[ii].position;
            //float x2 = _tracers[min(kSize-1,ii+1)].position;

            //if (x2 - x0 < min_distance && x2 - x0 < .01f * kSize / kNumTracers) {
            //    min_distance = x2 - x0;
            //    best_index = ii;
            //}
            if (x1 - x0 > gap_distance) {
                gap_distance = x1 - x0;
                gap_index = ii;
            }
        }

        if (-tracers[0].position > 0.f) {
            best_distance = -tracers[0].position;
            best_index = 0;
        } 
        if (tracers[kNumTracers - 1].position - kSize > best_distance) {
            best_distance = tracers[kNumTracers - 1].position - kSize;
            best_index = kNumTracers - 1;
        }

        if (best_index != -1) {
            if (tracers[0].position > gap_distance) {
                tracers[best_index].position = 0.f;
            } else if (kSize - 1 - tracers[kNumTracers - 1].position > gap_distance) {
                tracers[best_index].position = kSize - 1 - tracers[0].position;
            } else {
                tracers[best_index].position = (tracers[gap_index - 1].position
                    + tracers[gap_index].position) * 0.5f;
            }
        }
    }
}

//------------------------------------------------------------------------------
void fluid::diffuse(time_delta delta_time, float nu, float* quantity) const
{
    const float dt = delta_time.to_seconds();

    float laplacian[kSize];
    for (int ii = 0; ii < kSize; ++ii) {
        int i0 = max(0, ii - 1);
        int i2 = min(kSize - 1, ii + 1);

        laplacian[ii] = (quantity[i0] * _density[i0] - 2.f * quantity[ii] * _density[ii] + quantity[i2] * _density[i2]) * invdx * invdx;
    }

    for (int ii = 0; ii < kSize; ++ii) {
        quantity[ii] += dt * nu * laplacian[ii] / _density[ii];
    }
}

} // namespace game

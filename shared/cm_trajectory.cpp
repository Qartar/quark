// cm_trajectory.cpp
//

#include "cm_trajectory.h"

//------------------------------------------------------------------------------
/**
 * Implementation of the Stumpff functions using Taylor series
 */
void stumpff_taylor(float x, float* c)
{
    constexpr float rfact[] = {
        1.f / 1.f, 1.f / 1.f, 1.f / 2.f, 1.f / 6.f
    };

    constexpr float coeff[] = {
        1.f / ( 1.f *  2.f),     1.f / ( 2.f *  3.f),
        1.f / ( 3.f *  4.f),     1.f / ( 4.f *  5.f),
        1.f / ( 5.f *  6.f),     1.f / ( 6.f *  7.f),
        1.f / ( 7.f *  8.f),     1.f / ( 8.f *  9.f),
        1.f / ( 9.f * 10.f),     1.f / (10.f * 11.f),
        1.f / (11.f * 12.f),     1.f / (12.f * 13.f),
        1.f / (13.f * 14.f),     1.f / (14.f * 15.f),
        1.f / (15.f * 16.f),     1.f / (16.f * 17.f),
    };

    constexpr int sz = (sizeof(coeff) / sizeof(coeff[0]) - 4) / 2;

    for (int jj = 0; jj < 4; ++jj) {
        c[jj] = 1.f;
        for (int ii = sz; ii >= 0; --ii) {
            c[jj] = 1.f - x * coeff[ii * 2 + jj] * c[jj];
        }
        c[jj] = rfact[jj] * c[jj];
    }
}

//------------------------------------------------------------------------------
/**
 * Implementation of the Stumpff functions using trigonometric identities
 */
void stumpff_trig(float x, float* c)
{
    if (x > +0.0) {
        float y = std::sqrt(+x);
        float cy = std::cos(y / 2.f);
        float sy = std::sin(y / 2.f);

        c[1] = 2.f * cy * sy / y;
        c[2] = 2.f * sy * sy / x;
        c[3] = (1.f - c[1] + std::fma(-2.f * cy, sy / y, c[1])) / x;
        c[0] = 1.f - x * c[2] + std::fma(-x, c[2], x * c[2]);
    } else if (x < 0.f) {
        float y = std::sqrt(-x);
        float cy = std::cosh(y / 2.f);
        float sy = std::sinh(y / 2.f);

        c[1] = 2.f * cy * sy / y;
        c[2] = -2.f * sy * sy / x;
        c[3] = (1.f - c[1] + std::fma(-2.f * cy, sy / y, c[1])) / x;
        c[0] = 1.f - x * c[2] + std::fma(-x, c[2], x * c[2]);
    } else {
        c[0] = 1.f;
        c[1] = 1.f;
        c[2] = .5f;
        c[3] = 1.f / 6.f;
    }
}

//------------------------------------------------------------------------------
void stumpff(float x, float* c)
{
    // Calculate Stumpff coefficients using Taylor series.
    if (std::abs(x) < 1.0) {
        return stumpff_taylor(x, c);
    }
    // Calculate Stumpff coefficients using trig identities.
    else {
        return stumpff_trig(x, c);
    }
}

//------------------------------------------------------------------------------
void stumpff_d(float x, float* c, float* dcdx)
{
    stumpff(x, c);

    if (std::abs(x) == 0.0) {
        dcdx[0] = -1.f / 2.f;
        dcdx[1] = -1.f / 6.f;
        dcdx[2] = -1.f / 24.f;
        dcdx[3] = -1.f / 120.f;
    } else {
        float r2x = 1.f / ( 2.f * x );

        dcdx[0] = -c[1] / 2.f;
        dcdx[1] = r2x * (c[0] - c[1]);
        dcdx[2] = r2x * (c[1] - 2.f * c[2]);
        dcdx[3] = r2x * (c[2] - 3.f * c[3]);
    }
}

////////////////////////////////////////////////////////////////////////////////
trajectory::trajectory(float mu, time_value t0, vec2 r0, vec2 v0)
    : _mu(mu)
    , _t0(t0)
    , _r0(r0)
    , _v0(v0)
{
    _r0norm = length(_r0);
    _alpha = 2.f * _mu / _r0norm - dot(_v0, _v0);
}

//------------------------------------------------------------------------------
void trajectory::calculate(time_value t, vec2* r, vec2* v) const
{
    float s; //!< Reparameterization of `t` s.t. ds/dt = 1/r
    float s2; //!< Square of `s`
    float c[4]; //!< Stumpff coefficients

    // Calculate `s` and Stumpff coefficients
    calculate_s(t, &s, &s2, c);

    // Calculate displament at epoch `t`
    float f = 1.f - _mu / _r0norm * s2 * c[2];
    float g = (t - _t0).to_seconds() - _mu * s * s2 * c[3];

    *r = _r0 * f + _v0 * g;

    // Calculate velocity at epoch `t`
    float rnorm = length(*r);

    float dfdt = -_mu / (rnorm * _r0norm) * s * c[1];
    float dgdt = 1.f - _mu / rnorm * s2 * c[2];

    *v = _r0 * dfdt + _v0 * dgdt;
}

//------------------------------------------------------------------------------
void trajectory::step(float s, vec2* r, vec2* v) const
{
    float s2 = s * s; //!< Square of `s`
    float c[4]; //!< Stumpff coefficients

    // Calculate Stumpff coefficients and derivatives from `s`
    stumpff(_alpha * s2, c);

    // Calculate `t` from `s`
    time_value t = calculate_t(s, s2, c);

    // Calculate displament at epoch `t`
    float f = 1.f - _mu / _r0norm * s2 * c[2];
    float g = (t - _t0).to_seconds() - _mu * s * s2 * c[3];

    *r = _r0 * f + _v0 * g;

    // Calculate velocity at epoch `t`
    float rnorm = length(*r);

    float dfdt = -_mu / (rnorm * _r0norm) * s * c[1];
    float dgdt = 1.f - _mu / rnorm * s2 * c[2];

    *v = _r0 * dfdt + _v0 * dgdt;
}

//------------------------------------------------------------------------------
/**
 * Solve the equation t - t₀ = f(s) for s using the Newton-Raphson method.
 *
 *  where:
 *      f(s) = t - t₀ = r₀‧s‧c₁(αs²) + r₀‧v₀‧s²‧c₂(αs²) + μ‧s³‧c₃(αs²)
 */
void trajectory::calculate_s(time_value t, float* s_ptr, float* s2_ptr, float* c) const
{
    // Initial approximation
    float s = 0.f;
    float s2 = 0.f;
    float ds = (t - _t0).to_seconds() / _r0norm;
    float omega = 1.f;

    for (std::size_t ii = 0; ii < 16; ++ii) {
        // Refine approximation
        s = s + omega * ds; s2 = s * s;

        //! Derivatives of Stumpff coefficients
        float dcds[4];

        // Calculate Stumpff coefficients and derivatives from `s`
        stumpff_d(_alpha * s2, c, dcds);

        // Calculate the differential equation for `t`
        float f_s = (calculate_t(s, s2, c) - t).to_seconds();

        // Calculate the derivative of the differential equation for `t`
        float dfds = calculate_dtds(s, s2, c, dcds);

        float ds0 = -f_s / dfds;

        omega = abs(ds0 / ds) > 1.f  ? 0.5f * omega :
                (ds0 / ds) < 0.f     ? 0.9f * omega :
                omega < (1.f / 1.5f) ? 1.5f * omega : 1.f;

        // Estimate error
        ds = -f_s / dfds;
    }

    // Store results
    *s_ptr = s;
    *s2_ptr = s2;
}

//------------------------------------------------------------------------------
time_value trajectory::calculate_t(float s, float s2, float const* c) const
{
    return _t0 + time_delta::from_seconds(_r0norm * s * c[1]
                                        + dot(_r0, _v0) * s2 * c[2]
                                        + _mu * s * s2 * c[3]);
}

//------------------------------------------------------------------------------
float trajectory::calculate_dtds(float s, float s2, float const* c, float const* dcds) const
{
    float dads = 2.f * _alpha * s; //!< Derivative of Stumpff argument

    return _r0norm * c[1] + _r0norm * s * dcds[1] * dads
            + 2.f * dot(_r0, _v0) * s * c[2] + dot(_r0, _v0) * s2 * dcds[2] * dads
            + 3.f * _mu * s2 * c[3] + _mu * s * s2 * dcds[3] * dads;
}

// cm_table.h
//

#pragma once

#include <vector>

////////////////////////////////////////////////////////////////////////////////
template<typename T> class table
{
public:
    table()
        : _scale(0)
        , _bias(0)
        , _values({})
    {}

    template<std::size_t N>
    table(float scale, float bias, T const (&values)[N])
        : _scale(scale)
        , _bias(bias)
        , _values(values, values + N)
    {}

    T interpolate(float x) const {
        if (!_values.size()) {
            return T{};
        } else if (x < _bias) {
            return _values[0];
        }

        std::size_t i0 = std::size_t((x - _bias) / _scale);
        std::size_t i1 = i0 + 1;

        if (i1 >= _values.size()) {
            return _values.back();
        }

        // linear interpolation
        float t = (x - i0 * _scale - _bias) / _scale;
        return _values[i0] + (_values[i1] - _values[i0]) * t;
    }

protected:
    float _scale;
    float _bias;
    std::vector<T> _values;
};

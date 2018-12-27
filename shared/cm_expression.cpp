// cm_expression.cpp
//

#include "cm_expression.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
float* expression::evaluate(random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    for (std::size_t ii = 0, sz = _ops.size(); ii < sz; ++ii) {
        evaluate_op(ii, r, values + _constants.size());
    }
    return values + _constants.size();
}

//------------------------------------------------------------------------------
float expression::evaluate_one(value val, random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    evaluate_op_r(val, r, values + _constants.size());
    return *(values + _constants.size() + val);
}

//------------------------------------------------------------------------------
void expression::evaluate_op(std::ptrdiff_t index, random& r, float* values) const
{
    expression::op const& op = _ops[index];
    switch (op.type) {
        case op_type::none:
            break;

        case op_type::add:
            values[index] = values[op.lhs] + values[op.rhs];
            break;

        case op_type::subtract:
            values[index] = values[op.lhs] - values[op.rhs];
            break;

        case op_type::multiply:
            values[index] = values[op.lhs] * values[op.rhs];
            break;

        case op_type::divide:
            values[index] = values[op.lhs] / values[op.rhs];
            break;

        case op_type::sqrt:
            values[index] = std::sqrt(values[op.lhs]);
            break;

        case op_type::sine:
            values[index] = std::sin(values[op.lhs]);
            break;

        case op_type::cosine:
            values[index] = std::cos(values[op.lhs]);
            break;

        case op_type::random:
            values[index] = r.uniform_real();
            break;
    }
}

//------------------------------------------------------------------------------
void expression::evaluate_op_r(std::ptrdiff_t index, random& r, float* values) const
{
    expression::op const& op = _ops[index];
    switch (op.type) {
        // independent ops
        case op_type::none:
        case op_type::random:
            evaluate_op(index, r, values);
            break;

        // binary ops
        case op_type::add:
        case op_type::subtract:
        case op_type::multiply:
        case op_type::divide:
            evaluate_op_r(op.lhs, r, values);
            evaluate_op_r(op.rhs, r, values);
            evaluate_op(index, r, values);
            break;

        // unary ops
        case op_type::sqrt:
        case op_type::sine:
        case op_type::cosine:
            evaluate_op_r(op.lhs, r, values);
            evaluate_op(index, r, values);
            break;
    }
}

//------------------------------------------------------------------------------
expression_parser::expression_parser(char const* const* inputs, std::size_t num_inputs)
{
    for (std::size_t ii = 0; ii < num_inputs; ++ii) {
        _symbols[inputs[ii]] = ii;
        _expression._ops.push_back({expression::op_type::none, 0, 0});
    }
    _expression._num_inputs = num_inputs;
}

//------------------------------------------------------------------------------
void expression_parser::assign(char const* name, expression::value value)
{
    if (_symbols.find(name) != _symbols.cend()) {
        //log::warning("
    } else {
        _symbols[name] = value;
    }
}

//------------------------------------------------------------------------------
expression::value expression_parser::parse(char const* begin, char const* end)
{
    while (begin < end && isspace(*begin)) {
        ++begin;
    }

    expression::value lhs;

    if (*begin == '(') {
        char const* subexpr_begin = begin + 1;
        char const* subexpr_end = begin + 1;
        for (int p = 1; p && subexpr_end < end; ++subexpr_end) {
            if (*subexpr_end == '(') {
                ++p;
            } else if (*subexpr_end == ')') {
                --p;
            }
        }

        lhs = parse(subexpr_begin, subexpr_end);
        begin = subexpr_end;
    } else {
        // ...
    }

    return parse_op(lhs, begin, end);
}

//------------------------------------------------------------------------------
expression::value expression_parser::parse_op(expression::value lhs, char const* begin, char const* end)
{
    while (begin < end && isspace(*begin)) {
        ++begin;
    }

    if (begin == end) {
        return lhs;
    }

    if (*begin == '+') {
    } else if (*begin == '-') {
    } else if (*begin == '*') {
    } else if (*begin == '/') {
    }
}

//------------------------------------------------------------------------------
expression::value expression_parser::add_constant(float value)
{
    auto it = _constants.find(value);
    if (it != _constants.cend()) {
        return it->second;
    } else {
        _expression._constants.push_back(value);
        _constants[value] = 0 - _expression._constants.size();
        return 0 - _expression._constants.size();
    }
}

//------------------------------------------------------------------------------
expression::value expression_parser::add_op(expression::op_type type, expression::value lhs, expression::value rhs)
{
    _expression._ops.push_back({type, lhs, rhs});
    return _expression._ops.size() - 1;
}

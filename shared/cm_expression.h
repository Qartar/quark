// cm_expression.h
//

#pragma once

#include "cm_random.h"

#include <map>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
class expression
{
public:
    using value = std::ptrdiff_t;

    enum class op_type {
        none,
        add,
        subtract,
        multiply,
        divide,
        sqrt,
        sine,
        cosine,
        random,
    };

    struct op {
        op_type type;
        value lhs;
        value rhs;
    };

    std::size_t num_inputs() const { return _num_inputs; }
    std::size_t num_values() const { return _ops.size(); }

    float* evaluate(random& r, float const* inputs, float* values) const;
    float evaluate_one(value val, random& r, float const* inputs, float* values) const;

protected:
    friend class expression_parser;

    std::size_t _num_inputs;

    std::vector<op> _ops;
    std::vector<float> _constants;

protected:
    void evaluate_op(std::ptrdiff_t index, random& r, float* values) const;
    void evaluate_op_r(std::ptrdiff_t index, random& r, float* values) const;
};

//------------------------------------------------------------------------------
class expression_parser
{
public:
    expression_parser(char const* const* inputs, std::size_t num_inputs);
    template<std::size_t num_inputs> explicit expression_parser(char const* (&inputs)[num_inputs])
        : expression_parser(inputs, num_inputs) {}

    expression compile() const;

    void assign(char const* name, expression::value value);
    expression::value parse(char const* begin, char const* end);

protected:
    expression _expression;

    std::map<std::string, expression::value> _symbols;
    std::map<float, expression::value> _constants;

protected:
    expression::value add_constant(float value);
    expression::value add_op(expression::op_type type, expression::value lhs, expression::value rhs);

    expression::value parse_op(expression::value lhs, char const* begin, char const* end);
};

// cm_expression.h
//

#pragma once

#include "cm_parser.h"
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
        constant,
        sum,
        difference,
        negative,
        product,
        quotient,
        exponent,
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

    void evaluate(random& r, float const* inputs, float* values) const;
    float evaluate_one(value val, random& r, float const* inputs, float* values) const;

protected:
    friend class expression_builder;

    std::size_t _num_inputs;

    std::vector<op> _ops;
    std::vector<float> _constants;

protected:
    static float evaluate_op(random& r, op_type type, float lhs, float rhs);
    float evaluate_op_r(std::ptrdiff_t index, random& r, float* values) const;
};

//------------------------------------------------------------------------------
class expression_builder
{
public:
    expression_builder(char const* const* inputs, std::size_t num_inputs);
    template<std::size_t num_inputs> explicit expression_builder(char const* (&&inputs)[num_inputs])
        : expression_builder(inputs, num_inputs) {}

    expression compile(expression::value* map) const;

    expression::value add_constant(float value);
    expression::value add_op(expression::op_type type, expression::value lhs, expression::value rhs);

    bool is_constant(expression::value value) const {
        return _expression._ops[value].type == expression::op_type::constant;
    }

    void mark_used(expression::value value);

protected:
    expression _expression;

    std::map<std::string, expression::value> _symbols;
    std::map<float, expression::value> _constants;
    std::vector<bool> _used;
};

//------------------------------------------------------------------------------
class expression_parser : public expression_builder
{
public:
    using token = parser::token;
    template<typename T> using result = parser::result<T>;

    expression_parser(char const* const* inputs, std::size_t num_inputs);
    template<std::size_t num_inputs> explicit expression_parser(char const* (&&inputs)[num_inputs])
        : expression_parser(inputs, num_inputs) {}

    std::size_t num_values() const { return _expression.num_values(); }
    float evaluate_one(expression::value val, random& r, float const* inputs, float* values) const {
        return _expression.evaluate_one(val, r, inputs, values);
    }

    void assign(char const* name, expression::value value);

    result<expression::value> parse_expression(token const*& tokens, token const* end) {
        auto result = parse_expression(tokens, end, INT_MAX);
        if (std::holds_alternative<expression::value>(result)) {
            mark_used(std::get<expression::value>(result));
        }
        return result;
    }

protected:
    result<expression::op_type> parse_operator(token const*& tokens, token const* end);
    //result<expression> parse_binary_function(token const*& tokens, token const* end, expression::op_type type)
    result<expression::value> parse_unary_function(token const*& tokens, token const* end, expression::op_type type, expression::value rhs = 0);
    result<expression::value> parse_operand_explicit(token const*& tokens, token const* end);
    result<expression::value> parse_operand(token const*& tokens, token const* end);
    result<expression::value> parse_expression(token const*& tokens, token const* end, int precedence);
};

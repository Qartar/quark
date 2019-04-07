// cm_expression.h
//

#pragma once

#include "cm_parser.h"
#include "cm_random.h"

#include <map>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
class expression
{
public:
    using value = std::ptrdiff_t;

    enum class type {
        scalar,
        vec2,
        vec4,
    };

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
        member,
    };

    struct op {
        op_type type;
        value lhs;
        value rhs;
    };

    //! Number of operands used by an op_type
    enum class op_arity {
        nullary,    //!< op has no operands
        unary,      //!< op has a single operand
        binary,     //!< op has two operands
    };

    //! Returns the number of operands used by the given op_type
    static constexpr expression::op_arity arity(expression::op_type op) {
        switch (op) {
            // nullary ops
            case expression::op_type::none:
            case expression::op_type::constant:
            case expression::op_type::random:
                return op_arity::nullary;

            // unary ops
            case expression::op_type::negative:
            case expression::op_type::sqrt:
            case expression::op_type::sine:
            case expression::op_type::cosine:
            case expression::op_type::member:
                return op_arity::unary;

            // binary ops
            case expression::op_type::sum:
            case expression::op_type::difference:
            case expression::op_type::product:
            case expression::op_type::quotient:
            case expression::op_type::exponent:
                return op_arity::binary;
        }
        // rely on compiler warnings to detect missing case labels
        __assume(false);
    }

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
    expression_builder(string::view const* inputs, std::size_t num_inputs);
    template<std::size_t num_inputs> explicit expression_builder(string::view (&&inputs)[num_inputs])
        : expression_builder(inputs, num_inputs) {}

    expression compile(expression::value* map) const;

    expression::value add_constant(float value);
    expression::value add_op(expression::op_type type, expression::value lhs, expression::value rhs);

    bool is_constant(expression::value value) const {
        return _expression._ops[value].type == expression::op_type::constant;
    }

    //! Returns true if the given value depends on the random generator during evaluation
    bool is_random(expression::value value) const;

    void mark_used(expression::value value);

protected:
    struct field_info {
        string::buffer name;
        expression::type type;
        std::size_t offset;
    };

    struct type_info {
        string::buffer name;
        std::vector<field_info> fields;
        std::size_t size;
    };

protected:
    expression _expression;

    std::map<string::buffer, expression::value> _symbols;
    std::map<float, expression::value> _constants;
    std::vector<bool> _used;
    std::vector<expression::type> _types;

    std::vector<type_info> _type_info;

protected:
    expression::value alloc_type(expression::type type);
};

//------------------------------------------------------------------------------
class expression_parser : public expression_builder
{
public:
    using token = parser::token;
    template<typename T> using result = parser::result<T>;

    expression_parser(string::view const* inputs, std::size_t num_inputs);
    template<std::size_t num_inputs> explicit expression_parser(string::view (&&inputs)[num_inputs])
        : expression_parser(inputs, num_inputs) {}

    std::size_t num_values() const { return _expression.num_values(); }
    float evaluate_one(expression::value val, random& r, float const* inputs, float* values) const {
        return _expression.evaluate_one(val, r, inputs, values);
    }

    void assign(string::view name, expression::value value);

    result<expression::value> parse_expression(parser::context& context) {
        auto result = parse_expression(context, INT_MAX);
        if (std::holds_alternative<expression::value>(result)) {
            mark_used(std::get<expression::value>(result));
        }
        return result;
    }

    result<expression::value> parse_temporary(parser::context& context) {
        return parse_expression(context, INT_MAX);
    }

protected:
    result<expression::op_type> peek_operator(parser::context const& context) const;
    result<expression::op_type> parse_operator(parser::context& context);
    //result<expression> parse_binary_function(parser::context& context, expression::op_type type)
    result<expression::value> parse_unary_function(parser::context& context, expression::op_type type, expression::value rhs = 0);
    result<expression::value> parse_operand_explicit(parser::context& context);
    result<expression::value> parse_operand(parser::context& context);
    result<expression::value> parse_expression(parser::context& context, int precedence);
    result<expression::value> parse_member(parser::context& context, type_info const& type);
};

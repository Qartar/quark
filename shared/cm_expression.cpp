// cm_expression.cpp
//

#include "cm_expression.h"
#include "cm_shared.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////

namespace {

constexpr bool is_nullary(expression::op_type op)
{
    return expression::arity(op) == expression::op_arity::nullary;
}

constexpr bool is_unary(expression::op_type op)
{
    return expression::arity(op) == expression::op_arity::unary;
}

constexpr bool is_binary(expression::op_type op)
{
    return expression::arity(op) == expression::op_arity::binary;
}

} // anonymous namespace

//------------------------------------------------------------------------------
void expression::evaluate(random& r, float const* inputs, float* values) const
{
    memcpy(values, inputs, sizeof(float) * _num_inputs);
    memcpy(values + _num_inputs, _constants.data(), sizeof(float) * _constants.size());

    for (std::size_t ii = 0, sz = _ops.size(); ii < sz; ++ii) {
        if (_ops[ii].type != op_type::none && _ops[ii].type != op_type::constant) {
            values[ii] = evaluate_op(r, _ops[ii].type, values[_ops[ii].lhs], values[_ops[ii].rhs]);
        }
    }
}

//------------------------------------------------------------------------------
float expression::evaluate_one(value val, random& r, float const* inputs, float* values) const
{
    memcpy(values, inputs, sizeof(float) * _num_inputs);
    memcpy(values + _num_inputs, _constants.data(), sizeof(float) * _constants.size());

    return evaluate_op_r(val, r, values);
}

//------------------------------------------------------------------------------
float expression::evaluate_op(random& r, op_type type, float lhs, float rhs)
{
    switch (type) {
        case op_type::none:
        case op_type::constant:
            assert(false);
            return 0.f;

        case op_type::sum:
            return lhs + rhs;

        case op_type::difference:
            return lhs - rhs;

        case op_type::negative:
            return -lhs;

        case op_type::product:
            return lhs * rhs;

        case op_type::quotient:
            return lhs / rhs;

        case op_type::exponent:
            return std::pow(lhs, rhs);

        case op_type::sqrt:
            return std::sqrt(lhs);

        case op_type::sine:
            return std::sin(lhs);

        case op_type::cosine:
            return std::cos(lhs);

        case op_type::random:
            return r.uniform_real();
    }
    // rely on compiler warnings to detect missing case labels
    __assume(false);
}

//------------------------------------------------------------------------------
float expression::evaluate_op_r(std::ptrdiff_t index, random& r, float* values) const
{
    expression::op const& op = _ops[index];

    switch (op.type) {
        // no-op
        case op_type::none:
        case op_type::constant:
            return values[index];

        default: {
            // evaluate operands
            float lhs = 0.f;
            float rhs = 0.f;

            switch (arity(op.type)) {
                case op_arity::nullary:
                    break;

                case op_arity::unary:
                    lhs = evaluate_op_r(op.lhs, r, values);
                    break;

                case op_arity::binary:
                    lhs = evaluate_op_r(op.lhs, r, values);
                    rhs = evaluate_op_r(op.rhs, r, values);
                    break;
            }

            return evaluate_op(r, op.type, lhs, rhs);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
expression_builder::expression_builder(char const* const* inputs, std::size_t num_inputs)
{
    for (std::size_t ii = 0; ii < num_inputs; ++ii) {
        _symbols[inputs[ii]] = ii;
        _expression._ops.push_back({expression::op_type::none, 0, 0});
        _used.push_back(true); // assume inputs are always used
    }
    _expression._num_inputs = num_inputs;
}

//------------------------------------------------------------------------------
expression expression_builder::compile(expression::value* map) const
{
    expression out;
    std::size_t num_used = 0;

    // always use all inputs, even if they aren't referenced
    out._num_inputs = _expression._num_inputs;
    for (std::size_t ii = 0, sz = _expression.num_inputs(); ii < sz; ++ii) {
        map[ii] = num_used++;
    }

    // make sure all constants are placed in front of the calculated values
    for (std::size_t ii = _expression.num_inputs(), sz = _used.size(); ii < sz; ++ii) {
        if (_used[ii] && _expression._ops[ii].type == expression::op_type::constant) {
            out._constants.push_back(_expression._constants[ii - _expression.num_inputs()]);
            map[ii] = num_used++;
        }
    }

    for (std::size_t ii = _expression.num_inputs(), sz = _used.size(); ii < sz; ++ii) {
        if (_used[ii] && _expression._ops[ii].type != expression::op_type::constant) {
            map[ii] = num_used++;
        }
    }

    out._ops.resize(num_used);
    for (std::size_t ii = 0, sz = _used.size(); ii < sz; ++ii) {
        if (_used[ii]) {
            expression::value jj = map[ii];
            out._ops[jj].type = _expression._ops[ii].type;
            out._ops[jj].lhs = map[_expression._ops[ii].lhs];
            out._ops[jj].rhs = map[_expression._ops[ii].rhs];
            // assert that operands are evaluated prior to this value
            assert(is_nullary(out._ops[jj].type) || out._ops[jj].lhs <= jj);
            assert(!is_binary(out._ops[jj].type) || out._ops[jj].rhs <= jj);
        }
    }

    return out;
}

//------------------------------------------------------------------------------
expression::value expression_builder::add_constant(float value)
{
    auto it = _constants.find(value);
    if (it != _constants.cend()) {
        return it->second;
    } else {
        _expression._ops.push_back({expression::op_type::constant, 0, 0});
        _expression._constants.push_back(value);
        _constants[value] = _expression._ops.size() - 1;
        _used.push_back(false);
        return _expression._ops.size() - 1;
    }
}

//------------------------------------------------------------------------------
expression::value expression_builder::add_op(expression::op_type type, expression::value lhs, expression::value rhs)
{
    if (is_constant(lhs) && is_constant(rhs)) {
        static random r; // not used
        return add_constant(
            expression::evaluate_op(
                r,
                type,
                _expression._constants[lhs - _expression._num_inputs],
                _expression._constants[rhs - _expression._num_inputs]));
    } else {
        _expression._constants.push_back(0.f);
        _expression._ops.push_back({type, lhs, rhs});
        _used.push_back(false);
        return _expression._ops.size() - 1;
    }
}

//------------------------------------------------------------------------------
bool expression_builder::is_random(expression::value value) const
{
    expression::op const& op = _expression._ops[value];
    if (op.type == expression::op_type::random) {
        return true;
    } else {
        switch (expression::arity(op.type)) {
            case expression::op_arity::nullary:
                return false;

            case expression::op_arity::unary:
                return is_random(op.lhs);

            case expression::op_arity::binary:
                return is_random(op.lhs) || is_random(op.rhs);
        }
        // rely on compiler warnings to detect missing case labels
        __assume(false);
    }
}

//------------------------------------------------------------------------------
void expression_builder::mark_used(expression::value value)
{
    // no need to mark operands if already marked as used
    if (_used[value]) {
        return;
    }

    _used[value] = true;

    expression::op const& op = _expression._ops[value];
    switch (expression::arity(op.type)) {
        case expression::op_arity::nullary:
            break;

        case expression::op_arity::unary:
            mark_used(op.lhs);
            break;

        case expression::op_arity::binary:
            mark_used(op.lhs);
            mark_used(op.rhs);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
expression_parser::expression_parser(char const* const* inputs, std::size_t num_inputs)
    : expression_builder(inputs, num_inputs)
{
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

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
parser::result<expression::op_type> expression_parser::peek_operator(parser::context const& context) const
{
    auto result = context.peek_token();
    if (std::holds_alternative<parser::error>(result)) {
        // TODO: "expected operator ..."
        return std::get<parser::error>(result);
    }

    auto token = std::get<parser::token>(result);
    if (token == '+') {
        return expression::op_type::sum;
    } else if (token == '-') {
        return expression::op_type::difference;
    } else if (token == '*') {
        return expression::op_type::product;
    } else if (token == '/') {
        return expression::op_type::quotient;
    } else if (token == '^') {
        return expression::op_type::exponent;
    } else {
        return parser::error{token, "expected operator, found '" + token + "'"};
    }
}

//------------------------------------------------------------------------------
parser::result<expression::op_type> expression_parser::parse_operator(parser::context& context)
{
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        // TODO: "expected operator ..."
        return std::get<parser::error>(result);
    }

    auto token = std::get<parser::token>(result);
    if (token == '+') {
        return expression::op_type::sum;
    } else if (token == '-') {
        return expression::op_type::difference;
    } else if (token == '*') {
        return expression::op_type::product;
    } else if (token == '/') {
        return expression::op_type::quotient;
    } else if (token == '^') {
        return expression::op_type::exponent;
    } else {
        return context.set_error({token, "expected operator, found '" + token + "'"});
    }
}

//------------------------------------------------------------------------------
//result<expression> parse_binary_function(token const*& tokens, token const* end, expression::op_type type)
//{
//    if (tokens >= end) {
//        return error{*(tokens - 1), "expected '(' after '" + *(tokens - 1) + "'"};
//    } else if (*tokens != '(') {
//        return error{*tokens, "expected '(' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
//    } else {
//        ++tokens;
//    }
//
//    result<expression> lhs = parse_expression(context);
//    if (std::holds_alternative<error>(lhs)) {
//        return std::get<error>(lhs);
//    }
//
//    if (tokens >= end) {
//        return error{*(tokens - 1), "expected ',' after '" + *(tokens - 1) + "'"};
//    } else if (*tokens != ',') {
//        return error{*tokens, "expected ',' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
//    } else {
//        ++tokens;
//    }
//
//    result<expression> rhs = parse_expression(context);
//    if (std::holds_alternative<error>(rhs)) {
//        return std::get<error>(rhs);
//    }
//
//    if (tokens >= end) {
//        return error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
//    } else if (*tokens != ')') {
//        return error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
//    } else {
//        ++tokens;
//    }
//
//    return expression::op{type, std::get<expression>(lhs), std::get<expression>(rhs)};
//}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_unary_function(parser::context& context, expression::op_type type, expression::value rhs)
{
    result<expression::value> arg = parse_operand(context);
    if (std::holds_alternative<parser::error>(arg)) {
        return std::get<parser::error>(arg);
    }
    return add_op(type, std::get<expression::value>(arg), rhs);
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_operand_explicit(parser::context& context)
{
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        // TODO: "expected expression ..."
        return std::get<parser::error>(result);
    }

    auto token = std::get<parser::token>(result);
    if (token == ')' || token == ',' || token == ';') {
        //context.set_error();
        //return parser::error{token, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
        return context.get_error();
    }

    if (token == '(') {
        parser::result<expression::value> expr = parse_expression(context);
        if (std::holds_alternative<parser::error>(expr)) {
            return std::get<parser::error>(expr);
        }
        if (!context.expect_token(")")) {
            return context.get_error();
        } else {
            return std::get<expression::value>(expr);
        }

    //
    //  functions
    //

    } else if (token == "random") {
        return add_op(expression::op_type::random, 0, 0);
    //} else if (token == "ln") {
    //    return parse_unary_function(context, op_type::logarithm, constant::e);
    //} else if (token == "log") {
    //    return parse_binary_function(context, op_type::logarithm);
    } else if (token == "sin") {
        return parse_unary_function(context, expression::op_type::sine);
    } else if (token == "cos") {
        return parse_unary_function(context, expression::op_type::cosine);
    //} else if (token == "tan") {
    //    return parse_unary_function(context, op_type::tangent);
    //} else if (token == "sec") {
    //    return parse_unary_function(context, op_type::secant);
    //} else if (token == "csc") {
    //    return parse_unary_function(context, op_type::cosecant);
    //} else if (token == "cot") {
    //    return parse_unary_function(context, op_type::cotangent);

    //
    //  constants
    //

    } else if (token == "pi") {
        return add_constant(math::pi<float>);
    } else if (token == 'e') {
        return add_constant(math::e<float>);

    //
    //  values
    //

    } else if (*token.begin >= '0' && *token.begin <= '9' || *token.begin == '.') {
        expression::value v = add_constant((float)std::atof(token.begin));
        return v;

    //
    //  symbols
    //

    } else if (*token.begin >= 'a' && *token.begin <= 'z' || *token.begin >= 'A' && *token.begin <= 'Z') {
        auto s = _symbols.find(std::string(token.begin, token.end));
        if (s == _symbols.cend()) {
            return context.set_error({token, "unrecognized symbol: '" + token + "'"});
        }
        return s->second;

    } else {
        return context.set_error({token, "syntax error: '" + token + "'"});
    }
}

//------------------------------------------------------------------------------
constexpr int op_precedence(expression::op_type t)
{
    switch (t) {
        case expression::op_type::sum: return 6;
        case expression::op_type::difference: return 6;
        case expression::op_type::product: return 5;
        case expression::op_type::quotient: return 5;
        case expression::op_type::exponent: return 4;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_operand(parser::context& context)
{
#if 0
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        // TODO: "expected expression ..."
        return std::get<parser::error>(result);
    }

    auto token = std::get<parser::token>(result);
    if (token == ')' || token == ',' || token == ';') {
        //return parser::error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
        return context.get_error();
    }

    expression::value out;
    bool is_negative = false;

    // check for negation
    if (token == '-') {
        is_negative = true;
    }

    parser::result<expression::value> operand = parse_operand_explicit(context);
    if (std::holds_alternative<parser::error>(operand)) {
        return std::get<parser::error>(operand);
    } else {
        out = std::get<expression::value>(operand);
    }

    // apply negation after implicit multiplication
    if (is_negative) {
        return add_op(expression::op_type::negative, out, 0);
    } else {
        return out;
    }
#else
    return parse_operand_explicit(context);
#endif
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_expression(parser::context& context, int precedence)
{
    parser::result<expression::value> lhs = parse_operand(context);
    if (std::holds_alternative<parser::error>(lhs)) {
        return std::get<parser::error>(lhs);
    }

    while (context.has_token()
        && !context.peek_token(")")
        && !context.peek_token(",")
        && !context.peek_token(";")) {


        parser::result<expression::op_type> lhs_op = peek_operator(context);
        if (std::holds_alternative<parser::error>(lhs_op)) {
            return std::get<parser::error>(lhs_op);
        }

        int lhs_precedence = op_precedence(std::get<expression::op_type>(lhs_op));
        if (precedence <= lhs_precedence) {
            break;
        } else {
            lhs_op = parse_operator(context);
            assert(!std::holds_alternative<parser::error>(lhs_op));
        }

        parser::result<expression::value> rhs = parse_expression(context, lhs_precedence);
        if (std::holds_alternative<parser::error>(rhs)) {
            return std::get<parser::error>(rhs);
        } else {
            lhs = add_op(std::get<expression::op_type>(lhs_op),
                         std::get<expression::value>(lhs),
                         std::get<expression::value>(rhs));
        }
    }

    return lhs;
}

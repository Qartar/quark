// cm_expression.cpp
//

#include "cm_expression.h"
#include "cm_shared.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
void expression::evaluate(random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    for (std::size_t ii = 0, sz = _ops.size(); ii < sz; ++ii) {
        if (_ops[ii].type != op_type::none && _ops[ii].type != op_type::constant) {
            values[ii] = evaluate_op(r, _ops[ii].type, values[_ops[ii].lhs], values[_ops[ii].rhs]);
        }
    }
}

//------------------------------------------------------------------------------
float expression::evaluate_one(value val, random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    return evaluate_op_r(val, r, values);
}

//------------------------------------------------------------------------------
float expression::evaluate_op(random& r, op_type type, float lhs, float rhs)
{
    switch (type) {
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

        case op_type::constant:
        case op_type::none:
        default:
            assert(false);
            return 0.f;
    }
}

//------------------------------------------------------------------------------
float expression::evaluate_op_r(std::ptrdiff_t index, random& r, float* values) const
{
    expression::op const& op = _ops[index];
    float lhs = 0.f;
    float rhs = 0.f;

    switch (op.type) {
        // no-op
        case op_type::none:
        case op_type::constant:
            return values[index];

        // independent ops
        case op_type::random:
            break;

        // binary ops
        case op_type::sum:
        case op_type::difference:
        case op_type::product:
        case op_type::quotient:
        case op_type::exponent:
            lhs = evaluate_op_r(op.lhs, r, values);
            rhs = evaluate_op_r(op.rhs, r, values);
            break;

        // unary ops
        case op_type::negative:
        case op_type::sqrt:
        case op_type::sine:
        case op_type::cosine:
            lhs = evaluate_op_r(op.lhs, r, values);
            break;
    }

    return evaluate_op(r, op.type, lhs, rhs);
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
expression_builder::expression_builder(char const* const* inputs, std::size_t num_inputs)
{
    for (std::size_t ii = 0; ii < num_inputs; ++ii) {
        _symbols[inputs[ii]] = ii;
        _expression._ops.push_back({expression::op_type::none, 0, 0});
    }
    _expression._num_inputs = num_inputs;
}

//------------------------------------------------------------------------------
expression expression_builder::compile(expression::value* map) const
{
    expression out;
    out._num_inputs = _expression._num_inputs;
    std::vector<bool> used(_expression._ops.size(), false);
    for (std::size_t ii = 0, sz = used.size(); ii < sz; ++ii) {
        expression::op const& op = _expression._ops[ii];
        switch (op.type) {
            // no-op
            case expression::op_type::none:
            case expression::op_type::constant:
            case expression::op_type::random:
                break;

            // binary ops
            case expression::op_type::sum:
            case expression::op_type::difference:
            case expression::op_type::product:
            case expression::op_type::quotient:
            case expression::op_type::exponent:
                used[op.lhs] = true;
                used[op.rhs] = true;
                used[ii] = true;
                break;

            // unary ops
            case expression::op_type::negative:
            case expression::op_type::sqrt:
            case expression::op_type::sine:
            case expression::op_type::cosine:
                used[op.lhs] = true;
                used[ii] = true;
                break;
        }
    }

    std::size_t num_used = 0;
    // always use all inputs, even if they aren't referenced
    for (std::size_t ii = 0, sz = _expression.num_inputs(); ii < sz; ++ii) {
        map[ii] = num_used++;
    }
    // make sure all constants are placed in front of the calculated values
    for (std::size_t ii = _expression.num_inputs(), sz = used.size(); ii < sz; ++ii) {
        if (used[ii] && _expression._ops[ii].type == expression::op_type::constant) {
            out._constants.push_back(_expression._constants[ii]);
            map[ii] = num_used++;
        }
    }

    for (std::size_t ii = _expression.num_inputs(), sz = used.size(); ii < sz; ++ii) {
        if (used[ii] && _expression._ops[ii].type != expression::op_type::constant) {
            map[ii] = num_used++;
        }
    }

    out._ops.resize(num_used);
    for (std::size_t ii = 0, jj = 0; jj < num_used; ++ii) {
        if (used[ii]) {
            out._ops[jj].type = _expression._ops[ii].type;
            out._ops[jj].lhs = map[_expression._ops[ii].lhs];
            out._ops[jj].rhs = map[_expression._ops[ii].rhs];
            ++jj;
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
        return _expression._ops.size() - 1;
    }
}

//------------------------------------------------------------------------------
expression::value expression_builder::add_op(expression::op_type type, expression::value lhs, expression::value rhs)
{
    if (_expression._ops[lhs].type == expression::op_type::constant
            && _expression._ops[rhs].type == expression::op_type::constant) {
        static random r; // not used
        return add_constant(
            expression::evaluate_op(
                r,
                type,
                _expression._constants[lhs],
                _expression._constants[rhs]));
    } else {
        _expression._constants.push_back(0.f);
        _expression._ops.push_back({type, lhs, rhs});
        return _expression._ops.size() - 1;
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
parser::result<expression::op_type> expression_parser::parse_operator(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return parser::error{*(tokens - 1), "expected operator after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return parser::error{*tokens, "expected operator after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (tokens[0] == '=') {
        ++tokens;
        return expression::op_type::equality;
    } else if (tokens[0] == '+') {
        ++tokens;
        return expression::op_type::sum;
    } else if (tokens[0] == '-') {
        ++tokens;
        return expression::op_type::difference;
    } else if (tokens[0] == '*') {
        ++tokens;
        return expression::op_type::product;
    } else if (tokens[0] == '/') {
        ++tokens;
        return expression::op_type::quotient;
    } else if (tokens[0] == '^') {
        ++tokens;
        return expression::op_type::exponent;
    } else {
        return parser::error{*tokens, "expected operator, found '" + *tokens + "'"};
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
//    result<expression> lhs = parse_expression(tokens, end);
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
//    result<expression> rhs = parse_expression(tokens, end);
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
parser::result<expression::value> expression_parser::parse_unary_function(token const*& tokens, token const* end, expression::op_type type, expression::value rhs)
{
    result<expression::value> arg = parse_operand(tokens, end);
    if (std::holds_alternative<parser::error>(arg)) {
        return std::get<parser::error>(arg);
    }
    return add_op(type, std::get<expression::value>(arg), rhs);
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_operand_explicit(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return parser::error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return parser::error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (*tokens == '(') {
        result<expression::value> expr = parse_expression(++tokens, end);
        if (std::holds_alternative<parser::error>(expr)) {
            return std::get<parser::error>(expr);
        }
        if (tokens >= end) {
            return parser::error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
        } else if (*tokens != ')') {
            return parser::error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
        }
        ++tokens;
        return std::get<expression::value>(expr);

    //
    //  functions
    //

    } else if (*tokens == "random") {
        ++tokens;
        return add_op(expression::op_type::random, 0, 0);
    //} else if (*tokens == "ln") {
    //    return parse_unary_function(++tokens, end, op_type::logarithm, constant::e);
    //} else if (*tokens == "log") {
    //    return parse_binary_function(++tokens, end, op_type::logarithm);
    } else if (*tokens == "sin") {
        return parse_unary_function(++tokens, end, expression::op_type::sine);
    } else if (*tokens == "cos") {
        return parse_unary_function(++tokens, end, expression::op_type::cosine);
    //} else if (*tokens == "tan") {
    //    return parse_unary_function(++tokens, end, op_type::tangent);
    //} else if (*tokens == "sec") {
    //    return parse_unary_function(++tokens, end, op_type::secant);
    //} else if (*tokens == "csc") {
    //    return parse_unary_function(++tokens, end, op_type::cosecant);
    //} else if (*tokens == "cot") {
    //    return parse_unary_function(++tokens, end, op_type::cotangent);

    //
    //  constants
    //

    } else if (tokens[0] == "pi") {
        ++tokens;
        return add_constant(math::pi<float>);
    //} else if (tokens[0] == 'e') {
    //    ++tokens;
    //    return constant::e;
    //} else if (tokens[0] == 'i') {
    //    ++tokens;
    //    return constant::i;

    //
    //  values
    //

    } else if (*tokens[0].begin >= '0' && *tokens[0].begin <= '9' || *tokens[0].begin == '.') {
        expression::value v = add_constant((float)std::atof(tokens[0].begin));
        ++tokens;
        return v;

    //
    // derivative
    //

    //} else if (end - tokens > 2 && tokens[0] == 'd' && tokens[1] == '/' && tokens[2][0] == 'd') {
    //    symbol s{tokens[2].begin + 1, tokens[2].end};
    //    tokens += 3;
    //    return parse_unary_function(tokens, end, op_type::derivative, s);

    //
    //  symbols
    //

    } else if (*tokens[0].begin >= 'a' && *tokens[0].begin <= 'z' || *tokens[0].begin >= 'A' && *tokens[0].begin <= 'Z') {
        auto s = _symbols.find(std::string(tokens[0].begin, tokens[0].end));
        if (s == _symbols.cend()) {
            return parser::error{*tokens, "unrecognized symbol: '" + *tokens + "'"};
        }
        ++tokens;
        return s->second;

    } else {
        return parser::error{*tokens, "syntax error: '" + *tokens + "'"};
    }
}

//------------------------------------------------------------------------------
constexpr int op_precedence(expression::op_type t)
{
    switch (t) {
        case expression::op_type::equality: return 16;
        case expression::op_type::sum: return 6;
        case expression::op_type::difference: return 6;
        case expression::op_type::product: return 5;
        case expression::op_type::quotient: return 5;
        case expression::op_type::exponent: return 4;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_operand(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return parser::error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return parser::error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    expression::value out;
    bool is_negative = false;

    // check for negation
    if (*tokens == '-') {
        is_negative = true;
        ++tokens;
    }

    result<expression::value> operand = parse_operand_explicit(tokens, end);
    if (std::holds_alternative<parser::error>(operand)) {
        return std::get<parser::error>(operand);
    } else {
        out = std::get<expression::value>(operand);
    }

    // check for implicit multiplication, e.g. `3x`
    //if (std::holds_alternative<value>(out) || std::holds_alternative<constant>(out)) {
    //    token const* saved = tokens;

    //    // do not parse `3-x` as `(3)(-x)`
    //    if (tokens < end && *tokens != '-') {
    //        result<expression> next = parse_expression(tokens, end, op_precedence(op_type::product));
    //        if (std::holds_alternative<error>(next)
    //            || std::holds_alternative<value>(std::get<expression>(next))) {
    //            tokens = saved;
    //        } else {
    //            out = op{op_type::product, out, std::get<expression>(next)};
    //        }
    //    }
    //}

    // apply negation after implicit multiplication
    if (is_negative) {
        return add_op(expression::op_type::negative, out, 0);
    } else {
        return out;
    }
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_expression(token const*& tokens, token const* end, int precedence)
{
    result<expression::value> lhs = parse_operand(tokens, end);
    if (std::holds_alternative<parser::error>(lhs)) {
        return std::get<parser::error>(lhs);
    }

    while (tokens < end && *tokens != ')' && *tokens != ',') {
        token const* saved = tokens;

        result<expression::op_type> lhs_op = parse_operator(tokens, end);
        if (std::holds_alternative<parser::error>(lhs_op)) {
            return std::get<parser::error>(lhs_op);
        }

        int lhs_precedence = op_precedence(std::get<expression::op_type>(lhs_op));
        if (precedence <= lhs_precedence) {
            tokens = saved;
            break;
        }

        result<expression::value> rhs = parse_expression(tokens, end, lhs_precedence);
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

// cm_expression.cpp
//

#include "cm_expression.h"
#include "cm_shared.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
float* expression::evaluate(random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    for (std::size_t ii = 0, sz = _ops.size(); ii < sz; ++ii) {
        evaluate_op(ii, r, values);
    }
    return values + _constants.size();
}

//------------------------------------------------------------------------------
float expression::evaluate_one(value val, random& r, float const* inputs, float* values) const
{
    memcpy(values, _constants.data(), sizeof(float) * _constants.size());
    memcpy(values + _constants.size(), inputs, sizeof(float) * _num_inputs);

    evaluate_op_r(val, r, values);
    return *(values + val);
}

//------------------------------------------------------------------------------
void expression::evaluate_op(std::ptrdiff_t index, random& r, float* values) const
{
    expression::op const& op = _ops[index];
    switch (op.type) {
        case op_type::none:
            break;

        case op_type::sum:
            values[index] = values[op.lhs] + values[op.rhs];
            break;

        case op_type::difference:
            values[index] = values[op.lhs] - values[op.rhs];
            break;

        case op_type::negative:
            values[index] = -values[op.lhs];
            break;

        case op_type::product:
            values[index] = values[op.lhs] * values[op.rhs];
            break;

        case op_type::quotient:
            values[index] = values[op.lhs] / values[op.rhs];
            break;

        case op_type::exponent:
            values[index] = std::pow(values[op.lhs], values[op.rhs]);
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
    if (index < 0) {
        return;
    }

    expression::op const& op = _ops[index];
    switch (op.type) {
        // independent ops
        case op_type::none:
        case op_type::random:
            evaluate_op(index, r, values);
            break;

        // binary ops
        case op_type::sum:
        case op_type::difference:
        case op_type::product:
        case op_type::quotient:
        case op_type::exponent:
            evaluate_op_r(op.lhs, r, values);
            evaluate_op_r(op.rhs, r, values);
            evaluate_op(index, r, values);
            break;

        // unary ops
        case op_type::negative:
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
    end;
    result<parser::tokenized> tokens = parser::tokenize(begin);
    if (std::holds_alternative<parser::error>(tokens)) {
        log::message("%s\n", std::get<parser::error>(tokens).msg.c_str());
        return 0;
    }
    token const* tbegin = &std::get<parser::tokenized>(tokens)[0];
    token const* tend = tbegin + std::get<parser::tokenized>(tokens).size();
    result<expression::value> value = parse_expression(tbegin, tend);
    if (std::holds_alternative<parser::error>(value)) {
        log::message("%s\n", std::get<parser::error>(value).msg.c_str());
        return 0;
    }

    return std::get<expression::value>(value);
}

//------------------------------------------------------------------------------
expression::value expression_parser::add_constant(float value)
{
    auto it = _constants.find(value);
    if (it != _constants.cend()) {
        return it->second;
    } else {
        _expression._ops.push_back({expression::op_type::none, 0, 0});
        _expression._constants.push_back(value);
        _constants[value] = _expression._ops.size() - 1;
        return _expression._ops.size() - 1;
    }
}

//------------------------------------------------------------------------------
expression::value expression_parser::add_op(expression::op_type type, expression::value lhs, expression::value rhs)
{
    _expression._constants.push_back(0.f);
    _expression._ops.push_back({type, lhs, rhs});
    return _expression._ops.size() - 1;
}

////////////////////////////////////////////////////////////////////////////////
using namespace parser;

//------------------------------------------------------------------------------
parser::result<expression::op_type> expression_parser::parse_operator(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected operator after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected operator after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
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
        return error{*tokens, "expected operator, found '" + *tokens + "'"};
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
    if (std::holds_alternative<error>(arg)) {
        return std::get<error>(arg);
    }
    return add_op(type, std::get<expression::value>(arg), rhs);
}

//------------------------------------------------------------------------------
parser::result<expression::value> expression_parser::parse_operand_explicit(token const*& tokens, token const* end)
{
    if (tokens >= end) {
        return error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    if (*tokens == '(') {
        result<expression::value> expr = parse_expression(++tokens, end);
        if (std::holds_alternative<error>(expr)) {
            return std::get<error>(expr);
        }
        if (tokens >= end) {
            return error{*(tokens - 1), "expected ')' after '" + *(tokens - 1) + "'"};
        } else if (*tokens != ')') {
            return error{*tokens, "expected ')' after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
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

    //} else if (tokens[0] == "pi") {
    //    ++tokens;
    //    return constant::pi;
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
            return error{*tokens, "unrecognized symbol: '" + *tokens + "'"};
        }
        ++tokens;
        return s->second;

    } else {
        return error{*tokens, "syntax error: '" + *tokens + "'"};
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
        return error{*(tokens - 1), "expected expression after '" + *(tokens - 1) + "'"};
    } else if (*tokens == ')' || *tokens == ',') {
        return error{*tokens, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
    }

    expression::value out;
    bool is_negative = false;

    // check for negation
    if (*tokens == '-') {
        is_negative = true;
        ++tokens;
    }

    result<expression::value> operand = parse_operand_explicit(tokens, end);
    if (std::holds_alternative<error>(operand)) {
        return std::get<error>(operand);
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
    if (std::holds_alternative<error>(lhs)) {
        return std::get<error>(lhs);
    }

    while (tokens < end && *tokens != ')' && *tokens != ',') {
        token const* saved = tokens;

        result<expression::op_type> lhs_op = parse_operator(tokens, end);
        if (std::holds_alternative<error>(lhs_op)) {
            return std::get<error>(lhs_op);
        }

        int lhs_precedence = op_precedence(std::get<expression::op_type>(lhs_op));
        if (precedence <= lhs_precedence) {
            tokens = saved;
            break;
        }

        result<expression::value> rhs = parse_expression(tokens, end, lhs_precedence);
        if (std::holds_alternative<error>(rhs)) {
            return std::get<error>(rhs);
        } else {
            lhs = add_op(std::get<expression::op_type>(lhs_op), std::get<expression::value>(lhs), std::get<expression::value>(rhs));
        }
    }

    return lhs;
}

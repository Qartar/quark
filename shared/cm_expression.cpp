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

        case op_type::logarithm:
            return std::log(lhs) / std::log(rhs);

        case op_type::sqrt:
            return std::sqrt(lhs);

        case op_type::sine:
            return std::sin(lhs);

        case op_type::cosine:
            return std::cos(lhs);

        case op_type::random:
            return r.uniform_real();

        case op_type::member:
            assert(false);
            return lhs;
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
expression_builder::expression_builder(string::view const* inputs, std::size_t num_inputs)
{
    add_builtin_types();

    for (std::size_t ii = 0; ii < num_inputs; ++ii) {
        _symbols[string::buffer(inputs[ii])] = {expression::value(ii), expression::type::scalar};
        _expression._ops.push_back({expression::op_type::none, 0, 0});
        _used.push_back(true); // assume inputs are always used
        _types.push_back(expression::type::scalar);
    }
    _expression._num_inputs = num_inputs;

    // hard-coded particles input structure
    if (num_inputs) {
        _types[0] = static_cast<expression::type>(3);
        _symbols["in"] = {0, _types[0]};
    }
}

//------------------------------------------------------------------------------
void expression_builder::add_builtin_types()
{
    _type_info.push_back({"scalar", 1, {}});
    _type_info.push_back({"vec2", 2, {
        {"x", expression::type::scalar, 0},
        {"y", expression::type::scalar, 1}}
    });
    _type_info.push_back({"vec4", 4, {
        {"x", expression::type::scalar, 0},
        {"y", expression::type::scalar, 1},
        {"z", expression::type::scalar, 2},
        {"w", expression::type::scalar, 3}}
    });

    // hard-coded particles input structure
    _type_info.push_back({"!in", 8, {
        {"pos", expression::type::vec2, 0},
        {"vel", expression::type::vec2, 2},
        {"dir", expression::type::vec2, 4},
        {"str", expression::type::scalar, 6},
        {"idx", expression::type::scalar, 7},
    }});
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
expression::type_value expression_builder::add_constant(float value)
{
    auto it = _constants.find(value);
    if (it != _constants.cend()) {
        return {it->second, _types[it->second]};
    } else {
        _expression._ops.push_back({expression::op_type::constant, 0, 0});
        _expression._constants.push_back(value);
        _constants[value] = _expression._ops.size() - 1;
        _used.push_back(false);
        _types.push_back(expression::type::scalar);
        return {narrow_cast<expression::value>(_expression._ops.size() - 1), expression::type::scalar};
    }
}

//------------------------------------------------------------------------------
expression::value expression_builder::alloc_type(expression::type type)
{
    type_info const& info = _type_info[static_cast<std::size_t>(type)];
    expression::value base = _expression._ops.size();
    if (info.fields.size()) {
        // sub-allocate child types
        for (auto field : info.fields) {
            alloc_type(field.type);
        }
    } else {
        for (std::size_t ii = 0; ii < info.size; ++ii) {
            _expression._ops.push_back({expression::op_type::none, 0, 0});
            _expression._constants.push_back(0);
            _used.push_back(false);
            _types.push_back(expression::type::scalar);
        }
    }
    assert(_expression._ops.size() - base == info.size);
    _types[base] = type;
    return base;
}

//------------------------------------------------------------------------------
expression::value expression_builder::alloc_type(expression::type type, expression::type_value const* values, std::size_t num_values)
{
    type_info const& info = _type_info[static_cast<std::size_t>(type)];
    expression::value base = _expression._ops.size();
    assert(num_values == info.size);
    if (info.fields.size()) {
        // sub-allocate child types
        for (auto field : info.fields) {
            alloc_type(field.type, values + field.offset, _type_info[static_cast<std::size_t>(field.type)].size);
        }
    } else {
        for (std::size_t ii = 0; ii < info.size; ++ii) {
            _expression._ops.push_back(_expression._ops[values[ii].value]);
            _expression._constants.push_back(_expression._constants[values[ii].value]);
            _used.push_back(_used[values[ii].value]);
            _types.push_back(expression::type::scalar);
        }
    }
    assert(_expression._ops.size() - base == info.size);
    _types[base] = type;
    return base;
}

//------------------------------------------------------------------------------
bool expression_builder::is_valid_op(expression::op_type type, expression::type lhs, expression::type rhs) const
{
    return (is_unary(type) || rhs == expression::type::scalar)
        || (is_binary(type) && (lhs == expression::type::scalar || lhs == rhs));
}

//------------------------------------------------------------------------------
expression::type_value expression_builder::add_op(expression::op_type type, expression::type_value lhs, expression::type_value rhs)
{
    if (is_nullary(type)) {
        _expression._constants.push_back(0.f);
        _expression._ops.push_back({type, lhs.value, rhs.value});
        _used.push_back(false);
        _types.push_back(expression::type::scalar);
        return {narrow_cast<expression::value>(_expression._ops.size() - 1), expression::type::scalar};
    } else if (is_constant(lhs) && is_constant(rhs)) {
        static random r; // not used
        return add_constant(
            expression::evaluate_op(
                r,
                type,
                _expression._constants[lhs.value - _expression._num_inputs],
                _expression._constants[rhs.value - _expression._num_inputs]));
    } else if (is_unary(type) || rhs.type == expression::type::scalar) {
        type_info const& lhs_type_info = _type_info[static_cast<std::size_t>(lhs.type)];
        expression::value base = alloc_type(lhs.type);
        for (std::size_t ii = 0; ii < lhs_type_info.size; ++ii) {
            _expression._ops[base + ii] = {type, expression::value(lhs.value + ii), rhs.value};
        }
        return {base, _types[base]};
    } else if (is_binary(type)) {
        if (lhs.type == expression::type::scalar) {
            type_info const& rhs_type_info = _type_info[static_cast<std::size_t>(rhs.type)];
            expression::value base = alloc_type(rhs.type);
            for (std::size_t ii = 0; ii < rhs_type_info.size; ++ii) {
                _expression._ops[base + ii] = {type, lhs.value, expression::value(rhs.value + ii)};
            }
            return {base, _types[base]};
        } else if (lhs.type == rhs.type) {
            type_info const& lhs_type_info = _type_info[static_cast<std::size_t>(lhs.type)];
            expression::value base = alloc_type(lhs.type);
            for (std::size_t ii = 0; ii < lhs_type_info.size; ++ii) {
                _expression._ops[base + ii] = {type, expression::value(lhs.value + ii), expression::value(rhs.value + ii)};
            }
            return {base, _types[base]};
        } else {
            // fail bad no
            assert(false);
            return {0, expression::type::scalar};
        }
    }
    assert(false);
    return {0, expression::type::scalar};
}

//------------------------------------------------------------------------------
bool expression_builder::is_constant(expression::value value) const
{
    return is_constant({value, _types[value]});
}

//------------------------------------------------------------------------------
bool expression_builder::is_constant(expression::type_value value) const
{
    type_info const& info = _type_info[static_cast<std::size_t>(value.type)];
    if (info.fields.empty()) {
        assert(info.size == 1);
        return _expression._ops[value.value].type == expression::op_type::constant;
    } else {
        for (auto const& field : info.fields) {
            if (!is_constant({expression::value(value.value + field.offset), field.type})) {
                return false;
            }
        }
        return true;
    }
}

//------------------------------------------------------------------------------
bool expression_builder::is_random(expression::value value) const
{
    return is_random({value, _types[value]});
}

//------------------------------------------------------------------------------
bool expression_builder::is_random(expression::type_value value) const
{
    type_info const& info = _type_info[static_cast<std::size_t>(value.type)];
    if (info.fields.empty()) {
        assert(info.size == 1);
        expression::op const& op = _expression._ops[value.value];
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
    } else {
        for (auto const& field : info.fields) {
            if (is_random({expression::value(value.value + field.offset), field.type})) {
                return true;
            }
        }
        return false;
    }
}

//------------------------------------------------------------------------------
void expression_builder::mark_used(expression::value value)
{
    return mark_used({value, _types[value]});
}

//------------------------------------------------------------------------------
void expression_builder::mark_used(expression::type_value value)
{
    type_info const& info = _type_info[static_cast<std::size_t>(value.type)];
    if (info.fields.empty()) {
        assert(info.size == 1);

        // no need to mark operands if already marked as used
        if (_used[value.value]) {
            return;
        }

        _used[value.value] = true;

        expression::op const& op = _expression._ops[value.value];
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
    } else {
        for (auto const& field : info.fields) {
            mark_used({expression::value(value.value + field.offset), field.type});
        }
    }
}

//------------------------------------------------------------------------------
std::size_t expression_builder::type_size(expression::type type) const
{
    assert(static_cast<std::size_t>(type) < _type_info.size());
    return _type_info[static_cast<std::size_t>(type)].size;
}

////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
expression_parser::expression_parser(string::view const* inputs, std::size_t num_inputs)
    : expression_builder(inputs, num_inputs)
{
}

//------------------------------------------------------------------------------
void expression_parser::assign(string::view name, expression::type_value value)
{
    //if (_symbols.find(name) != _symbols.cend()) {
    //    //log::warning("
    //} else {
        _symbols[string::buffer(name)] = value;
    //}
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
    } else if (token == '.') {
        return expression::op_type::member;
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
        return context.set_error(std::get<parser::error>(result));
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
    } else if (token == '.') {
        return expression::op_type::member;
    } else {
        return context.set_error({token, "expected operator, found '" + token + "'"});
    }
}

//------------------------------------------------------------------------------
parser::result<std::monostate> expression_parser::parse_arguments(parser::context& context, parser::token fn, expression::type_value* arguments, std::size_t num_arguments)
{
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        return context.set_error({std::get<parser::token>(result), "expected '(' after '" + fn + "'"});
    } else if (std::get<parser::token>(result) != '(') {
        return context.set_error({std::get<parser::token>(result), "expected '(' after '" + fn + "', found '" + std::get<parser::token>(result) + "'"});
    }

    for (int ii = 0; ii < num_arguments; ++ii) {
        auto arg = parse_expression(context, INT_MAX);
        if (std::holds_alternative<parser::error>(arg)) {
            return std::get<parser::error>(arg);
        } else {
            arguments[ii] = std::get<expression::type_value>(arg);
        }

        auto next = context.peek_token();
        if (!std::holds_alternative<parser::token>(next)) {
            return context.set_error({std::get<parser::token>(result), "expected token"});
        }

        if (ii < num_arguments - 1 && std::get<parser::token>(next) != ",") {
            return context.set_error({std::get<parser::token>(next), "insufficient arguments for function '" + fn + "'"});
        } else if (ii == num_arguments - 1 && std::get<parser::token>(next) == ",") {
            return context.set_error({std::get<parser::token>(next), "too many arguments for function '" + fn + "'"});
        } else if (ii == num_arguments - 1 && std::get<parser::token>(next) != ")") {
            return context.set_error({std::get<parser::token>(next), "expected ')', found '" + std::get<parser::token>(next) + "'"});
        } else {
            context.next_token(); // consume token
        }
    }

    return std::monostate();
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_unary_function(parser::context& context, parser::token fn, expression::op_type type, expression::type_value rhs)
{
    expression::type_value arguments[1];
    auto result = parse_arguments(context, fn, arguments);
    if (std::holds_alternative<parser::error>(result)) {
        return std::get<parser::error>(result);
    }
    return add_op(type, arguments[0], rhs);
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_binary_function(parser::context& context, parser::token fn, expression::op_type type)
{
    expression::type_value arguments[2];
    auto result = parse_arguments(context, fn, arguments);
    if (std::holds_alternative<parser::error>(result)) {
        return std::get<parser::error>(result);
    }
    return add_op(type, arguments[0], arguments[1]);
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_operand_explicit(parser::context& context)
{
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        // TODO: "expected expression ..."
        return context.set_error(std::get<parser::error>(result));
    }

    auto token = std::get<parser::token>(result);
    if (token == ')' || token == ',' || token == ';') {
        //context.set_error();
        //return parser::error{token, "expected expression after '" + *(tokens - 1) + "', found '" + *tokens + "'"};
        return context.get_error();
    }

    if (token == '(') {
        parser::result<expression::type_value> expr = parse_expression(context);
        if (std::holds_alternative<parser::error>(expr)) {
            return context.set_error(std::get<parser::error>(expr));
        }
        if (!context.expect_token(")")) {
            return context.get_error();
        } else {
            return std::get<expression::type_value>(expr);
        }

    //
    //  functions
    //

    } else if (token == "random") {
        return add_op(expression::op_type::random, {}, {});
    } else if (token == "ln") {
        return parse_unary_function(context, token, expression::op_type::logarithm, add_constant(math::e<float>));
    } else if (token == "log") {
        return parse_binary_function(context, token, expression::op_type::logarithm);
    } else if (token == "sqrt") {
        return parse_unary_function(context, token, expression::op_type::sqrt);
    } else if (token == "sin") {
        return parse_unary_function(context, token, expression::op_type::sine);
    } else if (token == "cos") {
        return parse_unary_function(context, token, expression::op_type::cosine);
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
        expression::type_value v = add_constant((float)std::atof(token.begin));
        return v;

    //
    //  types and symbols
    //

    } else if (*token.begin >= 'a' && *token.begin <= 'z' || *token.begin >= 'A' && *token.begin <= 'Z') {
        auto t = std::find_if(_type_info.begin(), _type_info.end(), [token](type_info const& info) {
            return token == info.name;
        });
        if (t != _type_info.cend()) {
            auto type = static_cast<expression::type>(std::distance(_type_info.begin(), t));
            std::vector<expression::type_value> arguments(t->size);
            auto result2 = parse_arguments(context, token, arguments.data(), arguments.size());
            if (!std::holds_alternative<parser::error>(result2)) {
                return expression::type_value{alloc_type(type, arguments.data(), arguments.size()), type};
            } else {
                return std::get<parser::error>(result2);
            }
        }

        auto s = _symbols.find(string::view(token.begin, token.end));
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
        case expression::op_type::member: return 2;
        default: return 0;
    }
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_operand(parser::context& context)
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
#elif 1
    if (context.check_token("-")) {
        auto lhs = parse_operand_explicit(context);
        if (std::holds_alternative<parser::error>(lhs)) {
            return context.set_error(std::get<parser::error>(lhs));
        } else {
            return add_op(expression::op_type::negative, std::get<expression::type_value>(lhs), {});
        }
    } else {
        return parse_operand_explicit(context);
    }
#else
    return parse_operand_explicit(context);
#endif
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_member(parser::context& context, expression::type_value lhs)
{
    auto result = context.next_token();
    if (std::holds_alternative<parser::error>(result)) {
        return std::get<parser::error>(result);
    }

    auto const& type = _type_info[static_cast<size_t>(lhs.type)];
    auto name = std::get<parser::token>(result);
    for (std::size_t ii = 0; ii < type.fields.size(); ++ii) {
        if (name == type.fields[ii].name) {
            return expression::type_value{expression::value(lhs.value + type.fields[ii].offset), type.fields[ii].type};
        }
    }

    return context.set_error({name, "" + parser::token{} + "'" + type.name + "' has no member '" + name + "'"});
}

//------------------------------------------------------------------------------
parser::result<expression::type_value> expression_parser::parse_expression(parser::context& context, int precedence)
{
    parser::result<expression::type_value> lhs = parse_operand(context);
    if (std::holds_alternative<parser::error>(lhs)) {
        return context.set_error(std::get<parser::error>(lhs));
    }

    while (context.has_token()
        && !context.peek_token(")")
        && !context.peek_token(",")
        && !context.peek_token(";")) {


        parser::result<expression::op_type> lhs_op = peek_operator(context);
        if (std::holds_alternative<parser::error>(lhs_op)) {
            return context.set_error(std::get<parser::error>(lhs_op));
        }

        auto lhs_op_token = context.peek_token();
        int lhs_precedence = op_precedence(std::get<expression::op_type>(lhs_op));
        if (precedence <= lhs_precedence) {
            break;
        } else {
            lhs_op = parse_operator(context);
            assert(!std::holds_alternative<parser::error>(lhs_op));
        }

        if (std::get<expression::op_type>(lhs_op) == expression::op_type::member) {
            parser::result<expression::type_value> rhs = parse_member(context, std::get<expression::type_value>(lhs));
            if (std::holds_alternative<parser::error>(rhs)) {
                return std::get<parser::error>(rhs);
            } else {
                lhs = rhs;
                continue;
            }
        }

        parser::result<expression::type_value> rhs = parse_expression(context, lhs_precedence);
        if (std::holds_alternative<parser::error>(rhs)) {
            return context.set_error(std::get<parser::error>(rhs));
        } else if (!is_valid_op(std::get<expression::op_type>(lhs_op),
                                std::get<expression::type_value>(lhs).type,
                                std::get<expression::type_value>(rhs).type)) {
            auto lhs_typename = _type_info[static_cast<std::size_t>(std::get<expression::type_value>(lhs).type)].name;
            auto rhs_typename = _type_info[static_cast<std::size_t>(std::get<expression::type_value>(rhs).type)].name;
            return context.set_error({std::get<parser::token>(lhs_op_token), "" + parser::token{} + "invalid operator '" + lhs_typename + " " + std::get<parser::token>(lhs_op_token) + " " + rhs_typename + "'"});
        } else {
            lhs = add_op(std::get<expression::op_type>(lhs_op),
                         std::get<expression::type_value>(lhs),
                         std::get<expression::type_value>(rhs));
        }
    }

    return lhs;
}

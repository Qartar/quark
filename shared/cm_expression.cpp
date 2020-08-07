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

////////////////////////////////////////////////////////////////////////////////
#include <set>
#include "cm_filesystem.h"
//------------------------------------------------------------------------------
class dgml {
public:
    void add_node(string::view id, string::view label) {
        _nodes[string::buffer(id)] = string::buffer(label);
    }
    void add_link(string::view source_id, string::view target_id) {
        _links.insert({string::buffer(source_id), string::buffer(target_id)});
    }
    bool write(file::stream& s) {
        s.printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
        s.printf("<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\">\n");
        // write nodes
        s.printf("  <Nodes>\n");
        for (auto const& node : _nodes) {
            s.printf("    <Node Id=\"%s\" Label=\"%s\"/>\n", node.first.c_str(), node.second.c_str());
        }
        s.printf("  </Nodes>\n");
        // write links
        s.printf("  <Links>\n");
        for (auto const& link : _links) {
            s.printf("    <Link Source=\"%s\" Target=\"%s\"/>\n", link.source.c_str(), link.target.c_str());
        }
        s.printf("  </Links>\n");
        s.printf("</DirectedGraph>");
        return true;
    }
protected:
    struct link {
        string::buffer source;
        string::buffer target;
        bool operator<(link const& l) const {
            int c0 = strcmp(source, l.source);
            return c0 < 0 || (c0 == 0 && strcmp(target, l.target) < 0);
        }
    };
    std::map<string::buffer, string::buffer> _nodes;
    std::set<link> _links;
};

//------------------------------------------------------------------------------
expression expression_builder::compile(expression::value* map) const
{
    expression out;
    std::size_t num_used = 0;

    std::vector<int> num_references(_expression._ops.size(), 0);
    for (std::size_t ii = 0, sz = _expression._ops.size(); ii < sz; ++ii) {
        if (_used[ii]) {
            ++num_references[ii];
        }
        switch (expression::arity(_expression._ops[ii].type)) {
            case expression::op_arity::binary:
                ++num_references[_expression._ops[ii].rhs];
                [[fallthrough]];
            case expression::op_arity::unary:
                ++num_references[_expression._ops[ii].lhs];
                [[fallthrough]];
            case expression::op_arity::nullary:
                break;
        }
    }

    {
        std::size_t num_folded = 0;
        std::size_t num_merged = 0;
        std::size_t num_removed = 0;

        struct op_info {
            string::view symbol;
            expression::op op;
            float constant;
            int num_references;
        };

        auto dump_ast = [](std::vector<op_info> const& info, int n) {
            static int num_random = 0;
            dgml ast;
            std::vector<string::buffer> names(info.size());
            for (std::size_t ii = 0, sz = info.size(); ii < sz; ++ii) {
                if (info[ii].symbol != "") {
                    names[ii] = string::buffer(info[ii].symbol);
                } else if (info[ii].op.type == expression::op_type::constant) {
                    names[ii] = string::buffer(va("%0g", info[ii].constant));
                }
            }
            for (std::size_t ii = 0, sz = info.size(); ii < sz; ++ii) {
                if (info[ii].symbol == "") {
                    switch (info[ii].op.type) {
                        case expression::op_type::constant:
                            names[ii] = string::buffer(va("%0g", info[ii].constant));
                            break;

                        case expression::op_type::random:
                            names[ii] = string::buffer(va("random_%d", ++num_random));
                            break;

                        case expression::op_type::negative:
                            names[ii] = string::buffer(va("-(%s)", names[info[ii].op.lhs].c_str()));
                            break;

                        case expression::op_type::sqrt:
                            names[ii] = string::buffer(va("sqrt(%s)", names[info[ii].op.lhs].c_str()));
                            break;

                        case expression::op_type::sine:
                            names[ii] = string::buffer(va("sin(%s)", names[info[ii].op.lhs].c_str()));
                            break;

                        case expression::op_type::cosine:
                            names[ii] = string::buffer(va("cos(%s)", names[info[ii].op.lhs].c_str()));
                            break;

                        case expression::op_type::sum:
                            names[ii] = string::buffer(va("(%s)+(%s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        case expression::op_type::difference:
                            names[ii] = string::buffer(va("(%s)-(%s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        case expression::op_type::product:
                            names[ii] = string::buffer(va("(%s)*(%s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        case expression::op_type::quotient:
                            names[ii] = string::buffer(va("(%s)/(%s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        case expression::op_type::exponent:
                            names[ii] = string::buffer(va("(%s)^(%s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        case expression::op_type::logarithm:
                            names[ii] = string::buffer(va("log(%s, %s)", names[info[ii].op.lhs].c_str(), names[info[ii].op.rhs].c_str()));
                            break;

                        default:
                            break;
                    }
                }
            }
            for (std::size_t ii = 0, sz = info.size(); ii < sz; ++ii) {
                switch(expression::arity(info[ii].op.type)) {
                    case expression::op_arity::binary:
                        ast.add_link(va("%zd", ii), va("%zd", info[ii].op.rhs));
                        [[fallthrough]];
                    case expression::op_arity::unary:
                        ast.add_link(va("%zd", ii), va("%zd", info[ii].op.lhs));
                        [[fallthrough]];
                    case expression::op_arity::nullary:
                        ast.add_node(va("%zd", ii), names[ii]);
                        break;
                }
            }
            file::stream s = file::open(va("ast_%02d.dgml", n), file::mode::write);
            ast.write(s);
        };

        auto rotate_left = [](std::vector<op_info>& info, expression::value root) -> bool {
            if (root < 0 || root >= narrow_cast<expression::value>(info.size()) || !is_binary(info[root].op.type)) {
                return false;
            }

            expression::value pivot = info[root].op.rhs;
            if (pivot < 0 || pivot >= narrow_cast<expression::value>(info.size()) || !is_binary(info[pivot].op.type)) {
                return false;
            }

            //   root              root
            //   /  \              /  \
            //  a  pivot   =>   pivot  c
            //      / \          / \
            //     b   c        a   b

            expression::value a = info[root].op.lhs;
            expression::value b = info[pivot].op.lhs;
            expression::value c = info[pivot].op.rhs;

            // If there are other references to pivot then create a copy, otherwise modify in place
            if (info[pivot].num_references > 1) {
                --info[pivot].num_references;
                info.push_back(info[pivot]);
                pivot = info.size() - 1;
                info[pivot].num_references = 1;
            }

            // (a + (b + c)) => ((a + b) + c)
            // (a * (b * c)) => ((a * b) * c)
            // (a ^ (b ^ c)) => ((a ^ b) ^ c)
            if ((info[root].op.type == expression::op_type::sum && info[pivot].op.type == expression::op_type::sum)
                || (info[root].op.type == expression::op_type::product && info[pivot].op.type == expression::op_type::product)
                || (info[root].op.type == expression::op_type::exponent && info[pivot].op.type == expression::op_type::exponent)) {
                info[pivot].op.lhs = a;
                info[pivot].op.rhs = b;
                info[root].op.lhs = pivot;
                info[root].op.rhs = c;
                return true;

            // (a + (b - c)) => ((a + b) - c)
            // (a * (b / c)) => ((a * b) / c)
            } else if ((info[root].op.type == expression::op_type::sum && info[pivot].op.type == expression::op_type::difference)
                || (info[root].op.type == expression::op_type::product && info[pivot].op.type == expression::op_type::quotient)) {
                std::swap(info[pivot].op.type, info[root].op.type);
                info[pivot].op.lhs = a;
                info[pivot].op.rhs = b;
                info[root].op.lhs = pivot;
                info[root].op.rhs = c;
                return true;

            // (a - (b + c)) => ((a - b) - c)
            // (a / (b * c)) => ((a / b) / c)
            } else if ((info[root].op.type == expression::op_type::difference && info[pivot].op.type == expression::op_type::sum)
                || (info[root].op.type == expression::op_type::quotient && info[pivot].op.type == expression::op_type::product)) {
                info[pivot].op.type = info[root].op.type;
                info[pivot].op.lhs = a;
                info[pivot].op.rhs = b;
                info[root].op.lhs = pivot;
                info[root].op.rhs = c;
                return true;

            // (a - (b - c)) => ((a - b) + c)
            } else if (info[root].op.type == expression::op_type::difference && info[pivot].op.type == expression::op_type::difference) {
                info[pivot].op.lhs = a;
                info[pivot].op.rhs = b;
                info[root].op.type = expression::op_type::sum;
                info[root].op.lhs = pivot;
                info[root].op.rhs = c;
                return true;

            // (a / (b / c)) => ((a / b) * c)
            } else if (info[root].op.type == expression::op_type::quotient && info[pivot].op.type == expression::op_type::quotient) {
                info[pivot].op.lhs = a;
                info[pivot].op.rhs = b;
                info[root].op.type = expression::op_type::product;
                info[root].op.lhs = pivot;
                info[root].op.rhs = c;
                return true;

            } else {
                // Undo copy
                if (pivot != info[root].op.rhs) {
                    assert(pivot == narrow_cast<expression::value>(info.size() - 1));
                    info.pop_back();
                    ++info[info[root].op.rhs].num_references;
                }
                return false;
            }
        };

        auto rotate_right = [](std::vector<op_info>& info, expression::value root) -> bool {
            if (root < 0 || root >= narrow_cast<expression::value>(info.size()) || !is_binary(info[root].op.type)) {
                return false;
            }

            expression::value pivot = info[root].op.lhs;
            if (pivot < 0 || pivot >= narrow_cast<expression::value>(info.size()) || !is_binary(info[pivot].op.type)) {
                return false;
            }

            //     root          root
            //     /  \          /  \
            //  pivot  c   =>   a  pivot
            //   / \                / \
            //  a   b              b   c

            expression::value a = info[pivot].op.lhs;
            expression::value b = info[pivot].op.rhs;
            expression::value c = info[root].op.rhs;

            // If there are other references to pivot then create a copy, otherwise modify in place
            if (info[pivot].num_references > 1) {
                --info[pivot].num_references;
                info.push_back(info[pivot]);
                pivot = info.size() - 1;
                info[pivot].num_references = 1;
            }

            // ((a + b) + c) => (a + (b + c))
            // ((a * b) * c) => (a * (b * c))
            // ((a ^ b) ^ c) => (a ^ (b ^ c))
            if ((info[pivot].op.type == expression::op_type::sum && info[root].op.type == expression::op_type::sum)
                || (info[pivot].op.type == expression::op_type::product && info[root].op.type == expression::op_type::product)
                || (info[pivot].op.type == expression::op_type::exponent && info[root].op.type == expression::op_type::exponent)) {
                info[root].op.lhs = a;
                info[root].op.rhs = pivot;
                info[pivot].op.lhs = b;
                info[pivot].op.rhs = c;
                return true;

            // ((a + b) - c) => (a + (b - c))
            // ((a * b) / c) => (a * (b / c))
            } else if ((info[pivot].op.type == expression::op_type::sum && info[root].op.type == expression::op_type::difference)
                || (info[pivot].op.type == expression::op_type::product && info[root].op.type == expression::op_type::quotient)) {
                std::swap(info[pivot].op.type, info[root].op.type);
                info[root].op.lhs = a;
                info[root].op.rhs = pivot;
                info[pivot].op.lhs = b;
                info[pivot].op.rhs = c;
                return true;

            // ((a - b) + c) => (a - (b - c))
            // ((a / b) * c) => (a / (b / c))
            } else if ((info[pivot].op.type == expression::op_type::difference && info[root].op.type == expression::op_type::sum)
                || (info[pivot].op.type == expression::op_type::quotient && info[root].op.type == expression::op_type::product)) {
                info[root].op.type = info[pivot].op.type;
                info[root].op.lhs = a;
                info[root].op.rhs = pivot;
                info[pivot].op.lhs = b;
                info[pivot].op.rhs = c;
                return true;

            // ((a - b) - c) => (a - (b + c))
            } else if (info[pivot].op.type == expression::op_type::difference && info[root].op.type == expression::op_type::difference) {
                info[root].op.lhs = a;
                info[root].op.rhs = pivot;
                info[pivot].op.type = expression::op_type::sum;
                info[pivot].op.lhs = b;
                info[pivot].op.rhs = c;
                return true;

            // ((a / b) / c) => (a / (b * c))
            } else if (info[pivot].op.type == expression::op_type::quotient && info[root].op.type == expression::op_type::quotient) {
                info[root].op.lhs = a;
                info[root].op.rhs = pivot;
                info[pivot].op.type = expression::op_type::product;
                info[pivot].op.lhs = b;
                info[pivot].op.rhs = c;
                return true;

            } else {
                // Undo copy
                if (pivot != info[root].op.lhs) {
                    assert(pivot == narrow_cast<expression::value>(info.size() - 1));
                    info.pop_back();
                    ++info[info[root].op.lhs].num_references;
                }
                return false;
            }
        };

        std::vector<op_info> info(_expression._ops.size());
        for (std::size_t ii = 0, sz = _expression._ops.size(); ii < sz; ++ii) {
            for (auto& kv : _symbols) {
                if (kv.second.value == expression::value(ii)) {
                    info[ii].symbol = kv.first;
                    break;
                }
            }
            info[ii].op = _expression._ops[ii];
            if (info[ii].op.type == expression::op_type::constant) {
                info[ii].constant = _expression._constants[ii - _expression.num_inputs()];
            } else {
                info[ii].constant = NAN;
            }
            info[ii].num_references = num_references[ii];
        }

        dump_ast(info, 0);

        // constant folding
        std::size_t old_folded;
        std::size_t num_rotated;
        do
        {
            old_folded = num_folded;
            num_rotated = 0;

            random r;
            for (std::size_t ii = 0, sz = info.size(); ii < sz; ++ii) {
                bool is_constant = true;
                bool has_constant = false;
                if (expression::arity(info[ii].op.type) == expression::op_arity::nullary) {
                    continue;
                }
                switch (expression::arity(info[ii].op.type)) {
                    case expression::op_arity::binary:
                        is_constant &= info[info[ii].op.rhs].op.type == expression::op_type::constant;
                        has_constant |= info[info[ii].op.rhs].op.type == expression::op_type::constant;
                        [[fallthrough]];
                    case expression::op_arity::unary:
                        is_constant &= info[info[ii].op.lhs].op.type == expression::op_type::constant;
                        has_constant |= info[info[ii].op.lhs].op.type == expression::op_type::constant;
                        [[fallthrough]];
                    case expression::op_arity::nullary:
                        break;
                }
                // update in-place
                if (is_constant) {
                    ++num_folded;
                    info[ii].constant = expression::evaluate_op(
                        r,
                        info[ii].op.type,
                        info[info[ii].op.lhs].constant,
                        info[info[ii].op.rhs].constant);
                    switch (expression::arity(info[ii].op.type)) {
                        case expression::op_arity::binary:
                            assert(info[info[ii].op.rhs].num_references);
                            --info[info[ii].op.rhs].num_references;
                            [[fallthrough]];
                        case expression::op_arity::unary:
                            assert(info[info[ii].op.lhs].num_references);
                            --info[info[ii].op.lhs].num_references;
                            [[fallthrough]];
                        case expression::op_arity::nullary:
                            break;
                    }
                    info[ii].op.type = expression::op_type::constant;
                } else if (has_constant) {
                    if (info[info[ii].op.lhs].op.type == expression::op_type::constant) {
                        expression::value pivot = info[ii].op.rhs;
                        if (is_binary(info[pivot].op.type) && info[info[pivot].op.lhs].op.type == expression::op_type::constant) {
                            // try to rotate expression to put constant operands together
                            if (rotate_left(info, ii)) {
                                // re-evaluate current operation
                                ii = min<std::size_t>(ii, pivot) - 1;
                                ++num_rotated;
                                continue;
                            }
                        }
                    } else if (info[info[ii].op.rhs].op.type == expression::op_type::constant) {
                        expression::value pivot = info[ii].op.lhs;
                        if (is_binary(info[pivot].op.type) && info[info[pivot].op.rhs].op.type == expression::op_type::constant) {
                            // try to rotate expression to put constant operands together
                            if (rotate_right(info, ii)) {
                                // re-evaluate current operation
                                ii = min<std::size_t>(ii, pivot) - 1;
                                ++num_rotated;
                                continue;
                            }
                        }
                    }
                }
            }
        } while (num_rotated || num_folded > old_folded);

        dump_ast(info, 1);

        // common sub-expression elimination
        std::size_t old_merged;
        do
        {
            old_merged = num_merged;
            for (std::size_t ii = 0, sz = info.size(); ii < sz; ++ii) {
                for (std::size_t jj = ii + 1; jj < sz; ++jj) {
                    if (info[ii].op.type == info[jj].op.type) {
                        bool can_merge = false;
                        if (info[ii].op.type == expression::op_type::constant && info[ii].constant == info[jj].constant) {
                            can_merge = true;
                        } else if (expression::arity(info[ii].op.type) == expression::op_arity::binary) {
                            if (info[ii].op.lhs == info[jj].op.lhs && info[ii].op.rhs == info[jj].op.rhs) {
                                can_merge = true;
                            }
                        } else if (expression::arity(info[ii].op.type) == expression::op_arity::unary) {
                            if (info[ii].op.lhs == info[jj].op.lhs) {
                                can_merge = true;
                            }
                        }
                        if (can_merge) {
                            // need to check all ops, not just (jj,num_ops), since
                            // constant folding can introduce out-of-order constants
                            for (std::size_t kk = 0; kk < sz; ++kk) {
                                switch (expression::arity(info[kk].op.type)) {
                                    case expression::op_arity::binary:
                                        if (info[kk].op.rhs == expression::value(jj)) {
                                            info[kk].op.rhs = ii;
                                            ++info[ii].num_references;
                                            assert(info[jj].num_references);
                                            --info[jj].num_references;
                                            ++num_merged;
                                        }
                                        [[fallthrough]];
                                    case expression::op_arity::unary:
                                        if (info[kk].op.lhs == expression::value(jj)) {
                                            info[kk].op.lhs = ii;
                                            ++info[ii].num_references;
                                            assert(info[jj].num_references);
                                            --info[jj].num_references;
                                            ++num_merged;
                                        }
                                        [[fallthrough]];
                                    case expression::op_arity::nullary:
                                        break;
                                }
                            }
                        }
                    }
                }
            }
        } while (num_merged > old_merged);

        dump_ast(info, 2);

        // dead code elimination
        {
            for (std::size_t ii = info.size(); ii > 0; --ii) {
                if (!info[ii - 1].num_references) {
                    auto const& op = info[ii - 1].op;
                    switch (expression::arity(op.type)) {
                        case expression::op_arity::binary:
                            assert (info[op.rhs].num_references);
                            --info[op.rhs].num_references;
                            ii = std::max<std::size_t>(ii, op.rhs + 1);
                            [[fallthrough]];
                        case expression::op_arity::unary:
                            assert (info[op.lhs].num_references);
                            --info[op.lhs].num_references;
                            ii = std::max<std::size_t>(ii, op.lhs + 1);
                            [[fallthrough]];
                        case expression::op_arity::nullary:
                            break;
                    }
                    if (info[ii - 1].op.type != expression::op_type::none) {
                        info[ii - 1].op.type = expression::op_type::none;
                        ++num_removed;
                    }
                }
            }
        }

        dump_ast(info, 3);

        num_folded = num_folded;
        num_merged = num_merged;
        num_removed = num_removed;
    }

    // always use all inputs, even if they aren't referenced
    out._num_inputs = _expression._num_inputs;
    for (std::size_t ii = 0, sz = _expression.num_inputs(); ii < sz; ++ii) {
        map[ii] = num_used++;
    }

    // make sure all constants are placed in front of the calculated values
    for (std::size_t ii = _expression.num_inputs(), sz = _used.size(); ii < sz; ++ii) {
        if (num_references[ii] && _expression._ops[ii].type == expression::op_type::constant) {
            out._constants.push_back(_expression._constants[ii - _expression.num_inputs()]);
            map[ii] = num_used++;
        }
    }

    for (std::size_t ii = _expression.num_inputs(), sz = _used.size(); ii < sz; ++ii) {
        if (num_references[ii] && _expression._ops[ii].type != expression::op_type::constant) {
            map[ii] = num_used++;
        }
    }

    out._ops.resize(num_used);
    for (std::size_t ii = 0, sz = num_references.size(); ii < sz; ++ii) {
        if (num_references[ii]) {
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
        return it->second;
    } else {
        _expression._ops.push_back({expression::op_type::constant, 0, 0});
        _expression._constants.push_back(value);
        _constants[value] = {expression::value(_expression._ops.size() - 1), expression::type::scalar};
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

#pragma once

#include <Halide.h>

#include "Util.h"

namespace Halide {
namespace Element {

//
// Inline reduction
//
namespace {
using namespace Halide::Internal;
class FindFreeVars : public IRMutator {
public:
    std::vector<Var> free_vars;
    std::vector<Expr> call_args;
    RDom rdom;

    FindFreeVars(RDom r) : rdom(r) { }

private:
    Scope<int> internal;

    using IRMutator::visit;

    void visit(const Let *op) {
        Expr value = mutate(op->value);
        internal.push(op->name, 0);
        Expr body = mutate(op->body);
        internal.pop(op->name);
        if (value.same_as(op->value) &&
            body.same_as(op->body)) {
            expr = op;
        } else {
            expr = Let::make(op->name, value, body);
        }
    }

    void visit(const Variable *v) {

        std::string var_name = v->name;
        expr = v;

        if (internal.contains(var_name)) {
            // Don't capture internally defined vars
            return;
        }

        if (v->reduction_domain.defined()) {
            if (v->reduction_domain.same_as(rdom.domain())) {
                // This variable belongs to the explicit reduction domain, so
                // skip it.
                return;
            } else {
                // This should be converted to a pure variable and
                // added to the free vars list.
                var_name = unique_name('v');
                expr = Variable::make(v->type, var_name);
            }
        }

        if (v->param.defined()) {
            // Skip parameters
            return;
        }

        for (size_t i = 0; i < free_vars.size(); i++) {
            if (var_name == free_vars[i].name()) return;
        }

        free_vars.push_back(Var(var_name));
        call_args.push_back(v);
    }
};

Expr sum_unroll(RDom r, Expr e, const std::string& name = "sum_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    throw_assert(v.rdom.defined(), "Expression passed to sum must reference a reduction domain");

    Func f(name);
    f(v.free_vars) += e;
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

Expr product_unroll(RDom r, Expr e, const std::string &name = "produce_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    throw_assert(v.rdom.defined(), "Expression passed to product must reference a reduction domain");

    Func f(name);
    f(v.free_vars) *= e;
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

Expr maximum_unroll(RDom r, Expr e, const std::string &name = "maximum_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    throw_assert(v.rdom.defined(), "Expression passed to maximum must reference a reduction domain");

    Func f(name);
    f(v.free_vars) = e.type().min();
    f(v.free_vars) = max(f(v.free_vars), e);
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

Expr minimum_unroll(RDom r, Expr e, const std::string &name = "minimum_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    throw_assert(v.rdom.defined(), "Expression passed to minimum must reference a reduction domain");

    Func f(name);
    f(v.free_vars) = e.type().max();
    f(v.free_vars) = min(f(v.free_vars), e);
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

Tuple argmax_unroll(RDom r, Expr e, const std::string &name = "argmax_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    Func f(name);

    throw_assert(v.rdom.defined(), "Expression passed to argmax must reference a reduction domain");

    Tuple initial_tup(std::vector<Expr>(v.rdom.dimensions()+1));
    Tuple update_tup(std::vector<Expr>(v.rdom.dimensions()+1));
    for (int i = 0; i < v.rdom.dimensions(); i++) {
        initial_tup[i] = 0;
        update_tup[i] = v.rdom[i];
    }
    int value_index = (int)initial_tup.size()-1;
    initial_tup[value_index] = e.type().min();
    update_tup[value_index] = e;

    f(v.free_vars) = initial_tup;
    Expr better = e > f(v.free_vars)[value_index];
    Tuple update = tuple_select(better, update_tup, f(v.free_vars));
    f(v.free_vars) = update;
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

Tuple argmin_unroll(RDom r, Expr e, const std::string &name = "argmin_unroll") {
    FindFreeVars v(r);
    e = v.mutate(common_subexpression_elimination(e));

    Func f(name);

    throw_assert(v.rdom.defined(), "Expression passed to argmin must reference a reduction domain");

    Tuple initial_tup(std::vector<Expr>(v.rdom.dimensions()+1));
    Tuple update_tup(std::vector<Expr>(v.rdom.dimensions()+1));
    for (int i = 0; i < v.rdom.dimensions(); i++) {
        initial_tup[i] = 0;
        update_tup[i] = v.rdom[i];
    }
    int value_index = (int)initial_tup.size()-1;
    initial_tup[value_index] = e.type().max();
    update_tup[value_index] = e;

    f(v.free_vars) = initial_tup;
    Expr better = e < f(v.free_vars)[value_index];
    f(v.free_vars) = tuple_select(better, update_tup, f(v.free_vars));
#if defined(HALIDE_FOR_FPGA)
    for (auto var : v.free_vars) {
        f.unroll(var)
         .update().unroll(var);
    }
    for (int i=0; i<r.dimensions(); i++) {
        f.update().unroll(r[i]);
    }
#endif
    return f(v.call_args);
}

} //namespace anonymous
} //namespace Halide
} //namespace Element

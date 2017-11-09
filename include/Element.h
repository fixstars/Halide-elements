#ifndef HALIDE_ELEMENTS_H
#define HALIDE_ELEMENTS_H

#include <cassert>
#include <string>
#include <vector>
#include <exception>

#include <Halide.h>

#include "Arithmetic.h"

namespace Halide {
namespace Element {

void throw_assert(bool condition, const char *msg) 
{
    if (!condition) {
        throw std::runtime_error(msg);
    }
}

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
}

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


//
// Fixed point
//
template<typename T>
struct Upper;

template<>
struct Upper<int16_t> {
    using type = int32_t;
};

template<>
struct Upper<int32_t> {
    using type = int64_t;
};

template<>
struct Upper<float> {
    using type = float;
};

template<>
struct Upper<double> {
    using type = double;
};

template<typename BASE_T, uint32_t FB>
struct Fixed {
    Expr v;
    using upper_t = typename Upper<BASE_T>::type;

    explicit operator Expr() const {
        return v;
    }
};

template<typename BASE_T, uint32_t FB>
inline Fixed<BASE_T, FB> to_fixed(Expr x)
{
    throw_assert(x.defined(), "conversion of undefined Expr");
    if (x.type().is_float()) {
        return Fixed<BASE_T, FB>{cast<BASE_T>(x * make_const(type_of<BASE_T>(), 1<<FB))};
    } else {
        return Fixed<BASE_T, FB>{cast<BASE_T>(x) << FB};
    }
}

template<typename T, typename BASE_T, uint32_t FB>
inline Expr from_fixed(const Fixed<BASE_T, FB>& x)
{
    throw_assert(x.v.defined(), "conversion of undefined Expr");
    if (type_of<T>().is_float()) {
        return cast<T>(x.v) / cast<T>(1 << FB);
    } else {
        return cast<T>(x.v >> FB);
    }
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator-(const Fixed<BASE_T, FB>& x)
{
    return Fixed<BASE_T, FB>{-x.v};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator+(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return Fixed<BASE_T, FB>{x.v + y.v};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator-(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return Fixed<BASE_T, FB>{x.v - y.v};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator*(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>((cast<UPPER_T>(x.v) * cast<UPPER_T>(y.v)) >> FB)};
    return Fixed<BASE_T, FB>{1024};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator/(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>((cast<UPPER_T>(x.v) << FB) / cast<UPPER_T>(y.v))};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> operator<<(const Fixed<BASE_T, FB>& x, BASE_T y)
{
    return Fixed<BASE_T, FB>{x.v << y};
}

template<typename BASE_T, uint32_t FB>
Expr operator==(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v == y.v;
}

template<typename BASE_T, uint32_t FB>
Expr operator!=(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v != y.v;
}

template<typename BASE_T, uint32_t FB>
Expr operator<(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v < y.v;
}

template<typename BASE_T, uint32_t FB>
Expr operator<=(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v <= y.v;
}

template<typename BASE_T, uint32_t FB>
Expr operator>(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v > y.v;
}

template<typename BASE_T, uint32_t FB>
Expr operator>=(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return x.v >= y.v;
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> floor(const Fixed<BASE_T, FB>& x)
{
    constexpr BASE_T mask = ~((0x1 << FB) - 1);
    Fixed<BASE_T, FB> zero = to_fixed<BASE_T, FB>(.0f);
    Fixed<BASE_T, FB> one = to_fixed<BASE_T, FB>(1.0f);
    return Fixed<BASE_T, FB>{select(x >= zero, x.v, (x - one).v + 1) & mask};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> min(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return Fixed<BASE_T, FB>{min(x.v, y.v)};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> max(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    return Fixed<BASE_T, FB>{max(x.v, y.v)};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> clamp(const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& min_val, const Fixed<BASE_T, FB>& max_val)
{
    return Fixed<BASE_T, FB>{clamp(x.v, min_val.v, max_val.v)};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> select(Expr condition, const Fixed<BASE_T, FB>& true_value, const Fixed<BASE_T, FB>& false_value)
{
    return Fixed<BASE_T, FB>{select(condition, true_value.v, false_value.v)};
}

template<typename BASE_T, uint32_t FB, typename... Args>
Fixed<BASE_T, FB> select(Expr c0, const Fixed<BASE_T, FB>& v0, Expr c1, const Fixed<BASE_T, FB>& v1, Args&&... args)
{
    return Fixed<BASE_T, FB>{select(c0, static_cast<Expr>(v0), static_cast<Expr>(select(c1, v1, std::forward<Args>(args)...)))};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> mac(RDom r, const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>(sum(r, cast<UPPER_T>(x.v) * cast<UPPER_T>(y.v)) >> FB)};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> mac_unroll(RDom r, const Fixed<BASE_T, FB>& x, const Fixed<BASE_T, FB>& y)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>(sum_unroll(r, cast<UPPER_T>(x.v) * cast<UPPER_T>(y.v)) >> FB)};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> sum(RDom r, const Fixed<BASE_T, FB>& x)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>(sum(r, cast<UPPER_T>(x.v)))};
}

template<typename BASE_T, uint32_t FB>
Fixed<BASE_T, FB> sum_unroll(RDom r, const Fixed<BASE_T, FB>& x)
{
    using UPPER_T = typename Fixed<BASE_T, FB>::upper_t;
    return Fixed<BASE_T, FB>{cast<BASE_T>(sum_unroll(r, cast<UPPER_T>(x.v)))};
}

//
// Complex Expression
//

struct ComplexExpr {
    Expr x;
    Expr y;
};

ComplexExpr operator+(const ComplexExpr& a, const ComplexExpr& b)
{
    return {a.x+b.x, a.y+b.y};
}

ComplexExpr operator-(const ComplexExpr& a, const ComplexExpr& b)
{
    return {a.x-b.x, a.y-b.y};
}

ComplexExpr operator*(const ComplexExpr& a, const ComplexExpr& b)
{
    return {a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x};
}

//
// Convolutional Neural Network
//
Func conv(Func bottom, ImageParam weight, ImageParam bias,
          const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Originally outbounds should be padded by
    Func in = BoundaryConditions::constant_exterior(bottom, Expr(0),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
    f(c, x, y, n) = sum(r, in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) * weight(r.x, r.y, r.z, c)) + bias(c);

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

Func conv(Func bottom, ImageParam weight, ImageParam bias,
          const std::vector<int32_t>& weight_shape,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;
    return conv(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

template <uint32_t FB>
Func conv_fixed32(Func bottom, ImageParam weight, ImageParam bias,
                  const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Originally outbounds should be padded by
    Func in = BoundaryConditions::constant_exterior(bottom, Expr(0),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
    
    using Fixed32 = Fixed<int32_t, FB>;
    
    Fixed32 v = Fixed32{in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n)};
    Fixed32 w = Fixed32{weight(r.x, r.y, r.z, c)};
    Fixed32 b = Fixed32{bias(c)};

    f(c, x, y, n) = static_cast<Expr>(mac(r, v, w) + b);

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <uint32_t FB>
Func conv_fixed32(Func bottom, ImageParam weight, ImageParam bias,
                          const std::vector<int32_t>& weight_shape,
                          const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return conv_fixed32<FB>(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

Func bin_conv(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias,
                      const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                      const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Expr(0),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
    const float elem_num = static_cast<float>(weight_shape[0] * weight_shape[1] * weight_shape[2]);

    f(c, x, y, n) = (-elem_num +
                     2 * sum(cast<float>(!in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) ^
                                                         weight(r.x, r.y, r.z, c)))) *
        alpha(c) + bias(c);

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

Func bin_conv(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias, const std::vector<int32_t> &weight_shape,
                      const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return bin_conv(bottom, weight, alpha, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

template <uint32_t FB>
Func bin_conv_fixed32(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias,
                              const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                              const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Expr(0),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
    
    using Fixed32 = Fixed<int32_t, FB>;
    Fixed32 elem_num = to_fixed<int32_t, FB>(weight_shape[0] * weight_shape[1] * weight_shape[2]);
    Expr v = in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n);
    Expr w = weight(r.x, r.y, r.z, c);
    Fixed32 a = Fixed32{alpha(c)};
    Fixed32 b = Fixed32{bias(c)};
    
    f(c, x, y, n) = static_cast<Expr>((-elem_num + (sum(r, to_fixed<int32_t, FB>(!v ^ w)) << 1)) * a + b);
  
    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <uint32_t FB>
Func bin_conv_fixed32(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias, const std::vector<int32_t> &weight_shape,
                              const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return bin_conv_fixed32<FB>(bottom, weight, alpha, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

Func pool(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Float(32).min(),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    f(c, x, y, n) = maximum_unroll(r, in(c, x*stride + r.x, y*stride + r.y, n));

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0]) / stride + 1,
        (bottom_shape[2] - window_shape[1]) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template<uint32_t FB>
Func pool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
                          const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Int(32).min(),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    f(c, x, y, n) = maximum_unroll(r, in(c, x*stride + r.x, y*stride + r.y, n));

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0]) / stride + 1,
        (bottom_shape[2] - window_shape[1]) / stride + 1,
        bottom_shape[3]
    };

    return f;
}


Func relu2d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");

    Func f;
    f(i, n) = max(bottom(i, n), 0);

    top_shape = bottom_shape;

    return f;
}

Func relu4d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func f;
    f(c, x, y, n) = max(bottom(c, x, y, n), 0);

    top_shape = bottom_shape;

    return f;
}

Func bin_active(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = bottom(c, x, y, n) >= .0f;

    top_shape = bottom_shape;

    return f;
}

template <uint32_t FB>
Func bin_active_fixed32(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = Fixed<int32_t, FB>{bottom(c, x, y, n)} >= to_fixed<int32_t, FB>(.0f);

    top_shape = bottom_shape;

    return f;
}

Func scale(Func bottom, ImageParam weight, ImageParam bias,
                   const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = bottom(c, x, y, n) * weight(c) + bias(c);

    top_shape = bottom_shape;

    return f;
}

template <uint32_t FB>
Func scale_fixed32(Func bottom, ImageParam weight, ImageParam bias,
                           const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;
    Fixed32 v = Fixed32{bottom(c, x, y, n)};
    Fixed32 w = Fixed32{weight(c)};
    Fixed32 b = Fixed32{bias(c)};
    
    f(c, x, y, n) = static_cast<Expr>(v * w + b);

    top_shape = bottom_shape;

    return f;
}

Func bn(Func bottom, ImageParam mean, ImageParam variance,
                const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = (bottom(c, x, y, n) - mean(c)) / (sqrt(variance(c)) + Expr(.000001f));

    top_shape = bottom_shape;

    return f;
}

template <uint32_t FB>
Func bn_fixed32(Func bottom, ImageParam mean, ImageParam variance,
                        const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;
    Fixed32 b = Fixed32{bottom(c, x, y, n)};
    Fixed32 m = Fixed32{mean(c)};
    Fixed32 v = Fixed32{variance(c)};

    f(c, x, y, n) = static_cast<Expr>((b - m) / (to_fixed<int32_t, FB>(sqrt(from_fixed<float>(v))) + to_fixed<int32_t, FB>(.001f)));

    top_shape = bottom_shape;

    return f;
}

Func fc(Func bottom, ImageParam weight, ImageParam bias, const std::vector<int32_t>& weight_shape,
                const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;
    RDom r(0, weight_shape[0]);

    f(i, n) = sum(r, bottom(r.x, n) * weight(r.x, i)) + bias(i);

    top_shape = {
        weight_shape[1],
        bottom_shape[1]
    };

    return f;
}

template <uint32_t FB>
Func fc_fixed32(Func bottom, ImageParam weight, ImageParam bias,
                        const std::vector<int32_t>& weight_shape,
                        const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;
    RDom r(0, weight_shape[0]);

    using Fixed32 = Fixed<int32_t, FB>;
    Fixed32 v = Fixed32{bottom(r.x, n)};
    Fixed32 w = Fixed32{weight(r.x, i)};
    Fixed32 b = Fixed32{bias(i)};

    f(i, n) = static_cast<Expr>(mac(r, v, w) + b);

    top_shape = {
        weight_shape[1],
        bottom_shape[1]
    };

    return f;
}

Func softmax(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                     bool unrolled = false)
{
    Var i("i"), n("n");
    Func f, norm("norm");
    RDom r(0, bottom_shape[0]);

    norm(i, n) = exp(bottom(i, n) - maximum(r, bottom(r.x, n)));
    norm.compute_root();

    f(i, n) = norm(i, n) / sum(r, norm(r.x, n));

    top_shape = bottom_shape;

    // Constraints
    norm.bound(i, 0, top_shape[0])
        .bound(n, 0, top_shape[1]);

    if (unrolled) {
        norm.unroll(i);
        f.unroll(i);
    }

    return f;
}

Func im2vec(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;

    f(i, n) = bottom(i % Expr(bottom_shape[0]),
                     (i / Expr(bottom_shape[0])) % Expr(bottom_shape[1]),
                     ((i / Expr(bottom_shape[0])) / Expr(bottom_shape[1])) % Expr(bottom_shape[2]),
                     n);

    top_shape = {
        bottom_shape[0] * bottom_shape[1] * bottom_shape[2],
        bottom_shape[3]
    };

    return f;
}

template <uint32_t FB>
Func tofloat(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;
    f(i, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(i, n)});

    top_shape = bottom_shape;

    return f;
}

// 
// Sdheculing
//
ImageParam& schedule(ImageParam& ip, const std::vector<int32_t>& shape)
{
    for (size_t i=0; i<shape.size(); ++i) {
        ip.dim(i).set_bounds(0, shape[i]);
    }
    return ip;
}

Func& schedule(Func& f, const std::vector<int32_t>& shape)
{
    f.compute_root();
    for (size_t i=0; i<shape.size(); ++i) {
        f.bound(f.args()[i], 0, shape[i]);
    }
    return f;
}

}
}

#endif

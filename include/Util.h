#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <exception>

#include <Halide.h>

namespace Halide {
namespace Element {

void throw_assert(bool condition, const char *msg) 
{
    if (!condition) {
        throw std::runtime_error(msg);
    }
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


Func scale(Func bottom, ImageParam weight, ImageParam bias,
                   const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = bottom(c, x, y, n) * weight(c) + bias(c);

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



} //namespace Element
} //namespace Halide


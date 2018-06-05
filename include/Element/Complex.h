#pragma once

#include <Halide.h>

namespace Halide {
namespace Element {
namespace {

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

ComplexExpr operator/(const ComplexExpr& a, const Expr& b)
{
    return {a.x/b, a.y/b};
}

ComplexExpr conj(const ComplexExpr& a)
{
    return {a.x, -a.y};
}

ComplexExpr norm(const ComplexExpr& a)
{
    Expr len = sqrt(a.x*a.x + a.y*a.y);
    return {a.x / len, a.y / len};
}

} // anonymous
} // Element
} // Halide

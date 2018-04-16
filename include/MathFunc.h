#ifndef HALIDE_ELEMENT_MATH_H
#define HALIDE_ELEMENT_MATH_H

#include "Halide.h"
#ifndef he_user_assert
#define he_user_assert(c) _halide_internal_assertion(c, Internal::ErrorReport::User)
#endif

namespace Halide {
namespace Element {

namespace {

inline Expr log2(const Expr &x) {
    he_user_assert(x.defined()) << "log of undefined Expr\n";
#if defined(HALIDE_FOR_FPGA)
    if (x.type() == Int(32)) {
        return Internal::Call::make(Int(32), "log2_i32", {x}, Internal::Call::PureExtern);
    } else if (x.type() == UInt(32)) {
        return Internal::Call::make(UInt(32), "log2_u32", {x}, Internal::Call::PureExtern);
    } else {
        return log(x) / log(2);
    }
#else
    return log(x) / log(2);
#endif
}

// This is approximated as 2 * log2(x) + x.bit[31-nlz(x)-1]
inline Expr logr2(const Expr &x) {
    he_user_assert(x.defined()) << "log of undefined Expr\n";
#if defined(HALIDE_FOR_FPGA)
    if (x.type() == Int(32)) {
        return Internal::Call::make(Int(32), "logr2_i32", {x}, Internal::Call::PureExtern);
    } else if (x.type() == UInt(32)) {
        return Internal::Call::make(UInt(32), "logr2_u32", {x}, Internal::Call::PureExtern);
    } else {
        return log(x) / log(sqrt(2));
    }
#else
    return log(x) / log(sqrt(2));
#endif
}

} // anonymous
} // Element
} // Halide

#endif

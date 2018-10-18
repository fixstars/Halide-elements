#pragma once

#include <Halide.h>

#include "Reduction.h"
#include "Util.h"

namespace Halide {
namespace Element {
namespace {

//
// Fixed point
//

Type base_type(uint32_t nb, bool is_signed) {
    // If not used with hls-backend, represent the type by the next uppermost integer type valid in C
#if !defined(HALIDE_FOR_FPGA)
    int new_nb;
    for (new_nb = 8; new_nb < nb; new_nb *= 2);
    nb = new_nb;
#endif
    return is_signed ? Int(nb) : UInt(nb);
}

Type upper_type(const Type& type) {
    return type.with_bits(type.bits() * 2);
}

template<uint32_t NB, uint32_t FB, bool is_signed = true>
struct FixedN {
    Expr v;

    explicit operator Expr() const {
        return v;
    }

    static inline FixedN<NB, FB, is_signed> to_fixed(Expr x)
    {
        Type type = base_type(NB, is_signed);
        throw_assert(x.defined(), "conversion of undefined Expr");
        if (x.type().is_float()) {
            return FixedN<NB, FB, is_signed>{cast(type, x * Halide::Internal::make_const(type, 1<<FB))};
        } else {
            return FixedN<NB, FB, is_signed>{cast(type, x) << FB};
        }
    }

    template<typename T> static inline Expr from_fixed(const FixedN<NB, FB, is_signed>& x)
    {
        throw_assert(x.v.defined(), "conversion of undefined Expr");
        if (type_of<T>().is_float()) {
            return cast<T>(x.v) / cast<T>(1 << FB);
        } else {
            return cast<T>(x.v >> FB);
        }
    }

    static inline Expr fixed_expr(Expr x)
    {
        auto fixed = FixedN<NB, FB, is_signed>::to_fixed(x);
        return static_cast<Expr>(fixed);
    }

    template<typename T> static inline Expr from_fixed_expr(Expr x)
    {
        return from_fixed<T>(FixedN<NB, FB, is_signed>{x});
    }
};

template<uint32_t NB, uint32_t FB, bool is_signed = true>
inline FixedN<NB, FB, is_signed> to_fixed(Expr x)
{
    return FixedN<NB, FB, is_signed>::to_fixed(x);
}

template<typename T, uint32_t NB, uint32_t FB, bool is_signed>
inline Expr from_fixed(const FixedN<NB, FB, is_signed>& x)
{
    return FixedN<NB, FB, is_signed>::template from_fixed<T>(x);
}

template<uint32_t NB, uint32_t FB, bool is_signed>
inline Expr fixed_expr(Expr x)
{
    return FixedN<NB, FB, is_signed>::fixed_expr(x);
}

template<typename T, uint32_t FB>
inline Expr fixed_expr(Expr x)
{
    return fixed_expr<sizeof(T) * 8, FB, std::is_signed<T>::value>(x);
}

template<typename T, uint32_t NB, uint32_t FB, bool is_signed>
inline Expr from_fixed_expr(Expr x)
{
    return from_fixed<T>(FixedN<NB, FB, is_signed>{x});
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator-(const FixedN<NB, FB, is_signed>& x)
{
    return FixedN<NB, FB, is_signed>{-x.v};
}

template<typename BASE_T, uint32_t FB>
inline BASE_T to_fixed_base(float x)
{
    return static_cast<BASE_T>(x * (1<<FB));
}

template<typename T, typename BASE_T, uint32_t FB>
inline T from_fixed(BASE_T x)
{
    return cast<T>(x) / cast<T>(1 << FB);
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator+(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return FixedN<NB, FB, is_signed>{x.v + y.v};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator-(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return FixedN<NB, FB, is_signed>{x.v - y.v};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator*(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, (cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator/(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, (cast(upper_t, x.v) << FB) / cast(upper_t, y.v))};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator<<(const FixedN<NB, FB, is_signed>& x, Expr y)
{
    return FixedN<NB, FB, is_signed>{x.v << y};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> operator>>(const FixedN<NB, FB, is_signed>& x, Expr y)
{
    return FixedN<NB, FB, is_signed>{x.v >> y};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator==(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v == y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator!=(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v != y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator<(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v < y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator<=(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v <= y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator>(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v > y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
Expr operator>=(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return x.v >= y.v;
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> floor(const FixedN<NB, FB, is_signed>& x)
{
    constexpr uint64_t mask = ~((0x1 << FB) - 1);
    return FixedN<NB, FB, is_signed>{x.v & make_const(base_type(NB, is_signed), mask)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> abs(const FixedN<NB, FB>& x)
{
    FixedN<NB, FB> zero = to_fixed<NB, FB>(.0f);
    return select(x >= zero, x, -x);
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> min(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return FixedN<NB, FB, is_signed>{min(x.v, y.v)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> max(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    return FixedN<NB, FB, is_signed>{max(x.v, y.v)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> clamp(const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& min_val, const FixedN<NB, FB, is_signed>& max_val)
{
    return FixedN<NB, FB, is_signed>{clamp(x.v, min_val.v, max_val.v)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> select(Expr condition, const FixedN<NB, FB, is_signed>& true_value, const FixedN<NB, FB, is_signed>& false_value)
{
    return FixedN<NB, FB, is_signed>{select(condition, true_value.v, false_value.v)};
}

template<uint32_t NB, uint32_t FB, bool is_signed, typename... Args>
FixedN<NB, FB, is_signed> select(Expr c0, const FixedN<NB, FB, is_signed>& v0, Expr c1, const FixedN<NB, FB, is_signed>& v1, Args&&... args)
{
    return FixedN<NB, FB, is_signed>{select(c0, static_cast<Expr>(v0), static_cast<Expr>(select(c1, v1, std::forward<Args>(args)...)))};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> mac(RDom r, const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, sum(r, cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> mac_unroll(RDom r, const FixedN<NB, FB, is_signed>& x, const FixedN<NB, FB, is_signed>& y)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, sum_unroll(r, cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> sum(RDom r, const FixedN<NB, FB, is_signed>& x)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, sum(r, cast(upper_t, x.v)))};
}

template<uint32_t NB, uint32_t FB, bool is_signed>
FixedN<NB, FB, is_signed> sum_unroll(RDom r, const FixedN<NB, FB, is_signed>& x)
{
    Type base_t = base_type(NB, is_signed);
    Type upper_t = upper_type(base_t);
    return FixedN<NB, FB, is_signed>{cast(base_t, sum_unroll(r, cast(upper_t, x.v)))};
}

template<typename T, uint32_t FB>
using Fixed = FixedN<sizeof(T) * 8, FB, std::is_signed<T>::value>;

template<typename T, uint32_t FB>
inline Fixed<T, FB> to_fixed(Expr x)
{
    return to_fixed<sizeof(T) * 8, FB, std::is_signed<T>::value>(x);
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, NB - 2> sin(const FixedN<NB, FB>& x) {
//
// NOTE: This implementation is reverted. See issue #206 on M3/Halide.
//
//#if defined(HALIDE_FOR_FPGA)
//    return FixedN<NB, NB - 2>{Internal::Call::make(Int(NB), "sin_fixed", {(Expr)x, make_const(UInt(32), FB)}, Internal::Call::PureExtern)};
//#else
    return to_fixed<NB, NB - 2>(sin(from_fixed<double>(x)));
//#endif
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, NB - 2> cos(const FixedN<NB, FB>& x) {
//
// NOTE: This implementation is reverted. See issue #206 on M3/Halide.
//
//#if defined(HALIDE_FOR_FPGA)
//    return FixedN<NB, NB - 2>{Internal::Call::make(Int(NB), "cos_fixed", {(Expr)x, make_const(UInt(32), FB)}, Internal::Call::PureExtern)};
//#else
    return to_fixed<NB, NB - 2>(cos(from_fixed<double>(x)));
//#endif
}

}
} //namespace Element
} //namespace Halide

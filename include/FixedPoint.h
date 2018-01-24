#pragma once

#include <Halide.h>

#include "Reduction.h"
#include "Util.h"

namespace Halide {
namespace Element {

//
// Fixed point
//

template<uint32_t NB, uint32_t FB, bool is_signed = true>
struct FixedN {
    Expr v;

    explicit operator Expr() const {
        return v;
    }
};

Type base_type(uint32_t nb, bool is_signed) {
    return is_signed ? Int(nb) : UInt(nb);
}

Type upper_type(const Type& type) {
    return type.with_bits(type.bits() * 2);
}

template<uint32_t NB, uint32_t FB, bool is_signed = true>
inline FixedN<NB, FB, is_signed> to_fixed(Expr x)
{
    const Type& type = base_type(NB, is_signed);
    throw_assert(x.defined(), "conversion of undefined Expr");
    if (x.type().is_float()) {
        return FixedN<NB, FB>{cast(type, x * Halide::Internal::make_const(type, 1<<FB))};
    } else {
        return FixedN<NB, FB>{cast(type, x) << FB};
    }
}

template<typename T, uint32_t NB, uint32_t FB>
inline Expr from_fixed(const FixedN<NB, FB>& x)
{
    throw_assert(x.v.defined(), "conversion of undefined Expr");
    if (type_of<T>().is_float()) {
        return cast<T>(x.v) / cast<T>(1 << FB);
    } else {
        return cast<T>(x.v >> FB);
    }
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator-(const FixedN<NB, FB>& x)
{
    return FixedN<NB, FB>{-x.v};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator+(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return FixedN<NB, FB>{x.v + y.v};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator-(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return FixedN<NB, FB>{x.v - y.v};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator*(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, (cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator/(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, (cast(upper_t, x.v) << FB) / cast(upper_t, y.v))};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> operator<<(const FixedN<NB, FB>& x, int32_t y)
{
    return FixedN<NB, FB>{x.v << y};
}

template<uint32_t NB, uint32_t FB>
Expr operator==(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v == y.v;
}

template<uint32_t NB, uint32_t FB>
Expr operator!=(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v != y.v;
}

template<uint32_t NB, uint32_t FB>
Expr operator<(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v < y.v;
}

template<uint32_t NB, uint32_t FB>
Expr operator<=(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v <= y.v;
}

template<uint32_t NB, uint32_t FB>
Expr operator>(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v > y.v;
}

template<uint32_t NB, uint32_t FB>
Expr operator>=(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return x.v >= y.v;
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> floor(const FixedN<NB, FB>& x)
{
    constexpr uint64_t mask = ~((0x1 << FB) - 1);
    FixedN<NB, FB> zero = to_fixed<NB, FB>(.0f);
    FixedN<NB, FB> one = to_fixed<NB, FB>(1.0f);
    return FixedN<NB, FB>{select(x >= zero, x.v, (x - one).v + 1) & make_const(base_type(NB, FB), mask)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> min(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return FixedN<NB, FB>{min(x.v, y.v)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> max(const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    return FixedN<NB, FB>{max(x.v, y.v)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> clamp(const FixedN<NB, FB>& x, const FixedN<NB, FB>& min_val, const FixedN<NB, FB>& max_val)
{
    return FixedN<NB, FB>{clamp(x.v, min_val.v, max_val.v)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> select(Expr condition, const FixedN<NB, FB>& true_value, const FixedN<NB, FB>& false_value)
{
    return FixedN<NB, FB>{select(condition, true_value.v, false_value.v)};
}

template<uint32_t NB, uint32_t FB, typename... Args>
FixedN<NB, FB> select(Expr c0, const FixedN<NB, FB>& v0, Expr c1, const FixedN<NB, FB>& v1, Args&&... args)
{
    return FixedN<NB, FB>{select(c0, static_cast<Expr>(v0), static_cast<Expr>(select(c1, v1, std::forward<Args>(args)...)))};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> mac(RDom r, const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, sum(r, cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> mac_unroll(RDom r, const FixedN<NB, FB>& x, const FixedN<NB, FB>& y)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, sum_unroll(r, cast(upper_t, x.v) * cast(upper_t, y.v)) >> FB)};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> sum(RDom r, const FixedN<NB, FB>& x)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, sum(r, cast(upper_t, x.v)))};
}

template<uint32_t NB, uint32_t FB>
FixedN<NB, FB> sum_unroll(RDom r, const FixedN<NB, FB>& x)
{
    const Type& base_t = base_type(NB, FB);
    const Type& upper_t = upper_type(base_t);
    return FixedN<NB, FB>{cast(base_t, sum_unroll(r, cast(upper_t, x.v)))};
}

template<typename T, uint32_t FB>
using Fixed = FixedN<sizeof(T) * 8, FB, std::is_signed<T>::value>;

template<typename T, uint32_t FB>
inline Fixed<T, FB> to_fixed(Expr x)
{
    return to_fixed<sizeof(T) * 8, FB, std::is_signed<T>::value>(x);
}

} //namespace Element
} //namespace Halide

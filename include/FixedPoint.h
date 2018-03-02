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
        return Fixed<BASE_T, FB>{cast<BASE_T>(x * Halide::Internal::make_const(type_of<BASE_T>(), 1<<FB))};
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
Fixed<BASE_T, FB> abs(const Fixed<BASE_T, FB>& x)
{
    Fixed<BASE_T, FB> zero = to_fixed<BASE_T, FB>(.0f);
    return select(x >= zero, x, -x);
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


}
} //namespace Element
} //namespace Halide

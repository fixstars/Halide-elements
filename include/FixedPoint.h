#pragma once

#include <Halide.h>

#include "Util.h"

namespace Halide {
namespace Element {

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

template <uint32_t FB>
Func bin_active_fixed32(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = Fixed<int32_t, FB>{bottom(c, x, y, n)} >= to_fixed<int32_t, FB>(.0f);

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

template <uint32_t FB>
Func tofloat(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;
    f(i, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(i, n)});

    top_shape = bottom_shape;

    return f;
}

} //namespace Element
} //namespace Halide

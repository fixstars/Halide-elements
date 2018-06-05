#pragma once

#include <Halide.h>

namespace Halide {
namespace Element {

namespace {

//
// Convolutional Neural Network
//

std::vector<Expr> to_expr(const std::vector<int32_t>& vs)
{
    std::vector<Expr> es;
    for (auto v : vs) {
        es.push_back(v);
    }
    return es;
}

// Convolution
template <typename T>
Func conv(Func bottom, T weight, T bias,
          const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Originally outbounds should be padded by
    Func in = BoundaryConditions::constant_exterior(bottom, Expr(0.0f),
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

template <typename T>
Func conv(Func bottom, T weight, T bias,
          const std::vector<int32_t>& weight_shape,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;
    return conv(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

template <typename T, uint32_t FB>
Func conv_fixed32(Func bottom, T weight, T bias,
                  const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                  bool unroll = false)
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

    if (unroll) {
        f(c, x, y, n) = static_cast<Expr>(mac_unroll(r, v, w) + b);
    } else {
        f(c, x, y, n) = static_cast<Expr>(mac(r, v, w) + b);
    }

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <typename T, uint32_t FB>
Func conv_fixed32(Func bottom, T weight, T bias,
                  const std::vector<int32_t>& weight_shape,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape,
                  bool unroll = false)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return conv_fixed32<T, FB>(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape, unroll);
}

template <typename T, uint32_t FB>
Func conv_qr_fixed32(Func bottom, T weight, T bias,
                     const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                     bool unroll = false)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Originally outbounds should be padded by
    constexpr uint8_t zero_code = 0x7f;
    Func in = BoundaryConditions::constant_exterior(bottom, cast<uint8_t>(zero_code),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);

    using Fixed32 = Fixed<int32_t, FB>;

    Expr w = weight(r.x, r.y, r.z, c);
    Expr v = in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) & 0x7f;
    Expr vi = cast<int8_t>(v >> 1);
    Expr vf = (v & 0x1) == 0x1;
    Expr sign = (in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) & 0x80) == 0x80;
    Expr offset = cast<uint8_t>(Expr(FB));

    Expr e = cast<int32_t>(((cast<int64_t>(w) << vi) + select(vf, cast<int64_t>(w) << max(0, vi-1), 0)) >> offset);
    // Mark sign.
    e = select(v == zero_code, 0, sign, -e, e);

    Fixed32 vw = Fixed32{e};
    Fixed32 b = Fixed32{bias(c)};

    if (unroll) {
        f(c, x, y, n) = static_cast<Expr>(sum_unroll(r, vw) + b);
    } else {
        f(c, x, y, n) = static_cast<Expr>(sum(r, vw) + b);
    }

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <typename T, uint32_t FB>
Func conv_qr_fixed32(Func bottom, T weight, T bias,
                     const std::vector<int32_t>& weight_shape,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape,
                     bool unroll = false)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return conv_qr_fixed32<T, FB>(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape, unroll);
}

template <typename T, uint32_t FB>
Func conv_qq_fixed32(Func bottom, T weight, T bias,
                     const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                     bool unroll = false)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Originally outbounds should be padded by
    Func in = BoundaryConditions::constant_exterior(bottom, cast<uint8_t>(0x7f),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);

    using Fixed32 = Fixed<int32_t, FB>;

    Expr w = weight(r.x, r.y, r.z, c) & 0x7F;
    Expr wi = cast<int8_t>(w >> 1);
    Expr wf = cast<int32_t>(w & 0x1);
    Expr w_sign = (weight(r.x, r.y, r.z, c) & 0x80) == 0x80;
    Expr v = in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) & 0x7F;
    Expr vi = cast<int8_t>(v >> 1);
    Expr vf = cast<int32_t>(v & 0x1);
    Expr v_sign = (in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) & 0x80) == 0x80;

    Expr offset = cast<int8_t>(Expr(FB));

    Expr e = (cast<int32_t>(1) << max(wi + vi - offset, 0)) +
        ((wf + vf) << max(wi + vi - 1 - offset, 0)) +
        ((wf & vf) << max(wi + vi - 2 - offset, 0));
    // Mark sign.
    e = select(v == 0x7f, 0, w_sign ^ v_sign, -e, e);

    Fixed32 vw = Fixed32{e};
    Fixed32 b = Fixed32{bias(c)};

    if (unroll) {
        f(c, x, y, n) = static_cast<Expr>(sum_unroll(r, vw) + b);
    } else {
        f(c, x, y, n) = static_cast<Expr>(sum(r, vw) + b);
    }

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <typename T, uint32_t FB>
Func conv_qq_fixed32(Func bottom, T weight, T bias,
                     const std::vector<int32_t>& weight_shape,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape,
                     bool unroll = false)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return conv_qq_fixed32<T, FB>(bottom, weight, bias, weight_shape, stride, pad, bottom_shape, top_shape, unroll);
}

template <typename T>
Func bin_conv(Func bottom, T weight, T alpha, T bias,
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

    if (pad == 0) {
        f(c, x, y, n) = (-elem_num +
                         2 * sum(cast<float>(!in(r.x, x*stride - pad + r.y, y*stride - pad + r.z, n) ^
                                             weight(r.x, r.y, r.z, c)))) *
            alpha(c) + bias(c);
    } else {
        Expr tx = x*stride - pad + r.y;
        Expr ty = y*stride - pad + r.z;

        Expr iw = !in(r.x, tx, ty, n) ^ weight(r.x, r.y, r.z, c);
        f(c, x, y, n) = sum(r, select(tx >= 0 && tx < bottom_shape[1] && ty >= 0 && ty < bottom_shape[2],
                                      cast<float>(select(iw, 1, -1)),
                                      cast<float>(0))) * alpha(c) + bias(c);
    }

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template <typename T>
Func bin_conv(Func bottom, T weight, T alpha, T bias, const std::vector<int32_t> &weight_shape,
              const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return bin_conv(bottom, weight, alpha, bias, weight_shape, stride, pad, bottom_shape, top_shape);
}

template <typename T, uint32_t FB>
Func bin_conv_fixed32(Func bottom, T weight, T alpha, T bias,
                      const std::vector<int32_t>& weight_shape, int32_t stride, int32_t pad,
                      const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape,
                      bool unroll = false)
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

    if (pad == 0) {
        // f(c, x, y, n) = static_cast<Expr>((-elem_num + (sum(r, to_fixed<int32_t, FB>(!v ^ w)) << 1)) * a + b);
        Func sum;
        sum(c, x, y, n) += cast<int32_t>(!v ^ w);
        f(c, x, y, n) = static_cast<Expr>((-elem_num + (to_fixed<int32_t, FB>(sum(c, x, y, n)) << 1)) * a + b);
        if (unroll) {
            sum.update().unroll(r.x).unroll(r.y).unroll(r.z);
        }
    } else {
        Expr tx = x*stride - pad + r.y;
        Expr ty = y*stride - pad + r.z;

        //Expr iw = !v ^ w;
        //f(c, x, y, n) = static_cast<Expr>(sum(r, select(tx >= 0 && tx < bottom_shape[1] && ty >= 0 && ty < bottom_shape[2],
        //                                                to_fixed<int32_t, FB>(select(iw, 1, -1)),
        //                                                to_fixed<int32_t, FB>(0))) * a + b);
        Expr inside = tx >= 0 && tx < bottom_shape[1] && ty >= 0 && ty < bottom_shape[2];
        Func sum;
        sum(c, x, y, n) += select(inside, select(!v ^ w, 1, -1), 0);
        f(c, x, y, n) = static_cast<Expr>(to_fixed<int32_t, FB>(sum(c, x, y, n)) * a + b);
        if (unroll) {
            sum.update().unroll(r.x).unroll(r.y).unroll(r.z);
        }
    }

    top_shape = {
        weight_shape[3],
        (bottom_shape[1] - weight_shape[1] + 2*pad) / stride + 1,
        (bottom_shape[2] - weight_shape[2] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}


template <typename T, uint32_t FB>
Func bin_conv_fixed32(Func bottom, T weight, T alpha, T bias, const std::vector<int32_t> &weight_shape,
                      const std::vector<int32_t>& bottom_shape, std::vector<int32_t> &top_shape,
                      bool unroll = false)
{
    const int32_t stride = 1;
    const int32_t pad = weight_shape[1] / 2;

    return bin_conv_fixed32<T, FB>(bottom, weight, alpha, bias, weight_shape, stride, pad, bottom_shape, top_shape, unroll);
}


// Pooling
Func pool(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride, int32_t pad,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Float(32).min(),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    f(c, x, y, n) = maximum(r, in(c, x*stride - pad + r.x, y*stride - pad + r.y, n));

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0] + 2*pad) / stride + 1,
        (bottom_shape[2] - window_shape[1] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

Func pool(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
          const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    const int32_t pad = 0;
    return pool(bottom, window_shape, stride, pad, bottom_shape, top_shape);
}

Func avgpool(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride, int32_t pad,
             const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, 0,
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    int32_t num = window_shape[0] * window_shape[1];
    f(c, x, y, n) = sum(r, in(c, x*stride - pad + r.x, y*stride - pad + r.y, n)) / num;

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0] + 2*pad) / stride + 1,
        (bottom_shape[2] - window_shape[1] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

Func avgpool(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
             std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    const int32_t pad = 0;
    return avgpool(bottom, window_shape, stride, pad, bottom_shape, top_shape);
}

Func global_avgpool(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func f;
    const int32_t width = bottom_shape[1], height = bottom_shape[2];
    RDom r(0, width, 0, height);
    int32_t num = width * height;
    f(c, n) = sum(r, bottom(c, r.x, r.y, n)) / num;

    top_shape = {bottom_shape[0], bottom_shape[3]};

    return f;
}


template<uint32_t FB>
Func pool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride, int32_t pad,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                  bool unroll = false)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Int(32).min(),
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    if (unroll) {
        f(c, x, y, n) = maximum_unroll(r, in(c, x*stride - pad + r.x, y*stride - pad + r.y, n));
    } else {
        f(c, x, y, n) = maximum(r, in(c, x*stride - pad + r.x, y*stride - pad + r.y, n));
    }

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0] + 2*pad) / stride + 1,
        (bottom_shape[2] - window_shape[1] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template<uint32_t FB>
Func pool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                  bool unroll = false)
{
    const int32_t pad = 0;
    return pool_fixed32<FB>(bottom, window_shape, stride, pad, bottom_shape, top_shape, unroll);
}

template<uint32_t FB>
Func avgpool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride, int32_t pad,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                     bool unroll = false)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, 0,
                                                    0, bottom_shape[0],
                                                    0, bottom_shape[1],
                                                    0, bottom_shape[2]);
    using Fixed32 = Fixed<int32_t, FB>;

    Func f;
    RDom r(0, window_shape[0], 0, window_shape[1]);
    Fixed32 num = to_fixed<int32_t, FB>(window_shape[0] * window_shape[1]);
    Fixed32 v = Fixed32{in(c, x*stride - pad + r.x, y*stride - pad + r.y, n)};
    if (unroll) {
        f(c, x, y, n) = static_cast<Expr>(sum_unroll(r, v) / num);
    } else {
        f(c, x, y, n) = static_cast<Expr>(sum(r, v) / num);
    }

    top_shape = {
        bottom_shape[0],
        (bottom_shape[1] - window_shape[0] + 2*pad) / stride + 1,
        (bottom_shape[2] - window_shape[1] + 2*pad) / stride + 1,
        bottom_shape[3]
    };

    return f;
}

template<uint32_t FB>
Func avgpool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
                     std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                     bool unroll = false)
{
    const int32_t pad = 0;
    return avgpool_fixed32<FB>(bottom, window_shape, stride, pad, bottom_shape, top_shape, unroll);
}

template<uint32_t FB>
Func global_avgpool_fixed32(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    using Fixed32 = Fixed<int32_t, FB>;

    Func f;
    const int32_t width = bottom_shape[1], height = bottom_shape[2];
    RDom r(0, width, 0, height);
    Fixed32 num = to_fixed<int32_t, FB>(width * height);
    Fixed32 v = Fixed32{bottom(c, r.x, r.y, n)};
    f(c, n) = static_cast<Expr>(sum(r, v) / num);

    top_shape = {bottom_shape[0], bottom_shape[3]};

    return f;
}


// ReLU
Func relu(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func f;
    int dim = bottom.dimensions();

    if (dim == 2) {
        f(c, n) = max(bottom(c, n), 0);
    } else if (dim == 4) {
        f(c, x, y, n) = max(bottom(c, x, y, n), 0);
    } else {
        throw_assert(true, "unhandled dimensions");
    }

    top_shape = bottom_shape;

    return f;
}

// BinActive
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

// Batch Normalization
Func bn(Func bottom, ImageParam mean, ImageParam variance,
        const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = (bottom(c, x, y, n) - mean(c)) / (sqrt(variance(c)) + Expr(.000001f));

    top_shape = bottom_shape;

    return f;
}


template <typename T, uint32_t FB>
Func bn_fixed32(Func bottom, T mean, T variance,
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

// root 2 base log quantization.
Func log_quant(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), x("x"), y("y"), c("c"), n("n");
    Func f;

    if (bottom.dimensions() == 2) {
        Expr v = select(bottom(i, n) == 0, 1, bottom(i, n));
        v = pow(sqrt(2), round(logr2(abs(v))));

        f(i, n) = select(bottom(i, n) == .0f, .0f,
                         bottom(i, n) > .0f, v, -v);
    } else if (bottom.dimensions() == 4) {
        Expr v = select(bottom(c, x, y, n) == .0f, 1.0f, bottom(c, x, y, n));
        v = pow(sqrt(2), round(logr2(abs(v))));

        f(c, x, y, n) = select(bottom(c, x, y, n) == .0f, .0f,
                               bottom(c, x, y, n) > .0f, v, -v);
    }

    top_shape = bottom_shape;

    return f;
}

template <uint32_t FB>
Func log_quant_fixed32(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), x("x"), y("y"), c("c"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;
    Fixed32 zero = to_fixed<int32_t, FB>(.0f);
    Fixed32 one = to_fixed<int32_t, FB>(1.0f);
    // The value 0x7f expresses the bottom is zero.
    Expr zero_v = cast<uint8_t>(0x7f);

    if (bottom.dimensions() == 2) {
        Fixed32 b = Fixed32{bottom(i, n)};
        b = select(b == zero, Fixed32{1}, b);
        Expr v = select(b == zero, zero_v, cast<uint8_t>(logr2(static_cast<Expr>(abs(b)))));
        // Mark sign bit to MSB.
        Expr sign = select(bottom(i, n) < 0, make_const(UInt(8), 0x80), make_const(UInt(8), 0x00));
        f(i, n) = sign | v;
    } else if (bottom.dimensions() == 4) {
        Fixed32 b = Fixed32{bottom(c, x, y, n)};
        b = select(b == zero, Fixed32{1}, b);
        Expr v = select(b == zero, zero_v, cast<uint8_t>(logr2(static_cast<Expr>(abs(b)))));
        // Mark sign bit to MSB.
        Expr sign = select(bottom(c, x, y, n) < 0, make_const(UInt(8), 0x80), make_const(UInt(8), 0x00));
        f(c, x, y, n) = sign | v;
    }

    top_shape = bottom_shape;

    return f;
}

// Scale
Func scale(Func bottom, ImageParam weight, ImageParam bias,
           const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;

    f(c, x, y, n) = bottom(c, x, y, n) * weight(c) + bias(c);

    top_shape = bottom_shape;

    return f;
}

template <typename T, uint32_t FB>
Func scale_fixed32(Func bottom, T weight, T bias,
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


// Fully Connected
template <typename T>
Func fc(Func bottom, T weight, T bias, const std::vector<int32_t>& weight_shape,
        const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
        bool previous_conv = false)
{
    Var i("i"), n("n");
    Func f;

    if (previous_conv) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        f(i, n) = sum(r, bottom(r.x, r.y, r.z, n) * weight(r.x, r.y, r.z, i)) + bias(i);
        top_shape = {weight_shape[3], bottom_shape[3]};
    } else {
        RDom r(0, weight_shape[0]);
        f(i, n) = sum(r, bottom(r.x, n) * weight(r.x, i)) + bias(i);
        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}


template <typename T, uint32_t FB>
Func fc_fixed32(Func bottom, T weight, T bias, const std::vector<int32_t>& weight_shape,
                const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                bool previous_conv = false)
{
    Var i("i"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;

    if (previous_conv) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        Fixed32 v = Fixed32{bottom(r.x, r.y, r.z, n)};
        Fixed32 w = Fixed32{weight(r.x, r.y, r.z, i)};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(mac(r, v, w) + b);

        top_shape = {weight_shape[3], bottom_shape[3]};
    } else {
        RDom r(0, weight_shape[0]);
        Fixed32 v = Fixed32{bottom(r.x, n)};
        Fixed32 w = Fixed32{weight(r.x, i)};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(mac(r, v, w) + b);

        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}

template <typename T, uint32_t FB>
Func fc_qr_fixed32(Func bottom, T weight, T bias, const std::vector<int32_t>& weight_shape,
                   const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                   bool previous_conv = false)
{
    Var i("i"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;

    if (bottom.dimensions() == 4) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        Expr v = bottom(r.x, r.y, r.z, n) & 0x7F;
        Expr vi = cast<int8_t>(v >> 1);
        Expr vf = (v & 0x1) == 0x1;
        Expr w = weight(r.x, r.y, r.z, i);
        Expr sign = (bottom(r.x, r.y, r.z, n) & 0x80) == 0x80;
        Expr offset = cast<uint8_t>(Expr(FB));

        Expr e = cast<int32_t>(((cast<int64_t>(w) << vi) + select(vf, cast<int64_t>(w) << max(vi-1, 0), 0)) >> offset);
        // Mark sign.
        e = select(v == 0x7f, 0, sign, -e, e);

        Fixed32 vw = Fixed32{e};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(sum(r, vw) + b);

        top_shape = {weight_shape[3], bottom_shape[3]};
    } else if(bottom.dimensions() == 2) {
        RDom r(0, weight_shape[0]);
        Expr v = bottom(r.x, n) & 0x7F;
        Expr vi = cast<int8_t>(v >> 1);
        Expr vf = (v & 0x1) == 0x1;
        Expr w = weight(r.x, i);
        Expr sign = (bottom(r.x, n) & 0x80) == 0x80;
        Expr offset = cast<uint8_t>(Expr(FB));

        Expr e = cast<int32_t>(((cast<int64_t>(w) << vi) + select(vf, cast<int64_t>(w) << max(vi-1, 0), 0)) >> offset);
        // Mark sign.
        e = select(v == 0x7f, 0, sign, -e, e);

        Fixed32 vw = Fixed32{e};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(sum(r, vw) + b);

        top_shape = {weight_shape[1], bottom_shape[1]};
    } else {
        throw_assert(true, "unhandled dimensions");
    }

    return f;
}

template <typename T, uint32_t FB>
Func fc_qq_fixed32(Func bottom, T weight, T bias, const std::vector<int32_t>& weight_shape,
                   const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                   bool previous_conv = false)
{
    Var i("i"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;

    if (bottom.dimensions() == 4) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        Expr v = cast<int8_t>(bottom(r.x, r.y, r.z, n) & 0x7F);
        Expr w = cast<int8_t>(weight(r.x, r.y, r.z, i) & 0x7F);
        Expr v_sign = (bottom(r.x, r.y, r.z, n) & 0x80) == 0x80;
        Expr w_sign = (weight(r.x, r.y, r.z, i) & 0x80) == 0x80;
        Expr offset = cast<int8_t>(Expr(FB));

        Expr e = cast<int32_t>(1) << max(w + v - offset, 0);
        // Mark sign.
        e = select(v_sign ^ w_sign, -e, e);

        Fixed32 vw = Fixed32{e};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(sum(r, vw) + b);

        top_shape = {weight_shape[3], bottom_shape[3]};
    } else {
        RDom r(0, weight_shape[0]);
        Expr v = bottom(r.x, n) & 0x7F;
        Expr vi = cast<int8_t>(v >> 1);
        Expr vf = cast<int32_t>(v & 0x1);
        Expr v_sign = (bottom(r.x, n) & 0x80) == 0x80;
        Expr w = weight(r.x, i) & 0x7F;
        Expr wi = cast<int8_t>(w >> 1);
        Expr wf = cast<int32_t>(w & 0x1);
        Expr w_sign = (weight(r.x, i) & 0x80) == 0x80;

        Expr offset = cast<int8_t>(Expr(FB));

        Expr e = (cast<int32_t>(1) << max(wi + vi - offset, 0)) +
            ((wf + vf) << max(wi + vi - 1 - offset, 0)) +
            ((wf & vf) << max(wi + vi - 2 - offset, 0));
        // Mark sign.
        e = select(v == 0x7f, 0, w_sign ^ v_sign, -e, e);

        Fixed32 vw = Fixed32{e};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>(sum(r, vw) + b);

        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}


template <typename T>
Func bin_fc(Func bottom, T weight, T alpha, T bias, const std::vector<int32_t>& weight_shape,
            const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
            bool previous_conv = false)
{
    Var i("i"), n("n");
    Func f;

    if (previous_conv) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        const float elem_num = static_cast<float>(weight_shape[0] * weight_shape[1] * weight_shape[2]);
        f(i, n) = (-elem_num + 2 * sum(r, cast<float>(!bottom(r.x, r.y, r.z, n) ^ weight(r.x, r.y, r.z, i)))) * alpha(i) + bias(i);
        top_shape = {weight_shape[3], bottom_shape[3]};

    } else {
        RDom r(0, weight_shape[0]);
        const float elem_num = static_cast<float>(weight_shape[0]);
        f(i, n) = (-elem_num + 2 * sum(r, cast<float>(!bottom(r.x, n) ^ weight(r.x, i)))) * alpha(i) + bias(i);
        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}

template <typename T, uint32_t FB>
Func bin_fc_fixed32(Func bottom, T weight, T alpha, T bias, const std::vector<int32_t>& weight_shape,
                    const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                    bool previous_conv = false, bool unroll = false)
{
    Var i("i"), n("n");
    Func f;

    using Fixed32 = Fixed<int32_t, FB>;

    if (previous_conv) {
        RDom r(0, weight_shape[0], 0, weight_shape[1], 0, weight_shape[2]);
        Fixed32 elem_num = to_fixed<int32_t, FB>(static_cast<float>(weight_shape[0] * weight_shape[1] * weight_shape[2]));
        Expr v = bottom(r.x, r.y, r.z, n);
        Expr w = weight(r.x, r.y, r.z, i);
        Fixed32 a = Fixed32{alpha(i)};
        Fixed32 b = Fixed32{bias(i)};

        Func sum;
        sum(i, n) += cast<int32_t>(!v ^ w);
        if (unroll) {
            sum.update().unroll(r.x).unroll(r.y).unroll(r.z);
        }
        f(i, n) += static_cast<Expr>((-elem_num + (to_fixed<int32_t, FB>(sum(i, n)) << 1)) * a + b);

        top_shape = {weight_shape[3], bottom_shape[3]};
    } else {
        RDom r(0, weight_shape[0]);
        Fixed32 elem_num = to_fixed<int32_t, FB>(static_cast<float>(weight_shape[0]));
        Expr v = bottom(r.x, n);
        Expr w = weight(r.x, i);
        Fixed32 a = Fixed32{alpha(i)};
        Fixed32 b = Fixed32{bias(i)};

        Func sum;
        sum(i, n) += cast<int32_t>(!v ^ w);
        if (unroll) {
            sum.update().unroll(r.x);
        }
        f(i, n) += static_cast<Expr>((-elem_num + (to_fixed<int32_t, FB>(sum(i, n)) << 1)) * a + b);

        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}

// Softmax
Func softmax(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
               bool unrolled = false)
{
    Var c("c"), x("x"), y("y"), n("n");
    Func f, mins("mins"), norm("norm"), d("d");
    RDom r(0, bottom_shape[0]);
    int dim = bottom.dimensions();

    if (dim == 2) {
        mins(n) = minimum_unroll(r, bottom(r.x, n));
        schedule(mins, {bottom_shape[1]});

        norm(c, n) = exp(bottom(c, n) - mins(n));
        schedule(norm, to_expr(bottom_shape));

        d(n) = sum_unroll(r, norm(r.x, n));
        schedule(d, {bottom_shape[1]});

        f(c, n) = norm(c, n) / d(n);
    } else if (dim == 4) {
        mins(x, y, n) = minimum(r, bottom(r.x, x, y, n));
        schedule(mins, {bottom_shape[1], bottom_shape[2], bottom_shape[3]});

        norm(c, x, y, n) = exp(bottom(c, x, y, n) - mins(x, y, n));
        schedule(norm, to_expr(bottom_shape));

        d(x, y, n) = sum(r, norm(r.x, x, y, n));
        schedule(d, {bottom_shape[1], bottom_shape[2], bottom_shape[3]});

        f(c, x, y, n) = norm(c, x, y, n) / d(x, y, n);
    } else {
        throw_assert(true, "unhandled dimensions");
    }

    top_shape = bottom_shape;

    if (unrolled) {
        norm.unroll(c);
        f.unroll(c);
    }

    return f;
}

// Utils
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
    Var x("x"), y("y"), c("c"), n("n");
    Func f;
    int dim = bottom.dimensions();

    if (dim == 2) {
        f(c, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(c, n)});
    } else if (dim == 4) {
        f(c, x, y, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(c, x, y, n)});
    } else {
        throw_assert(true, "unhandled dimensions");
    }

    top_shape = bottom_shape;

    return f;
}

template <typename T>
Func binconv_module(Func bottom, std::vector<int32_t>& bottom_shape, const std::string& suffix,
                    T bn_mean, T bn_variance, T bn_weight, T bn_bias,
                    T conv_weight, T conv_alpha, T conv_bias, const std::vector<int32_t> conv_weight_shape,
                    std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Bn
    Func bn_f("bn" + suffix);
    std::vector<int32_t> bn_top_shape;
    bn_f(c, x, y, n) = bn(bottom, bn_mean, bn_variance, bottom_shape, bn_top_shape)(c, x, y, n);

    // Scale
    Func scale_f("scale" + suffix);
    std::vector<int32_t> scale_top_shape;
    scale_f(c, x, y, n) = scale(bn_f, bn_weight, bn_bias, bn_top_shape, scale_top_shape)(c, x, y, n);

    // BinActive
    Func active_f("active" + suffix);
    std::vector<int32_t> active_top_shape;
    active_f(c, x, y, n) = bin_active(scale_f, scale_top_shape, active_top_shape)(c, x, y, n);
    schedule(active_f, to_expr(active_top_shape));

    // Conv
    Func conv_f("conv" + suffix);
    std::vector<int32_t> conv_top_shape;
    conv_f(c, x, y, n) =
        bin_conv(active_f, conv_weight, conv_alpha, conv_bias, conv_weight_shape, active_top_shape, conv_top_shape)(c, x, y, n);

    // ReLU:
    Func relu_f("relu" + suffix);
    std::vector<int32_t> relu_top_shape;
    relu_f(c, x, y, n) = relu(conv_f, conv_top_shape, top_shape)(c, x, y, n);

    return relu_f;
}

template <typename T, uint32_t FB>
Func binconv_module_fixed32(Func bottom, const std::vector<int32_t>& bottom_shape, const std::string& suffix,
                            T bn_mean, T bn_variance, T bn_weight, T bn_bias,
                            T conv_weight, T conv_alpha, T conv_bias, const std::vector<int32_t> conv_weight_shape,
                            std::vector<int32_t>& top_shape,
                            bool unroll = false)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Bn
    Func bn_f("bn" + suffix);
    std::vector<int32_t> bn_top_shape;
    bn_f(c, x, y, n) = bn_fixed32<T, FB>(bottom, bn_mean, bn_variance, bottom_shape, bn_top_shape)(c, x, y, n);

    // Scale
    Func scale_f("scale" + suffix);
    std::vector<int32_t> scale_top_shape;
    scale_f(c, x, y, n) = scale_fixed32<T, FB>(bn_f, bn_weight, bn_bias, bn_top_shape, scale_top_shape)(c, x, y, n);
    schedule(scale_f, to_expr(scale_top_shape));

    // BinActive
    Func active_f("active" + suffix);
    std::vector<int32_t> active_top_shape;
    active_f(c, x, y, n) = bin_active_fixed32<FB>(scale_f, scale_top_shape, active_top_shape)(c, x, y, n);
    schedule(active_f, to_expr(active_top_shape));

    // Conv
    Func conv_f("conv" + suffix);
    std::vector<int32_t> conv_top_shape;
    conv_f(c, x, y, n) =
        bin_conv_fixed32<T, FB>(active_f, conv_weight, conv_alpha, conv_bias, conv_weight_shape, active_top_shape, conv_top_shape, unroll)(c, x, y, n);
    schedule(conv_f, to_expr(conv_top_shape));

    // ReLU:
    Func relu_f("relu" + suffix);
    std::vector<int32_t> relu_top_shape;
    relu_f(c, x, y, n) = relu(conv_f, conv_top_shape, top_shape)(c, x, y, n);

    return relu_f;
}

} // anonymous
} // Element
} // Halide

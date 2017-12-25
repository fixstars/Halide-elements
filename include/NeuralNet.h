#pragma once

#include <Halide.h>

namespace Halide {
namespace Element {

//
// Convolutional Neural Network
//

// Convolution
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

    if (pad == 0) {
        f(c, x, y, n) = static_cast<Expr>((-elem_num + (sum(r, to_fixed<int32_t, FB>(!v ^ w)) << 1)) * a + b);
    } else {
        Expr tx = x*stride - pad + r.y;
        Expr ty = y*stride - pad + r.z;

        Expr iw = !v ^ w;
        f(c, x, y, n) = static_cast<Expr>(sum(r, select(tx >= 0 && tx < bottom_shape[1] && ty >= 0 && ty < bottom_shape[2],
                                                        to_fixed<int32_t, FB>(select(iw, 1, -1)),
                                                        to_fixed<int32_t, FB>(0))) * a + b);
    }

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
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    Func in = BoundaryConditions::constant_exterior(bottom, Int(32).min(),
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

template<uint32_t FB>
Func pool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride,
                  const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    const int32_t pad = 0;
    return pool_fixed32<FB>(bottom, window_shape, stride, pad, bottom_shape, top_shape);
}

template<uint32_t FB>
Func avgpool_fixed32(Func bottom, const std::vector<int32_t>& window_shape, int32_t stride, int32_t pad,
                     const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
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
    f(c, x, y, n) = static_cast<Expr>(sum(r, v) / num);

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
                     std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    const int32_t pad = 0;
    return avgpool_fixed32<FB>(bottom, window_shape, stride, pad, bottom_shape, top_shape);
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


// Fully Connected
Func fc(Func bottom, ImageParam weight, ImageParam bias, const std::vector<int32_t>& weight_shape,
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

template <uint32_t FB>
Func fc_fixed32(Func bottom, ImageParam weight, ImageParam bias, const std::vector<int32_t>& weight_shape,
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

Func bin_fc(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias, const std::vector<int32_t>& weight_shape,
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

template <uint32_t FB>
Func bin_fc_fixed32(Func bottom, ImageParam weight, ImageParam alpha, ImageParam bias, const std::vector<int32_t>& weight_shape,
                    const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
                    bool previous_conv = false)
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

        f(i, n) = static_cast<Expr>((-elem_num + (sum(r, to_fixed<int32_t, FB>(!v ^ w)) << 1)) * a + b);

        top_shape = {weight_shape[3], bottom_shape[3]};
    } else {
        RDom r(0, weight_shape[0]);
        Fixed32 elem_num = to_fixed<int32_t, FB>(static_cast<float>(weight_shape[0]));
        Expr v = bottom(r.x, n);
        Expr w = weight(r.x, i);
        Fixed32 a = Fixed32{alpha(i)};
        Fixed32 b = Fixed32{bias(i)};

        f(i, n) = static_cast<Expr>((-elem_num + (sum(r, to_fixed<int32_t, FB>(!v ^ w)) << 1)) * a + b);

        top_shape = {weight_shape[1], bottom_shape[1]};
    }

    return f;
}


// Softmax
Func softmax2d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
               bool unrolled = false)
{
    Var i("i"), n("n");
    Func f, mins("mins"), norm("norm"), d("d");
    RDom r(0, bottom_shape[0]);

    mins(n) = minimum(r, bottom(r.x, n));
    schedule(mins, {bottom_shape[1]});

    norm(i, n) = exp(bottom(i, n) - mins(n));
    schedule(norm, bottom_shape);

    d(n) = sum(r, norm(r.x, n));
    schedule(d, {bottom_shape[1]});

    f(i, n) = norm(i, n) / d(n);

    top_shape = bottom_shape;

    if (unrolled) {
        norm.unroll(i);
        f.unroll(i);
    }

    return f;
}

Func softmax4d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape,
               bool unrolled = false)
{
    Var c("c"), x("x"), y("y"), n("n");
    Func f, mins("mins"), norm("norm"), d("d");
    RDom r(0, bottom_shape[0]);

    mins(x, y, n) = minimum(r, bottom(r.x, x, y, n));
    schedule(mins, {bottom_shape[1], bottom_shape[2], bottom_shape[3]});

    norm(c, x, y, n) = exp(bottom(c, x, y, n) - mins(x, y, n));
    schedule(norm, bottom_shape);

    d(x, y, n) = sum(r, norm(r.x, x, y, n));
    schedule(d, {bottom_shape[1], bottom_shape[2], bottom_shape[3]});

    f(c, x, y, n) = norm(c, x, y, n) / d(x, y, n);

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
Func tofloat2d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var i("i"), n("n");
    Func f;
    f(i, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(i, n)});

    top_shape = bottom_shape;

    return f;
}

template <uint32_t FB>
Func tofloat4d(Func bottom, const std::vector<int32_t>& bottom_shape, std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");
    Func f;
    f(c, x, y, n) = from_fixed<float>(Fixed<int32_t, FB>{bottom(c, x, y, n)});

    top_shape = bottom_shape;

    return f;
}


Func binconv_module(Func bottom, std::vector<int32_t>& bottom_shape, const std::string& suffix,
                    ImageParam bn_mean, ImageParam bn_variance, ImageParam bn_weight, ImageParam bn_bias,
                    ImageParam conv_weight, ImageParam conv_alpha, ImageParam conv_bias, const std::vector<int32_t> conv_weight_shape,
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
    schedule(active_f, active_top_shape);

    // Conv
    Func conv_f("conv" + suffix);
    std::vector<int32_t> conv_top_shape;
    conv_f(c, x, y, n) =
        bin_conv(active_f, conv_weight, conv_alpha, conv_bias, conv_weight_shape, active_top_shape, conv_top_shape)(c, x, y, n);

    // ReLU:
    Func relu_f("relu" + suffix);
    std::vector<int32_t> relu_top_shape;
    relu_f(c, x, y, n) = relu4d(conv_f, conv_top_shape, top_shape)(c, x, y, n);

    return relu_f;
}

template <uint32_t FB>
Func binconv_module_fixed32(Func bottom, std::vector<int32_t>& bottom_shape, const std::string& suffix,
                            ImageParam bn_mean, ImageParam bn_variance, ImageParam bn_weight, ImageParam bn_bias,
                            ImageParam conv_weight, ImageParam conv_alpha, ImageParam conv_bias, const std::vector<int32_t> conv_weight_shape,
                            std::vector<int32_t>& top_shape)
{
    Var x("x"), y("y"), c("c"), n("n");

    // Bn
    Func bn_f("bn" + suffix);
    std::vector<int32_t> bn_top_shape;
    bn_f(c, x, y, n) = bn_fixed32<FB>(bottom, bn_mean, bn_variance, bottom_shape, bn_top_shape)(c, x, y, n);

    // Scale
    Func scale_f("scale" + suffix);
    std::vector<int32_t> scale_top_shape;
    scale_f(c, x, y, n) = scale_fixed32<FB>(bn_f, bn_weight, bn_bias, bn_top_shape, scale_top_shape)(c, x, y, n);

    // BinActive
    Func active_f("active" + suffix);
    std::vector<int32_t> active_top_shape;
    active_f(c, x, y, n) = bin_active_fixed32<FB>(scale_f, scale_top_shape, active_top_shape)(c, x, y, n);
    schedule(active_f, active_top_shape);

    // Conv
    Func conv_f("conv" + suffix);
    std::vector<int32_t> conv_top_shape;
    conv_f(c, x, y, n) =
        bin_conv_fixed32<FB>(active_f, conv_weight, conv_alpha, conv_bias, conv_weight_shape, active_top_shape, conv_top_shape)(c, x, y, n);

    // ReLU:
    Func relu_f("relu" + suffix);
    std::vector<int32_t> relu_top_shape;
    relu_f(c, x, y, n) = relu4d(conv_f, conv_top_shape, top_shape)(c, x, y, n);

    return relu_f;
}


} //namespace Element
} //namespace Halide

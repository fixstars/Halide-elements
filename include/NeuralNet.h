#pragma once

#include <Halide.h>

namespace Halide {
namespace Element {

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

} //namespace Element
} //namespace Halide

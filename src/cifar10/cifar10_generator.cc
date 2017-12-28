#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class CIFAR10 : public Halide::Generator<CIFAR10> {
    ImageParam in{Int(32), 4, "in"};
    ImageParam c11w{Int(32), 4, "conv1_1_weight"};
    ImageParam c11b{Int(32), 1, "conv1_1_bias"};
    ImageParam bn11m{Int(32), 1, "bn1_1_mean"};
    ImageParam bn11v{Int(32), 1, "bn1_1_variance"};
    ImageParam bn12m{Int(32), 1, "bn1_2_mean"};
    ImageParam bn12v{Int(32), 1, "bn1_2_variance"};
    ImageParam bn12w{Int(32), 1, "bn1_2_weight"};
    ImageParam bn12b{Int(32), 1, "bn1_2_bias"};
    ImageParam c12w{Bool(), 4, "conv1_2_weight"};
    ImageParam c12a{Int(32), 1, "conv1_2_alpha"};
    ImageParam c12b{Int(32), 1, "conv1_2_bias"};
    ImageParam bn13m{Int(32), 1, "bn1_3_mean"};
    ImageParam bn13v{Int(32), 1, "bn1_3_variance"};
    ImageParam bn13w{Int(32), 1, "bn1_3_weight"};
    ImageParam bn13b{Int(32), 1, "bn1_3_bias"};
    ImageParam c13w{Bool(), 4, "conv1_3_weight"};
    ImageParam c13a{Int(32), 1, "conv1_3_alpha"};
    ImageParam c13b{Int(32), 1, "conv1_3_bias"};
    ImageParam bn21m{Int(32), 1, "bn2_1_mean"};
    ImageParam bn21v{Int(32), 1, "bn2_1_variance"};
    ImageParam bn21w{Int(32), 1, "bn2_1_weight"};
    ImageParam bn21b{Int(32), 1, "bn2_1_bias"};
    ImageParam c21w{Bool(), 4, "conv2_1_weight"};
    ImageParam c21a{Int(32), 1, "conv2_1_alpha"};
    ImageParam c21b{Int(32), 1, "conv2_1_bias"};
    ImageParam bn22m{Int(32), 1, "bn2_2_mean"};
    ImageParam bn22v{Int(32), 1, "bn2_2_variance"};
    ImageParam bn22w{Int(32), 1, "bn2_2_weight"};
    ImageParam bn22b{Int(32), 1, "bn2_2_bias"};
    ImageParam c22w{Bool(), 4, "conv2_2_weight"};
    ImageParam c22a{Int(32), 1, "conv2_2_alpha"};
    ImageParam c22b{Int(32), 1, "conv2_2_bias"};
    ImageParam bn23m{Int(32), 1, "bn2_3_mean"};
    ImageParam bn23v{Int(32), 1, "bn2_3_variance"};
    ImageParam bn23w{Int(32), 1, "bn2_3_weight"};
    ImageParam bn23b{Int(32), 1, "bn2_3_bias"};
    ImageParam c23w{Bool(), 4, "conv2_3_weight"};
    ImageParam c23a{Int(32), 1, "conv2_3_alpha"};
    ImageParam c23b{Int(32), 1, "conv2_3_bias"};
    ImageParam bn31m{Int(32), 1, "bn3_1_mean"};
    ImageParam bn31v{Int(32), 1, "bn3_1_variance"};
    ImageParam bn31w{Int(32), 1, "bn3_1_weight"};
    ImageParam bn31b{Int(32), 1, "bn3_1_bias"};
    ImageParam c31w{Bool(), 4, "conv3_1_weight"};
    ImageParam c31a{Int(32), 1, "conv3_1_alpha"};
    ImageParam c31b{Int(32), 1, "conv3_1_bias"};
    ImageParam bn32m{Int(32), 1, "bn3_2_mean"};
    ImageParam bn32v{Int(32), 1, "bn3_2_variance"};
    ImageParam bn32w{Int(32), 1, "bn3_2_weight"};
    ImageParam bn32b{Int(32), 1, "bn3_2_bias"};
    ImageParam c32w{Bool(), 4, "conv3_2_weight"};
    ImageParam c32a{Int(32), 1, "conv3_2_alpha"};
    ImageParam c32b{Int(32), 1, "conv3_2_bias"};
    ImageParam bn33m{Int(32), 1, "bn3_3_mean"};
    ImageParam bn33v{Int(32), 1, "bn3_3_variance"};
    ImageParam c33w{Int(32), 4, "conv3_3_weight"};
    ImageParam c33b{Int(32), 1, "conv3_3_bias"};

    GeneratorParam<int32_t> batch_size{"batch_size", 100};

public:
    Func build()
    {
        Var x{"x"}, y{"y"}, c{"c"}, n{"n"};
        const std::vector<int32_t> input_shape{3, 32, 32, batch_size};
        constexpr uint32_t FB = 20;

        Func input("input");
        Fixed<int32_t, FB> mean = to_fixed<int32_t, FB>(select(c == 0, 0.4914f, c == 1, 0.4822f, 0.4465f));
        Fixed<int32_t, FB> std = to_fixed<int32_t, FB>(select(c == 0, 0.2023f, c == 1, 0.1994f, 0.2010f));
        input(c, x, y, n) = static_cast<Expr>((Fixed<int32_t, FB>{in(c, x, y, n)} - mean) / std);
        schedule(input, input_shape);


        // Conv1_1(3x5x5x192): (3, 32, 32, n) -> (192, 32, 32, n)
        Func conv1_1("conv1_1");
        const std::vector<int32_t> c11w_shape{3, 5, 5, 192};
        std::vector<int32_t> conv1_1_top_shape;
        conv1_1(c, x, y, n) = conv_fixed32<FB>(input, c11w, c11b, c11w_shape, input_shape, conv1_1_top_shape)(c, x, y, n);

        // Bn1_1
        Func bn1_1("bn1_1");
        std::vector<int32_t> bn1_1_top_shape;
        bn1_1(c, x, y, n) = bn_fixed32<FB>(conv1_1, bn11m, bn11v, conv1_1_top_shape, bn1_1_top_shape)(c, x, y, n);

        // ReLU1_1:
        Func relu1_1("relu1_1");
        std::vector<int32_t> relu1_1_top_shape;
        relu1_1(c, x, y, n) = relu4d(bn1_1, bn1_1_top_shape, relu1_1_top_shape)(c, x, y, n);

        // Module1_2(192x1x1x160): (192, 32, 32, n) -> (160, 32, 32, n)
        const std::vector<int32_t> c12w_shape{192, 1, 1, 160};
        std::vector<int32_t> conv1_2_top_shape;
        Func conv1_2 = binconv_module_fixed32<FB>(relu1_1, relu1_1_top_shape, "1_2",
                                                  bn12m, bn12v, bn12w, bn12b, c12w, c12a, c12b, c12w_shape, conv1_2_top_shape);

        // Module1_3(160x1x1x96): (160, 32, 32, n) -> (96, 32, 32, n)
        const std::vector<int32_t> c13w_shape{160, 1, 1, 96};
        std::vector<int32_t> conv1_3_top_shape;
        Func conv1_3 = binconv_module_fixed32<FB>(conv1_2, conv1_2_top_shape, "1_3",
                                                  bn13m, bn13v, bn13w, bn13b, c13w, c13a, c13b, c13w_shape, conv1_3_top_shape);
        schedule(conv1_3, conv1_3_top_shape);

        // Pool1(3x3, 2): (96, 32, 32, n) -> (96, 16, 16, n)
        Func pool1("pool1");
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(conv1_3, {3, 3}, 2, 1, conv1_3_top_shape, pool1_top_shape)(c, x, y, n);


        // Module2_1(96x5x5x192): (96, 16, 16, n) -> (192, 16, 16, n)
        const std::vector<int32_t> c21w_shape{96, 5, 5, 192};
        std::vector<int32_t> conv2_1_top_shape;
        Func conv2_1 = binconv_module_fixed32<FB>(pool1, pool1_top_shape, "2_1",
                                                  bn21m, bn21v, bn21w, bn21b, c21w, c21a, c21b, c21w_shape, conv2_1_top_shape);

        // Module2_2(192x1x1x192): (192, 16, 16, n) -> (192, 16, 16, n)
        const std::vector<int32_t> c22w_shape{192, 1, 1, 192};
        std::vector<int32_t> conv2_2_top_shape;
        Func conv2_2 = binconv_module_fixed32<FB>(conv2_1, conv2_1_top_shape, "2_2",
                                                  bn22m, bn22v, bn22w, bn22b, c22w, c22a, c22b, c22w_shape, conv2_2_top_shape);

        // Module2_3(192x1x1x160): (192, 16, 16, n) -> (192, 16, 16, n)
        const std::vector<int32_t> c23w_shape{192, 1, 1, 192};
        std::vector<int32_t> conv2_3_top_shape;
        Func conv2_3 = binconv_module_fixed32<FB>(conv2_2, conv2_2_top_shape, "2_3",
                                                  bn23m, bn23v, bn23w, bn23b, c23w, c23a, c23b, c23w_shape, conv2_3_top_shape);
        schedule(conv2_3, conv2_3_top_shape);

        // Pool2(3x3, 2): (192, 16, 16, n) -> (192, 8, 8, n)
        Func pool2("pool2");
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = avgpool_fixed32<FB>(conv2_3, {3, 3}, 2, 1, conv2_3_top_shape, pool2_top_shape)(c, x, y, n);


        // Module3_1(192x3x3x192): (192, 8, 8, n) -> (192, 8, 8, n)
        const std::vector<int32_t> c31w_shape{192, 3, 3, 192};
        std::vector<int32_t> conv3_1_top_shape;
        Func conv3_1 = binconv_module_fixed32<FB>(pool2, pool2_top_shape, "3_1",
                                                  bn31m, bn31v, bn31w, bn31b, c31w, c31a, c31b, c31w_shape, conv3_1_top_shape);

        // Module3_2(192x1x1x192): (192, 8, 8, n) -> (192, 8, 8, n)
        const std::vector<int32_t> c32w_shape{192, 1, 1, 192};
        std::vector<int32_t> conv3_2_top_shape;
        Func conv3_2 = binconv_module_fixed32<FB>(conv3_1, conv3_1_top_shape, "3_2",
                                                  bn32m, bn32v, bn32w, bn32b, c32w, c32a, c32b, c32w_shape, conv3_2_top_shape);

        // Bn3_3
        Func bn3_3("bn3_3");
        std::vector<int32_t> bn3_3_top_shape;
        bn3_3(c, x, y, n) = bn_fixed32<FB>(conv3_2, bn33m, bn33v, conv3_2_top_shape, bn3_3_top_shape)(c, x, y, n);
        schedule(bn3_3, bn3_3_top_shape);

        // Conv3_3(192x1x1x10): (192, 8, 8, n) -> (10, 8, 8, n)
        const std::vector<int32_t> c33w_shape{192, 1, 1, 10};
        Func conv3_3("conv3_3");
        std::vector<int32_t> conv3_3_top_shape;
        conv3_3(c, x, y, n) = conv_fixed32<FB>(bn3_3, c33w, c33b, c33w_shape, bn3_3_top_shape, conv3_3_top_shape)(c, x, y, n);

        // ReLU3_3:
        Func relu3_3("relu3_3");
        std::vector<int32_t> relu3_3_top_shape;
        relu3_3(c, x, y, n) = relu4d(conv3_3, conv3_3_top_shape, relu3_3_top_shape)(c, x, y, n);
        schedule(relu3_3, relu3_3_top_shape);

        // Pool3(8x8, 1): (10, 8, 8, n) -> (10, 1, 1, n)
        Func pool3("pool3");
        std::vector<int32_t> pool3_top_shape;
        pool3(c, n) = global_avgpool_fixed32<FB>(relu3_3, relu3_3_top_shape, pool3_top_shape)(c, n);

        // tofloat:
        Func tof("tof");
        std::vector<int32_t> tof_top_shape;
        tof(c, n) = tofloat2d<FB>(pool3, pool3_top_shape, tof_top_shape)(c, n);
        schedule(tof, tof_top_shape);

        schedule(in, input_shape);
        schedule(c11w, c11w_shape);
        schedule(c11b, {c11w_shape[3]});
        schedule(bn11m, {c11w_shape[3]});
        schedule(bn11v, {c11w_shape[3]});
        schedule(bn12m, {c11w_shape[3]});
        schedule(bn12v, {c11w_shape[3]});
        schedule(bn12w, {c11w_shape[3]});
        schedule(bn12b, {c11w_shape[3]});
        schedule(c12w,  c12w_shape);
        schedule(c12a,   {c12w_shape[3]});
        schedule(c12b,   {c12w_shape[3]});
        schedule(bn13m, {c12w_shape[3]});
        schedule(bn13v, {c12w_shape[3]});
        schedule(bn13w, {c12w_shape[3]});
        schedule(bn13b, {c12w_shape[3]});
        schedule(c13w,  c13w_shape);
        schedule(c13a,   {c13w_shape[3]});
        schedule(c13b,   {c13w_shape[3]});
        schedule(bn21m, {c13w_shape[3]});
        schedule(bn21v, {c13w_shape[3]});
        schedule(bn21w, {c13w_shape[3]});
        schedule(bn21b, {c13w_shape[3]});
        schedule(c21w,  c21w_shape);
        schedule(c21a,   {c21w_shape[3]});
        schedule(c21b,   {c21w_shape[3]});
        schedule(bn22m, {c21w_shape[3]});
        schedule(bn22v, {c21w_shape[3]});
        schedule(bn22w, {c21w_shape[3]});
        schedule(bn22b, {c21w_shape[3]});
        schedule(c22w,  c22w_shape);
        schedule(c22a,   {c22w_shape[3]});
        schedule(c22b,   {c22w_shape[3]});
        schedule(bn23m, {c22w_shape[3]});
        schedule(bn23v, {c22w_shape[3]});
        schedule(bn23w, {c22w_shape[3]});
        schedule(bn23b, {c22w_shape[3]});
        schedule(c23w,  c23w_shape);
        schedule(c23a,   {c23w_shape[3]});
        schedule(c23b,   {c23w_shape[3]});
        schedule(bn31m, {c23w_shape[3]});
        schedule(bn31v, {c23w_shape[3]});
        schedule(bn31w, {c23w_shape[3]});
        schedule(bn31b, {c23w_shape[3]});
        schedule(c31w,  c31w_shape);
        schedule(c31a,   {c31w_shape[3]});
        schedule(c31b,   {c31w_shape[3]});
        schedule(bn32m, {c31w_shape[3]});
        schedule(bn32v, {c31w_shape[3]});
        schedule(bn32w, {c31w_shape[3]});
        schedule(bn32b, {c31w_shape[3]});
        schedule(c32w,  c32w_shape);
        schedule(c32a,   {c32w_shape[3]});
        schedule(c32b,   {c32w_shape[3]});
        schedule(bn33m, {c32w_shape[3]});
        schedule(bn33v, {c32w_shape[3]});
        schedule(c33w,  c33w_shape);
        schedule(c33b,   {c33w_shape[3]});

        schedule(input, input_shape);
        schedule(conv1_3, conv1_3_top_shape);
        schedule(conv2_3, conv2_3_top_shape);
        schedule(bn3_3, bn3_3_top_shape);
        schedule(relu3_3, relu3_3_top_shape);
        schedule(tof, tof_top_shape);

        return tof;
    }
};
HALIDE_REGISTER_GENERATOR(CIFAR10, "cifar10")

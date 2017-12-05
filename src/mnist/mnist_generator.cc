#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class MNIST : public Halide::Generator<MNIST> {
    ImageParam in{Int(32), 4, "in"};
    ImageParam conv1_weight{Int(32), 4, "conv1_weight"};
    ImageParam conv1_bias{Int(32), 1, "conv1_bias"};
    ImageParam bn1_mean{Int(32), 1, "bn1_mean"};
    ImageParam bn1_variance{Int(32), 1, "bn1_variance"};
    ImageParam bn2_mean{Int(32), 1, "bn2_mean"};
    ImageParam bn2_variance{Int(32), 1, "bn2_variance"};
    ImageParam scale2_weight{Int(32), 1, "scale2_weight"};
    ImageParam scale2_bias{Int(32), 1, "scale2_bias"};
    ImageParam conv2_weight{Bool(), 4, "conv2_weight"};
    ImageParam conv2_alpha{Int(32), 1, "conv2_alpha"};
    ImageParam conv2_bias{Int(32), 1, "conv2_bias"};
    ImageParam bn3_mean{Int(32), 1, "bn3_mean"};
    ImageParam bn3_variance{Int(32), 1, "bn3_variance"};
    ImageParam scale3_weight{Int(32), 1, "scale3_weight"};
    ImageParam scale3_bias{Int(32), 1, "scale3_bias"};
    ImageParam fc3_weight{Bool(), 4, "fc3_weight"};
    ImageParam fc3_alpha{Int(32), 1, "fc3_alpha"};
    ImageParam fc3_bias{Int(32), 1, "fc3_bias"};
    ImageParam fc4_weight{Int(32), 2, "fc4_weight"};
    ImageParam fc4_bias{Int(32), 1, "fc4_bias"};

    GeneratorParam<int32_t> batch_size{"batch_size", 2};

public:
    Func build()
    {
        Var x{"x"}, y{"y"}, c{"c"}, n{"n"}, i{"i"};
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        constexpr uint32_t FB = 20;

        Func input("input");
        input(c, x, y, n) = static_cast<Expr>((Fixed<int32_t, FB>{in(c, x, y, n)} - to_fixed<int32_t, FB>(0.1307f)) /
                                              to_fixed<int32_t, FB>(0.3081f));


        // Conv1(1x5x5x20): (1, 28, 28, n) -> (20, 24, 24, n)
        Func conv1("conv1");
        const std::vector<int32_t> conv1_weight_shape{1, 5, 5, 20};
        std::vector<int32_t> conv1_top_shape;
        conv1(c, x, y, n) =
            conv_fixed32<FB>(input, conv1_weight, conv1_bias, conv1_weight_shape, 1, 0, input_shape, conv1_top_shape)(c, x, y, n);

        // Bn1
        Func bn1("bn1");
        std::vector<int32_t> bn1_top_shape;
        bn1(c, x, y, n) = bn_fixed32<FB>(conv1, bn1_mean, bn1_variance, conv1_top_shape, bn1_top_shape)(c, x, y, n);

        // ReLU1:
        Func relu1("relu1");
        std::vector<int32_t> relu1_top_shape;
        relu1(c, x, y, n) = relu4d(bn1, bn1_top_shape, relu1_top_shape)(c, x, y, n);

        // Pool1(2x2, 2): (20, 24, 24, n) -> (20, 12, 12, n)
        Func pool1("pool1");
        const std::vector<int32_t> pool1_window_shape{2, 2};
        const int32_t pool1_stride = 2;
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(relu1, pool1_window_shape, pool1_stride, relu1_top_shape, pool1_top_shape)(c, x, y, n);


        // Bn2
        Func bn2("bn2");
        std::vector<int32_t> bn2_top_shape;
        bn2(c, x, y, n) = bn_fixed32<FB>(pool1, bn2_mean, bn2_variance, pool1_top_shape, bn2_top_shape)(c, x, y, n);

        // Scale2
        Func scale2("scale2");
        std::vector<int32_t> scale2_top_shape;
        scale2(c, x, y, n) = scale_fixed32<FB>(bn2, scale2_weight, scale2_bias, bn2_top_shape, scale2_top_shape)(c, x, y, n);

        // Active2
        Func active2("active2");
        std::vector<int32_t> active2_top_shape;
        active2(c, x, y, n) = bin_active_fixed32<FB>(scale2, scale2_top_shape, active2_top_shape)(c, x, y, n);

        // Conv2(20x5x5x50): (20, 12, 12, n) -> (50, 8, 8, n)
        Func conv2("conv2");
        const std::vector<int32_t> conv2_weight_shape{20, 5, 5, 50};
        std::vector<int32_t> conv2_top_shape;
        conv2(c, x, y, n) =
            bin_conv_fixed32<FB>(active2, conv2_weight, conv2_alpha, conv2_bias, conv2_weight_shape, 1, 0, active2_top_shape, conv2_top_shape)(c, x, y, n);

        // ReLU2:
        Func relu2("relu2");
        std::vector<int32_t> relu2_top_shape;
        relu2(c, x, y, n) = relu4d(conv2, conv2_top_shape, relu2_top_shape)(c, x, y, n);


        // Pool2(2x2, 2): (50, 8, 8, n) -> (50, 4, 4, n)
        Func pool2("pool2");
        const std::vector<int32_t> pool2_window_shape{2, 2};
        const int32_t pool2_stride = 2;
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = pool_fixed32<FB>(relu2, pool2_window_shape, pool2_stride, conv2_top_shape, pool2_top_shape)(c, x, y, n);


        // Bn3
        Func bn3("bn3");
        std::vector<int32_t> bn3_top_shape;
        bn3(c, x, y, n) = bn_fixed32<FB>(pool2, bn3_mean, bn3_variance, pool2_top_shape, bn3_top_shape)(c, x, y, n);

        // Scale3
        Func scale3("scale3");
        std::vector<int32_t> scale3_top_shape;
        scale3(c, x, y, n) = scale_fixed32<FB>(bn3, scale3_weight, scale3_bias, bn3_top_shape, scale3_top_shape)(c, x, y, n);

        // Active3
        Func active3("active3");
        std::vector<int32_t> active3_top_shape;
        active3(c, x, y, n) = bin_active_fixed32<FB>(scale3, scale3_top_shape, active3_top_shape)(c, x, y, n);

        // Fc3: (50, 4, 4, n) -> (500, n)
        Func fc3("fc3");
        const std::vector<int32_t> fc3_weight_shape{50, 4, 4, 500};
        std::vector<int32_t> fc3_top_shape;
        fc3(i, n) =
            bin_fc_fixed32<FB>(active3, fc3_weight, fc3_alpha, fc3_bias, fc3_weight_shape, active3_top_shape, fc3_top_shape, true)(i, n);

        // ReLU3:
        Func relu3("relu3");
        std::vector<int32_t> relu3_top_shape;
        relu3(i, n) = relu2d(fc3, fc3_top_shape, relu3_top_shape)(i, n);

        // Fc4: (500, n) -> (10, n)
        Func fc4("fc4");
        const std::vector<int32_t> fc4_weight_shape{500, 10};
        std::vector<int32_t> fc4_top_shape;
        fc4(i, n) = fc_fixed32<FB>(relu3, fc4_weight, fc4_bias, fc4_weight_shape, relu3_top_shape, fc4_top_shape)(i, n);

        // ToFloat
        Func tof("tofloat");
        std::vector<int32_t> tof_top_shape;
        tof(i, n) = tofloat2d<FB>(fc4, fc4_top_shape, tof_top_shape)(i, n);

        // Softmax
        Func prob("prob");
        std::vector<int32_t> prob_top_shape;
        prob(i, n) = softmax2d(tof, tof_top_shape, prob_top_shape)(i, n);

        schedule(in, input_shape);
        schedule(conv1_weight, conv1_weight_shape);
        schedule(conv1_bias, {conv1_weight_shape[3]});
        schedule(bn1_mean, {conv1_weight_shape[3]});
        schedule(bn1_variance, {conv1_weight_shape[3]});
        schedule(bn2_mean, {conv1_weight_shape[3]});
        schedule(bn2_variance, {conv1_weight_shape[3]});
        schedule(scale2_weight, {conv1_weight_shape[3]});
        schedule(scale2_bias, {conv1_weight_shape[3]});
        schedule(conv2_weight,  conv2_weight_shape);
        schedule(conv2_alpha,   {conv2_weight_shape[3]});
        schedule(conv2_bias,   {conv2_weight_shape[3]});
        schedule(bn3_mean, {conv2_weight_shape[3]});
        schedule(bn3_variance, {conv2_weight_shape[3]});
        schedule(scale3_weight, {conv2_weight_shape[3]});
        schedule(scale3_bias, {conv2_weight_shape[3]});
        schedule(fc3_weight,  fc3_weight_shape);
        schedule(fc3_alpha,   {fc3_weight_shape[3]});
        schedule(fc3_bias,   {fc3_weight_shape[3]});
        schedule(fc4_weight,  fc4_weight_shape);
        schedule(fc4_bias,   {fc4_weight_shape[1]});

        schedule(input, input_shape);
        schedule(relu1, relu1_top_shape);
        schedule(active2, active2_top_shape);
        schedule(relu2, relu2_top_shape);
        schedule(active3, active3_top_shape);
        schedule(relu3, relu3_top_shape);
        schedule(tof, tof_top_shape);
        schedule(prob, prob_top_shape);

        return prob;
    }
};
HALIDE_REGISTER_GENERATOR(MNIST, "mnist")

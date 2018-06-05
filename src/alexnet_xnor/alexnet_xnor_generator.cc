#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class AlexnetXNOR : public Halide::Generator<AlexnetXNOR> {
    Var x{"x"}, y{"y"}, c{"c"}, n{"n"}, i{"i"};

    GeneratorParam<int32_t> batch_size{"batch_size", 1};

    ImageParam in{Int(32), 4, "in"};

    ImageParam conv1_weight{Int(32), 4, "conv1_weight"};
    ImageParam conv1_bias{Int(32), 1, "conv1_bias"};
    ImageParam bn1_mean{Int(32), 1, "bn1_mean"};
    ImageParam bn1_variance{Int(32), 1, "bn1_variance"};
    ImageParam scale1_weight{Int(32), 1, "scale1_mean"};
    ImageParam scale1_bias{Int(32), 1, "scale1_variance"};

    ImageParam bn2_mean{Int(32), 1, "bn2_mean"};
    ImageParam bn2_variance{Int(32), 1, "bn2_variance"};
    ImageParam scale2_weight{Int(32), 1, "scale2_mean"};
    ImageParam scale2_bias{Int(32), 1, "scale2_variance"};
    ImageParam conv2_weight{Bool(), 4, "conv2_weight"};
    ImageParam conv2_alpha{Int(32), 1, "conv2_alpha"};
    ImageParam conv2_bias{Int(32), 1, "conv2_bias"};

    ImageParam bn3_mean{Int(32), 1, "bn3_mean"};
    ImageParam bn3_variance{Int(32), 1, "bn3_variance"};
    ImageParam scale3_weight{Int(32), 1, "scale3_mean"};
    ImageParam scale3_bias{Int(32), 1, "scale3_variance"};
    ImageParam conv3_weight{Bool(), 4, "conv3_weight"};
    ImageParam conv3_alpha{Int(32), 1, "conv3_alpha"};
    ImageParam conv3_bias{Int(32), 1, "conv3_bias"};

    ImageParam bn4_mean{Int(32), 1, "bn4_mean"};
    ImageParam bn4_variance{Int(32), 1, "bn4_variance"};
    ImageParam scale4_weight{Int(32), 1, "scale4_mean"};
    ImageParam scale4_bias{Int(32), 1, "scale4_variance"};
    ImageParam conv4_weight{Bool(), 4, "conv4_weight"};
    ImageParam conv4_alpha{Int(32), 1, "conv4_alpha"};
    ImageParam conv4_bias{Int(32), 1, "conv4_bias"};

    ImageParam bn5_mean{Int(32), 1, "bn5_mean"};
    ImageParam bn5_variance{Int(32), 1, "bn5_variance"};
    ImageParam scale5_weight{Int(32), 1, "scale5_mean"};
    ImageParam scale5_bias{Int(32), 1, "scale5_variance"};
    ImageParam conv5_weight{Bool(), 4, "conv5_weight"};
    ImageParam conv5_alpha{Int(32), 1, "conv5_alpha"};
    ImageParam conv5_bias{Int(32), 1, "conv5_bias"};

    ImageParam bn6_mean{Int(32), 1, "bn6_mean"};
    ImageParam bn6_variance{Int(32), 1, "bn6_variance"};
    ImageParam scale6_weight{Int(32), 1, "scale6_mean"};
    ImageParam scale6_bias{Int(32), 1, "scale6_variance"};
    ImageParam fc6_weight{Bool(), 4, "fc6_weight"};
    ImageParam fc6_alpha{Int(32), 1, "fc6_alpha"};
    ImageParam fc6_bias{Int(32), 1, "fc6_bias"};

    ImageParam bn7_mean{Int(32), 1, "bn7_mean"};
    ImageParam bn7_variance{Int(32), 1, "bn7_variance"};
    ImageParam scale7_weight{Int(32), 1, "scale7_mean"};
    ImageParam scale7_bias{Int(32), 1, "scale7_variance"};
    ImageParam fc7_weight{Bool(), 4, "fc7_weight"};
    ImageParam fc7_alpha{Int(32), 1, "fc7_alpha"};
    ImageParam fc7_bias{Int(32), 1, "fc7_bias"};

    ImageParam bn8_mean{Int(32), 1, "bn8_mean"};
    ImageParam bn8_variance{Int(32), 1, "bn8_variance"};
    ImageParam scale8_weight{Int(32), 1, "scale8_mean"};
    ImageParam scale8_bias{Int(32), 1, "scale8_variance"};
    ImageParam fc8_weight{Int(32), 4, "fc8_weight"};
    ImageParam fc8_bias{Int(32), 1, "fc8_bias"};

public:

    Func build()
    {
        const std::vector<int32_t> input_shape{3, 224, 224, batch_size};
        constexpr size_t FB = 16;

        Func input(lambda(_, in(_)));

        // Conv1(3x11x11x96, 4, 2): (3, 224, 224, n) -> (96, 56, 56, n)
        Func conv1("conv1");
        const int32_t conv1_num_output = 96;
        const std::vector<int32_t> conv1_weight_shape{input_shape[0], 11, 11, conv1_num_output};
        const int32_t conv1_stride = 4;
        const int32_t conv1_pad = 2;
        std::vector<int32_t> conv1_top_shape;
        conv1(c, x, y, n) = conv_fixed32<ImageParam, FB>(input, conv1_weight, conv1_bias, conv1_weight_shape, conv1_stride, conv1_pad,
                                             input_shape, conv1_top_shape)(c, x, y, n);

        // Bn1
        Func bn1("bn1");
        std::vector<int32_t> bn1_top_shape;
        bn1(c, x, y, n) = bn_fixed32<ImageParam, FB>(conv1, bn1_mean, bn1_variance, conv1_top_shape, bn1_top_shape)(c, x, y, n);

        // Scale1
        Func scale1("scale1");
        std::vector<int32_t> scale1_top_shape;
        scale1(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn1, scale1_weight, scale1_bias, bn1_top_shape, scale1_top_shape)(c, x, y, n);

        // ReLU1
        Func relu1("relu1");
        std::vector<int32_t> relu1_top_shape;
        relu1(c, x, y, n) = relu(scale1, scale1_top_shape, relu1_top_shape)(c, x, y, n);

        // Pool1(3x3, 2): (96, 56, 56, n) -> (96, 28, 28, n)
        Func pool1("pool1");
        const std::vector<int32_t> pool1_window_shape{3, 3};
        const int32_t pool1_stride = 2;
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(relu1, pool1_window_shape, pool1_stride, relu1_top_shape, pool1_top_shape)(c, x, y, n);


        // Bn2
        Func bn2("bn2");
        std::vector<int32_t> bn2_top_shape;
        bn2(c, x, y, n) = bn_fixed32<ImageParam, FB>(pool1, bn2_mean, bn2_variance, pool1_top_shape, bn2_top_shape)(c, x, y, n);

        // Scale2
        Func scale2("scale2");
        std::vector<int32_t> scale2_top_shape;
        scale2(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn2, scale2_weight, scale2_bias, bn2_top_shape, scale2_top_shape)(c, x, y, n);

        // Active2
        Func active2("active2");
        std::vector<int32_t> active2_top_shape;
        active2(c, x, y, n) = bin_active_fixed32<FB>(scale2, scale2_top_shape, active2_top_shape)(c, x, y, n);

        // Conv2(96x5x5x256): (96, 28, 28, n) -> (256, 28, 28, n)
        Func conv2("conv2");
        const int32_t conv2_num_output = 256;
        const std::vector<int32_t> conv2_weight_shape{conv1_num_output, 5, 5, conv2_num_output};
        std::vector<int32_t> conv2_top_shape;
        conv2(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active2, conv2_weight, conv2_alpha, conv2_bias, conv2_weight_shape,
                                     active2_top_shape, conv2_top_shape)(c, x, y, n);

        // Pool2(3x3, 2): (256, 28, 28, n) -> (256, 13, 13, n)
        Func pool2("pool2");
        const std::vector<int32_t> pool2_window_shape{3, 3};
        const int32_t pool2_stride = 2;
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = pool_fixed32<FB>(conv2, pool2_window_shape, pool2_stride, conv2_top_shape, pool2_top_shape)(c, x, y, n);

        // Bn3
        Func bn3("bn3");
        std::vector<int32_t> bn3_top_shape;
        bn3(c, x, y, n) = bn_fixed32<ImageParam, FB>(pool2, bn3_mean, bn3_variance, pool2_top_shape, bn3_top_shape)(c, x, y, n);

        // Scale3
        Func scale3("scale3");
        std::vector<int32_t> scale3_top_shape;
        scale3(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn3, scale3_weight, scale3_bias, bn3_top_shape, scale3_top_shape)(c, x, y, n);

        // Active3
        Func active3("active3");
        std::vector<int32_t> active3_top_shape;
        active3(c, x, y, n) = bin_active_fixed32<FB>(scale3, scale3_top_shape, active3_top_shape)(c, x, y, n);

        // Conv3(256x3x3x384): (256, 13, 13, n) -> (384, 13, 13, n)
        Func conv3("conv3");
        const int32_t conv3_num_output = 384;
        const std::vector<int32_t> conv3_weight_shape{conv2_num_output, 3, 3, conv3_num_output};
        std::vector<int32_t> conv3_top_shape;
        conv3(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active3, conv3_weight, conv3_alpha, conv3_bias, conv3_weight_shape,
                                     active3_top_shape, conv3_top_shape)(c, x, y, n);

        // Bn4
        Func bn4("bn4");
        std::vector<int32_t> bn4_top_shape;
        bn4(c, x, y, n) = bn_fixed32<ImageParam, FB>(conv3, bn4_mean, bn4_variance, conv3_top_shape, bn4_top_shape)(c, x, y, n);

        // Scale4
        Func scale4("scale4");
        std::vector<int32_t> scale4_top_shape;
        scale4(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn4, scale4_weight, scale4_bias, bn4_top_shape, scale4_top_shape)(c, x, y, n);

        // Active4
        Func active4("active4");
        std::vector<int32_t> active4_top_shape;
        active4(c, x, y, n) = bin_active_fixed32<FB>(scale4, scale4_top_shape, active4_top_shape)(c, x, y, n);

        // Conv4(384x3x3x384): (384, 14, 14, n) -> (384, 14, 14, n)
        Func conv4("conv4");
        const int32_t conv4_num_output = 384;
        const std::vector<int32_t> conv4_weight_shape{conv3_num_output, 3, 3, conv4_num_output};
        std::vector<int32_t> conv4_top_shape;
        conv4(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active4, conv4_weight, conv4_alpha, conv4_bias, conv4_weight_shape,
                                     active4_top_shape, conv4_top_shape)(c, x, y, n);


        // Bn5
        Func bn5("bn5");
        std::vector<int32_t> bn5_top_shape;
        bn5(c, x, y, n) = bn_fixed32<ImageParam, FB>(conv4, bn5_mean, bn5_variance, conv4_top_shape, bn5_top_shape)(c, x, y, n);

        // Scale5
        Func scale5("scale5");
        std::vector<int32_t> scale5_top_shape;
        scale5(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn5, scale5_weight, scale5_bias, bn5_top_shape, scale5_top_shape)(c, x, y, n);

        // Active5
        Func active5("active5");
        std::vector<int32_t> active5_top_shape;
        active5(c, x, y, n) = bin_active_fixed32<FB>(scale5, scale5_top_shape, active5_top_shape)(c, x, y, n);

        // Conv5(384x3x3x256): (384, 13, 13, n) -> (256, 14, 14, n)
        Func conv5("conv5");
        const int32_t conv5_num_output = 256;
        const std::vector<int32_t> conv5_weight_shape{conv4_num_output, 3, 3, conv5_num_output};
        std::vector<int32_t> conv5_top_shape;
        conv5(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active5, conv5_weight, conv5_alpha, conv5_bias, conv5_weight_shape,
                                     active5_top_shape, conv5_top_shape)(c, x, y, n);

        // Pool5(3x3, 2): (256, 13, 13, n) -> (256, 6, 6, n)
        Func pool5("pool5");
        const std::vector<int32_t> pool5_window_shape{3, 3};
        const int32_t pool5_stride = 2;
        std::vector<int32_t> pool5_top_shape;
        pool5(c, x, y, n) = pool_fixed32<FB>(conv5, pool5_window_shape, pool5_stride, conv5_top_shape, pool5_top_shape)(c, x, y, n);


        // Bn6
        Func bn6("bn6");
        std::vector<int32_t> bn6_top_shape;
        bn6(c, x, y, n) = bn_fixed32<ImageParam, FB>(pool5, bn6_mean, bn6_variance, pool5_top_shape, bn6_top_shape)(c, x, y, n);

        // Scale6
        Func scale6("scale6");
        std::vector<int32_t> scale6_top_shape;
        scale6(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn6, scale6_weight, scale6_bias, bn6_top_shape, scale6_top_shape)(c, x, y, n);

        // Active6
        Func active6("active6");
        std::vector<int32_t> active6_top_shape;
        active6(c, x, y, n) = bin_active(scale6, scale6_top_shape, active6_top_shape)(c, x, y, n);

        // Fc6(256x6x6x4096): (256, 6, 6, n) -> (4096, 1, 1, n)
        Func fc6("fc6");
        const int32_t fc6_num_output = 4096;
        const std::vector<int32_t> fc6_weight_shape{conv5_num_output, 6, 6, fc6_num_output};
        std::vector<int32_t> fc6_top_shape;
        fc6(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active6, fc6_weight, fc6_alpha, fc6_bias, fc6_weight_shape, 1, 0,
                                   active6_top_shape, fc6_top_shape)(c, x, y, n);


        // Bn7
        Func bn7("bn7");
        std::vector<int32_t> bn7_top_shape;
        bn7(c, x, y, n) = bn_fixed32<ImageParam, FB>(fc6, bn7_mean, bn7_variance, fc6_top_shape, bn7_top_shape)(c, x, y, n);

        // Scale7
        Func scale7("scale7");
        std::vector<int32_t> scale7_top_shape;
        scale7(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn7, scale7_weight, scale7_bias, bn7_top_shape, scale7_top_shape)(c, x, y, n);

        // Active7
        Func active7("active7");
        std::vector<int32_t> active7_top_shape;
        active7(c, x, y, n) = bin_active_fixed32<FB>(scale7, scale7_top_shape, active7_top_shape)(c, x, y, n);

        // Fc7(4096x1x1x4096): (4096, 1, 1, n) -> (4096, 1, 1, n)
        Func fc7("fc7");
        const int32_t fc7_num_output = 4096;
        const std::vector<int32_t> fc7_weight_shape{fc6_num_output, 1, 1, fc7_num_output};
        std::vector<int32_t> fc7_top_shape;
        fc7(c, x, y, n) = bin_conv_fixed32<ImageParam, FB>(active7, fc7_weight, fc7_alpha, fc7_bias, fc7_weight_shape, 1, 0,
                                   active7_top_shape, fc7_top_shape)(c, x, y, n);

        // Bn8
        Func bn8("bn8");
        std::vector<int32_t> bn8_top_shape;
        bn8(c, x, y, n) = bn_fixed32<ImageParam, FB>(fc7, bn8_mean, bn8_variance, fc7_top_shape, bn8_top_shape)(c, x, y, n);

        // Scale8
        Func scale8("scale8");
        std::vector<int32_t> scale8_top_shape;
        scale8(c, x, y, n) = scale_fixed32<ImageParam, FB>(bn8, scale8_weight, scale8_bias, bn8_top_shape, scale8_top_shape)(c, x, y, n);

        // relu8
        Func relu8("relu8");
        std::vector<int32_t> relu8_top_shape;
        relu8(c, x, y, n) = relu(scale8, scale8_top_shape, relu8_top_shape)(c, x, y, n);

        // Fc8(4096x1x1x1000): (4096, 1, 1, n) -> (1000, 1, 1, n)
        Func fc8("fc8");
        const int32_t fc8_num_output = 1000;
        const std::vector<int32_t> fc8_weight_shape{fc7_num_output, 1, 1, fc8_num_output};
        std::vector<int32_t> fc8_top_shape;
        fc8(c, x, y, n) = conv_fixed32<ImageParam, FB>(relu8, fc8_weight, fc8_bias, fc8_weight_shape, 1, 0, relu8_top_shape, fc8_top_shape)(c, x, y, n);

        // Im2Vec: (1000, 1, 1, n) -> (1000, n)
        Func i2v("im2vec");
        std::vector<int32_t> i2v_top_shape;
        i2v(i, n) = im2vec(fc8, fc8_top_shape, i2v_top_shape)(i, n);

        // ToFloat
        Func tof("tofloat");
        std::vector<int32_t> tof_top_shape;
        tof(i, n) = tofloat<FB>(i2v, i2v_top_shape, tof_top_shape)(i, n);

        // Softmax
        Func prob("prob");
        std::vector<int32_t> prob_top_shape;
        prob(i, n) = softmax(tof, tof_top_shape, prob_top_shape)(i, n);

        // Schedule
        schedule(in, to_expr(input_shape));

        schedule(conv1_weight, to_expr(conv1_weight_shape));
        schedule(conv1_bias,    {conv1_num_output});
        schedule(bn1_mean,      {conv1_num_output});
        schedule(bn1_variance,  {conv1_num_output});
        schedule(scale1_weight, {conv1_num_output});
        schedule(scale1_bias,   {conv1_num_output});

        schedule(bn2_mean,      {conv1_num_output});
        schedule(bn2_variance,  {conv1_num_output});
        schedule(scale2_weight, {conv1_num_output});
        schedule(scale2_bias,   {conv1_num_output});
        schedule(conv2_weight, to_expr(conv2_weight_shape));
        schedule(conv2_alpha,   {conv2_num_output});
        schedule(conv2_bias,    {conv2_num_output});

        schedule(bn3_mean,      {conv2_num_output});
        schedule(bn3_variance,  {conv2_num_output});
        schedule(scale3_weight, {conv2_num_output});
        schedule(scale3_bias,   {conv2_num_output});
        schedule(conv3_weight, to_expr(conv3_weight_shape));
        schedule(conv3_alpha,   {conv3_num_output});
        schedule(conv3_bias,    {conv3_num_output});

        schedule(bn4_mean,      {conv3_num_output});
        schedule(bn4_variance,  {conv3_num_output});
        schedule(scale4_weight, {conv3_num_output});
        schedule(scale4_bias,   {conv3_num_output});
        schedule(conv4_weight, to_expr(conv4_weight_shape));
        schedule(conv4_alpha,   {conv4_num_output});
        schedule(conv4_bias,    {conv4_num_output});

        schedule(bn5_mean,      {conv4_num_output});
        schedule(bn5_variance,  {conv4_num_output});
        schedule(scale5_weight, {conv4_num_output});
        schedule(scale5_bias,   {conv4_num_output});
        schedule(conv5_weight, to_expr(conv5_weight_shape));
        schedule(conv5_alpha,   {conv5_num_output});
        schedule(conv5_bias,    {conv5_num_output});

        schedule(bn6_mean,      {conv5_num_output});
        schedule(bn6_variance,  {conv5_num_output});
        schedule(scale6_weight, {conv5_num_output});
        schedule(scale6_bias,   {conv5_num_output});
        schedule(fc6_weight, to_expr(fc6_weight_shape));
        schedule(fc6_alpha,     {fc6_num_output});
        schedule(fc6_bias,      {fc6_num_output});

        schedule(bn7_mean,      {fc6_num_output});
        schedule(bn7_variance,  {fc6_num_output});
        schedule(scale7_weight, {fc6_num_output});
        schedule(scale7_bias,   {fc6_num_output});
        schedule(fc7_weight, to_expr(fc7_weight_shape));
        schedule(fc7_alpha,     {fc7_num_output});
        schedule(fc7_bias,      {fc7_num_output});

        schedule(bn8_mean,      {fc7_num_output});
        schedule(bn8_variance,  {fc7_num_output});
        schedule(scale8_weight, {fc7_num_output});
        schedule(scale8_bias,   {fc7_num_output});
        schedule(fc8_weight, to_expr(fc8_weight_shape));
        schedule(fc8_bias,      {fc8_num_output});

        schedule(relu1, to_expr(relu1_top_shape));
        schedule(active2, to_expr(active2_top_shape));
        schedule(conv1, to_expr(conv1_top_shape));
        schedule(conv2, to_expr(conv2_top_shape));
        schedule(active3, to_expr(active3_top_shape));
        schedule(active4, to_expr(active4_top_shape));
        schedule(active5, to_expr(active5_top_shape));
        schedule(conv5, to_expr(conv5_top_shape));
        schedule(active6, to_expr(active6_top_shape));
        schedule(active7, to_expr(active7_top_shape));
        schedule(relu8, to_expr(relu8_top_shape));
        schedule(fc8, to_expr(fc8_top_shape));
        schedule(i2v, to_expr(i2v_top_shape));
        schedule(prob, to_expr(prob_top_shape));

        return prob;
    }
};
HALIDE_REGISTER_GENERATOR(AlexnetXNOR, alexnet_xnor)

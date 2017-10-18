#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

class MNIST : public Halide::Generator<MNIST> {
    Var x{"x"}, y{"y"}, c{"c"}, n{"n"}, i{"i"};

    GeneratorParam<int32_t> batch_size{"batch_size", 2};
    ImageParam in{Int(32), 4, "in"};
    ImageParam conv1_weight{Int(32), 4, "conv1_weight"};
    ImageParam conv1_bias{Int(32), 1, "conv1_bias"};
    ImageParam conv2_weight{Int(32), 4, "conv2_weight"};
    ImageParam conv2_bias{Int(32), 1, "conv2_bias"};
    ImageParam fc1_weight{Int(32), 2, "fc1_weight"};
    ImageParam fc1_bias{Int(32), 1, "fc1_bias"};
    ImageParam fc2_weight{Int(32), 2, "fc2_weight"};
    ImageParam fc2_bias{Int(32), 1, "fc2_bias"};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        constexpr uint32_t FB = 22;

        Func input(lambda(_, in(_)));

        // Conv1(1x5x5x20): (1, 28, 28, n) -> (20, 28, 28, n)
        Func conv1("conv1");
        const std::vector<int32_t> conv1_weight_shape{1, 5, 5, 20};
        std::vector<int32_t> conv1_top_shape;
        conv1(c, x, y, n) = conv_fixed32<FB>(input, conv1_weight, conv1_bias, conv1_weight_shape, input_shape, conv1_top_shape)(c, x, y, n);
        
        // Pool1(2x2, 2): (20, 28, 28, n) -> (20, 14, 14, n)
        Func pool1("pool1");
        const std::vector<int32_t> pool1_window_shape{2, 2};
        const int32_t pool1_stride = 2;
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(conv1, pool1_window_shape, pool1_stride, conv1_top_shape, pool1_top_shape)(c, x, y, n);
        
        // Conv2(20x5x5x50): (20, 14, 14, n) -> (50, 14, 14, n)
        Func conv2("conv2");
        const std::vector<int32_t> conv2_weight_shape{20, 5, 5, 50};
        std::vector<int32_t> conv2_top_shape;
        conv2(c, x, y, n) = conv_fixed32<FB>(pool1, conv2_weight, conv2_bias, conv2_weight_shape, pool1_top_shape, conv2_top_shape)(c, x, y, n);
        
        // Pool2(2x2, 2): (50, 14, 14, n) -> (50, 7, 7, n)
        Func pool2("pool2");
        const std::vector<int32_t> pool2_window_shape{2, 2};
        const int32_t pool2_stride = 2;
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = pool_fixed32<FB>(conv2, pool2_window_shape, pool2_stride, conv2_top_shape, pool2_top_shape)(c, x, y, n);
        
        // Im2Vec: (50, 7, 7, n) -> (2450, n)
        Func i2v("im2vec");
        std::vector<int32_t> i2v_top_shape;
        i2v(i, n) = im2vec(pool2, pool2_top_shape, i2v_top_shape)(i, n);
       
        // Fc1: (2450, n) -> (500, n)
        Func fc1("fc1");
        const std::vector<int32_t> fc1_weight_shape{2450, 500};
        std::vector<int32_t> fc1_top_shape;
        fc1(i, n) = fc_fixed32<FB>(i2v, fc1_weight, fc1_bias, fc1_weight_shape, i2v_top_shape, fc1_top_shape)(i, n);

        // ReLU1: (500, n) -> (500, n)
        Func relu1("relu1");
        std::vector<int32_t> relu1_top_shape;
        relu1(i, n) = relu2d(fc1, fc1_top_shape, relu1_top_shape)(i, n);
        
        // Fc2: (500, n) -> (10, n)
        Func fc2("fc2");
        const std::vector<int32_t> fc2_weight_shape{500, 10};
        std::vector<int32_t> fc2_top_shape;
        fc2(i, n) = fc_fixed32<FB>(relu1, fc2_weight, fc2_bias, fc2_weight_shape, relu1_top_shape, fc2_top_shape)(i, n);
        
        // ToFloat
        Func tof("tofloat");
        std::vector<int32_t> tof_top_shape;
        tof(i, n) = tofloat<FB>(fc2, fc2_top_shape, tof_top_shape)(i, n);
      
        // Softmax
        Func prob("prob");
        std::vector<int32_t> prob_top_shape;
        prob(i, n) = softmax(tof, tof_top_shape, prob_top_shape)(i, n);

        schedule(in, input_shape);
        schedule(conv1_weight,  conv1_weight_shape);
        schedule(conv1_bias,   {conv1_weight_shape[3]});
        schedule(conv2_weight,  conv2_weight_shape);
        schedule(conv2_bias,   {conv2_weight_shape[3]});
        schedule(fc1_weight,  fc1_weight_shape);
        schedule(fc1_bias,   {fc1_weight_shape[1]});
        schedule(fc2_weight,  fc2_weight_shape);
        schedule(fc2_bias,   {fc2_weight_shape[1]});
        
        schedule(conv1, conv1_top_shape);
        schedule(pool1, pool1_top_shape);
        schedule(conv2, conv2_top_shape);
        schedule(pool2, pool2_top_shape);
        schedule(i2v, i2v_top_shape);
        schedule(relu1, relu1_top_shape);
        schedule(fc2, fc2_top_shape);
        schedule(tof, tof_top_shape);
        schedule(prob, prob_top_shape);

        return prob;
    }
};
HALIDE_REGISTER_GENERATOR(MNIST, "mnist")

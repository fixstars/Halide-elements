#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
Buffer<> load_bin(const std::string& fname, std::vector<int32_t> *extents)
{
    std::ifstream ifs(fname.c_str(), std::ios_base::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("File not found");
    }

    uint32_t dim;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));

    extents->resize(dim);

    for (size_t i=0; i<dim; i++) {
        uint32_t e;
        ifs.read(reinterpret_cast<char*>(&e), sizeof(e));
        extents->operator[](i) = static_cast<int32_t>(e);
    }

    ifs.seekg(0, std::ifstream::end);
    std::ifstream::pos_type end = ifs.tellg();

    ifs.seekg((1+dim)*sizeof(uint32_t), std::ifstream::beg);
    std::ifstream::pos_type beg = ifs.tellg();

    std::size_t buf_size_in_byte = end-beg;

    if (std::accumulate(extents->begin(), extents->end(), sizeof(T), std::multiplies<int32_t>()) != buf_size_in_byte) {
        throw std::runtime_error("Unexpected file");
    }


    Buffer<> ib(type_of<T>(), *extents);
    ifs.read(reinterpret_cast<char*>(ib.data()), buf_size_in_byte);
    return ib;
}

class MNIST : public Halide::Generator<MNIST> {
    ImageParam in{Int(32), 4, "in"};

    GeneratorParam<int32_t> batch_size{"batch_size", 1};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        schedule(in, to_expr(input_shape));

        std::vector<int32_t> conv1_weight_shape{1, 5, 5, 20};
        std::vector<int32_t> conv1_bias_shape{20};
        std::vector<int32_t> pool1_window_shape{2, 2};
        int32_t pool1_stride = 2;
        std::vector<int32_t> conv2_weight_shape{20, 5, 5, 50};
        std::vector<int32_t> conv2_bias_shape{50};
        std::vector<int32_t> pool2_window_shape{2, 2};
        int32_t pool2_stride = 2;
        std::vector<int32_t> fc3_weight_shape{50, 4, 4, 500};
        std::vector<int32_t> fc3_bias_shape{500};
        std::vector<int32_t> fc4_weight_shape{500, 10};
        std::vector<int32_t> fc4_bias_shape{10};

        Buffer<> c1w(load_bin<int32_t>("./data/conv1_weight.bin", &conv1_weight_shape));
        Buffer<> c1wq(load_bin<uint8_t>("./data/conv1_weight_q.bin", &conv1_weight_shape));
        Buffer<> c1b(load_bin<int32_t>("./data/conv1_bias.bin", &conv1_bias_shape));
        Buffer<> bn1m(load_bin<int32_t>("./data/bn1_mean.bin", &conv1_bias_shape));
        Buffer<> bn1v(load_bin<int32_t>("./data/bn1_variance.bin", &conv1_bias_shape));
        Buffer<> bn2m(load_bin<int32_t>("./data/bn2_mean.bin", &conv1_bias_shape));
        Buffer<> bn2v(load_bin<int32_t>("./data/bn2_variance.bin", &conv1_bias_shape));
        Buffer<> s2w(load_bin<int32_t>("./data/bn2_weight.bin", &conv1_bias_shape));
        Buffer<> s2b(load_bin<int32_t>("./data/bn2_bias.bin", &conv1_bias_shape));
        Buffer<> c2w(load_bin<bool>("./data/conv2_weight.bin", &conv2_weight_shape));
        Buffer<> c2a(load_bin<int32_t>("./data/conv2_alpha.bin", &conv2_bias_shape));
        Buffer<> c2b(load_bin<int32_t>("./data/conv2_bias.bin", &conv2_bias_shape));
        Buffer<> bn3m(load_bin<int32_t>("./data/bn3_mean.bin", &conv2_bias_shape));
        Buffer<> bn3v(load_bin<int32_t>("./data/bn3_variance.bin", &conv2_bias_shape));
        Buffer<> s3w(load_bin<int32_t>("./data/bn3_weight.bin", &conv2_bias_shape));
        Buffer<> s3b(load_bin<int32_t>("./data/bn3_bias.bin", &conv2_bias_shape));
        Buffer<> f3w(load_bin<bool>("./data/ip3_weight.bin", &fc3_weight_shape));
        Buffer<> f3a(load_bin<int32_t>("./data/ip3_alpha.bin", &fc3_bias_shape));
        Buffer<> f3b(load_bin<int32_t>("./data/ip3_bias.bin", &fc3_bias_shape));
        Buffer<> f4w(load_bin<int32_t>("./data/ip4_weight.bin", &fc4_weight_shape));
        Buffer<> f4wq(load_bin<uint8_t>("./data/ip4_weight_q.bin", &fc4_weight_shape));
        Buffer<> f4b(load_bin<int32_t>("./data/ip4_bias.bin", &fc4_bias_shape));

        schedule_burst(c1wq, conv1_weight_shape[0]*conv1_weight_shape[1]*conv1_weight_shape[2]);
        schedule_burst(c2w, conv2_weight_shape[0]*conv2_weight_shape[1]*conv2_weight_shape[2]);
        schedule_burst(f3w, fc3_weight_shape[0]*fc3_weight_shape[1]*fc3_weight_shape[2]);

        constexpr size_t FB = 20;
        Var x("x"), y("y"), c("c"), i("i"), n("n");

        Func input("input");
        input(c, x, y, n) = static_cast<Expr>((Fixed<int32_t, FB>{in(c, x, y, n)} - to_fixed<int32_t, FB>(0.1307f))
                                              / to_fixed<int32_t, FB>(0.3081f));

        // Lq1
        Func lq1("lq1");
        std::vector<int32_t> lq1_top_shape;
        lq1(c, x, y, n) = log_quant_fixed32<FB>(input, input_shape, lq1_top_shape)(c, x, y, n);
        schedule(lq1, to_expr(lq1_top_shape));

        // Conv1(1x5x5x20): (1, 28, 28, n) -> (20, 24, 24, n)
        Func conv1("conv1");
        std::vector<int32_t> conv1_top_shape;
        conv1(c, x, y, n) = conv_qq_fixed32<Buffer<>, FB>(lq1, c1wq, c1b, conv1_weight_shape, 1, 0, input_shape, conv1_top_shape, true)(c, x, y, n);

        // Bn1
        Func bn1("bn1");
        std::vector<int32_t> bn1_top_shape;
        bn1(c, x, y, n) = bn_fixed32<Buffer<>, FB>(conv1, bn1m, bn1v, conv1_top_shape, bn1_top_shape)(c, x, y, n);

        // ReLU1
        Func relu1("relu1");
        std::vector<int32_t> relu1_top_shape;
        relu1(c, x, y, n) = relu(bn1, bn1_top_shape, relu1_top_shape)(c, x, y, n);
        schedule(relu1, to_expr(relu1_top_shape));

        // Pool1(2x2, 2): (20, 24, 24, n) -> (20, 12, 12, n)
        Func pool1("pool1");
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(relu1, pool1_window_shape, pool1_stride, relu1_top_shape, pool1_top_shape, true)(c, x, y, n);

        // Bn2
        Func bn2("bn2");
        std::vector<int32_t> bn2_top_shape;
        bn2(c, x, y, n) = bn_fixed32<Buffer<>, FB>(pool1, bn2m, bn2v, pool1_top_shape, bn2_top_shape)(c, x, y, n);

        // Scale2
        Func scale2("scale2");
        std::vector<int32_t> scale2_top_shape;
        scale2(c, x, y, n) = scale_fixed32<Buffer<>, FB>(bn2, s2w, s2b, bn2_top_shape, scale2_top_shape)(c, x, y, n);

        // Active2
        Func active2("active2");
        std::vector<int32_t> active2_top_shape;
        active2(c, x, y, n) = bin_active_fixed32<FB>(scale2, scale2_top_shape, active2_top_shape)(c, x, y, n);
        schedule_burst(active2, to_expr(active2_top_shape), active2_top_shape[0]);

        // Conv2(20x5x5x50): (20, 12, 12, n) -> (50, 8, 8, n)
        Func conv2("conv2");
        std::vector<int32_t> conv2_top_shape;
        conv2(c, x, y, n) = bin_conv_fixed32<Buffer<>, FB>(active2, c2w, c2a, c2b, conv2_weight_shape, 1, 0,
                                                           active2_top_shape, conv2_top_shape, true)(c, x, y, n);

        // ReLU2
        Func relu2("relu2");
        std::vector<int32_t> relu2_top_shape;
        relu2(c, x, y, n) = relu(conv2, conv2_top_shape, relu2_top_shape)(c, x, y, n);
        schedule(relu2, to_expr(relu2_top_shape));

        // Pool2(2x2, 2): (50, 8, 8, n) -> (50, 4, 4, n)
        Func pool2("pool2");
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = pool_fixed32<FB>(relu2, pool2_window_shape, pool2_stride, relu2_top_shape, pool2_top_shape, true)(c, x, y, n);
        schedule(pool2, to_expr(pool2_top_shape));


        // Bn3
        Func bn3("bn3");
        std::vector<int32_t> bn3_top_shape;
        bn3(c, x, y, n) = bn_fixed32<Buffer<>, FB>(pool2, bn3m, bn3v, pool2_top_shape, bn3_top_shape)(c, x, y, n);

        // Scale3
        Func scale3("scale3");
        std::vector<int32_t> scale3_top_shape;
        scale3(c, x, y, n) = scale_fixed32<Buffer<>, FB>(bn3, s3w, s3b, bn3_top_shape, scale3_top_shape)(c, x, y, n);

        // Active3
        Func active3("active3");
        std::vector<int32_t> active3_top_shape;
        active3(c, x, y, n) = bin_active_fixed32<FB>(scale3, scale3_top_shape, active3_top_shape)(c, x, y, n);
        schedule_burst(active3, to_expr(active3_top_shape), active3_top_shape[0]*active3_top_shape[1]*active3_top_shape[2]);

        // Fc3: (800, n) -> (500, n)
        Func fc3("fc3");
        std::vector<int32_t> fc3_top_shape;
        fc3(i, n) = bin_fc_fixed32<Buffer<>, FB>(active3, f3w, f3a, f3b, fc3_weight_shape, active3_top_shape, fc3_top_shape, true, true)(i, n);

        // ReLU3:
        Func relu3("relu3");
        std::vector<int32_t> relu3_top_shape;
        relu3(i, n) = relu(fc3, fc3_top_shape, relu3_top_shape)(i, n);

        // Lq4
        Func lq4("lq4");
        std::vector<int32_t> lq4_top_shape;
        lq4(i, n) = log_quant_fixed32<FB>(relu3, relu3_top_shape, lq4_top_shape)(i, n);
        schedule_memory(lq4, to_expr(lq4_top_shape));

        // Fc4: (500, n) -> (10, n)
        Func fc4("fc4");
        std::vector<int32_t> fc4_top_shape;
        fc4(i, n) = fc_qq_fixed32<Buffer<>, FB>(lq4, f4wq, f4b, fc4_weight_shape, lq4_top_shape, fc4_top_shape)(i, n);

        // tofloat:
        Func tof("tof");
        std::vector<int32_t> tof_top_shape;
        tof(i, n) = tofloat<FB>(fc4, fc4_top_shape, tof_top_shape)(i, n);
        schedule(tof, to_expr(tof_top_shape));

        // Softmax
        Func prob("prob");
        std::vector<int32_t> prob_top_shape;
        prob(i, n) = softmax(tof, tof_top_shape, prob_top_shape)(i, n);
        schedule(prob, to_expr(prob_top_shape));

        return prob;
    }
};
HALIDE_REGISTER_GENERATOR(MNIST, mnist)

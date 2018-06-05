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

class CIFAR10 : public Halide::Generator<CIFAR10> {

    ImageParam in{Int(32), 4, "in"};

    GeneratorParam<int32_t> batch_size{"batch_size", 1};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{3, 32, 32, batch_size};
        schedule(in, to_expr(input_shape));

        std::vector<int32_t> c11w_shape{3, 5, 5, 192};
        std::vector<int32_t> c11b_shape{192};
        std::vector<int32_t> c12w_shape{192, 1, 1, 160};
        std::vector<int32_t> c12b_shape{160};
        std::vector<int32_t> c13w_shape{160, 1, 1, 96};
        std::vector<int32_t> c13b_shape{96};
        std::vector<int32_t> c21w_shape{96, 5, 5, 192};
        std::vector<int32_t> c21b_shape{192};
        std::vector<int32_t> c22w_shape{192, 1, 1, 192};
        std::vector<int32_t> c22b_shape{192};
        std::vector<int32_t> c23w_shape{192, 1, 1, 192};
        std::vector<int32_t> c23b_shape{192};
        std::vector<int32_t> c31w_shape{192, 3, 3, 192};
        std::vector<int32_t> c31b_shape{192};
        std::vector<int32_t> c32w_shape{192, 1, 1, 192};
        std::vector<int32_t> c32b_shape{192};
        std::vector<int32_t> c33w_shape{192, 1, 1, 10};
        std::vector<int32_t> c33b_shape{10};

        Buffer<> c11w(load_bin<int32_t>("./data/conv1_1_weight.bin", &c11w_shape));
        Buffer<> c11wq(load_bin<uint8_t>("./data/conv1_1_weight_q.bin", &c11w_shape));
        Buffer<> c11b(load_bin<int32_t>("./data/conv1_1_bias.bin", &c11b_shape));
        Buffer<> bn11m(load_bin<int32_t>("./data/bn1_1_mean.bin", &c11b_shape));
        Buffer<> bn11v(load_bin<int32_t>("./data/bn1_1_variance.bin", &c11b_shape));
        Buffer<> bn12m(load_bin<int32_t>("./data/bn1_2_mean.bin", &c11b_shape));
        Buffer<> bn12v(load_bin<int32_t>("./data/bn1_2_variance.bin", &c11b_shape));
        Buffer<> bn12w(load_bin<int32_t>("./data/bn1_2_weight.bin", &c11b_shape));
        Buffer<> bn12b(load_bin<int32_t>("./data/bn1_2_bias.bin", &c11b_shape));
        Buffer<> c12w(load_bin<bool>("./data/conv1_2_weight.bin", &c12w_shape));
        Buffer<> c12a(load_bin<int32_t>("./data/conv1_2_alpha.bin", &c12b_shape));
        Buffer<> c12b(load_bin<int32_t>("./data/conv1_2_bias.bin", &c12b_shape));
        Buffer<> bn13m(load_bin<int32_t>("./data/bn1_3_mean.bin", &c12b_shape));
        Buffer<> bn13v(load_bin<int32_t>("./data/bn1_3_variance.bin", &c12b_shape));
        Buffer<> bn13w(load_bin<int32_t>("./data/bn1_3_weight.bin", &c12b_shape));
        Buffer<> bn13b(load_bin<int32_t>("./data/bn1_3_bias.bin", &c12b_shape));
        Buffer<> c13w(load_bin<bool>("./data/conv1_3_weight.bin", &c13w_shape));
        Buffer<> c13a(load_bin<int32_t>("./data/conv1_3_alpha.bin", &c13b_shape));
        Buffer<> c13b(load_bin<int32_t>("./data/conv1_3_bias.bin", &c13b_shape));
        Buffer<> bn21m(load_bin<int32_t>("./data/bn2_1_mean.bin", &c13b_shape));
        Buffer<> bn21v(load_bin<int32_t>("./data/bn2_1_variance.bin", &c13b_shape));
        Buffer<> bn21w(load_bin<int32_t>("./data/bn2_1_weight.bin", &c13b_shape));
        Buffer<> bn21b(load_bin<int32_t>("./data/bn2_1_bias.bin", &c13b_shape));
        Buffer<> c21w(load_bin<bool>("./data/conv2_1_weight.bin", &c21w_shape));
        Buffer<> c21a(load_bin<int32_t>("./data/conv2_1_alpha.bin", &c21b_shape));
        Buffer<> c21b(load_bin<int32_t>("./data/conv2_1_bias.bin", &c21b_shape));
        Buffer<> bn22m(load_bin<int32_t>("./data/bn2_2_mean.bin", &c21b_shape));
        Buffer<> bn22v(load_bin<int32_t>("./data/bn2_2_variance.bin", &c21b_shape));
        Buffer<> bn22w(load_bin<int32_t>("./data/bn2_2_weight.bin", &c21b_shape));
        Buffer<> bn22b(load_bin<int32_t>("./data/bn2_2_bias.bin", &c21b_shape));
        Buffer<> c22w(load_bin<bool>("./data/conv2_2_weight.bin", &c22w_shape));
        Buffer<> c22a(load_bin<int32_t>("./data/conv2_2_alpha.bin", &c22b_shape));
        Buffer<> c22b(load_bin<int32_t>("./data/conv2_2_bias.bin", &c22b_shape));
        Buffer<> bn23m(load_bin<int32_t>("./data/bn2_3_mean.bin", &c22b_shape));
        Buffer<> bn23v(load_bin<int32_t>("./data/bn2_3_variance.bin", &c22b_shape));
        Buffer<> bn23w(load_bin<int32_t>("./data/bn2_3_weight.bin", &c22b_shape));
        Buffer<> bn23b(load_bin<int32_t>("./data/bn2_3_bias.bin", &c22b_shape));
        Buffer<> c23w(load_bin<bool>("./data/conv2_3_weight.bin", &c23w_shape));
        Buffer<> c23a(load_bin<int32_t>("./data/conv2_3_alpha.bin", &c23b_shape));
        Buffer<> c23b(load_bin<int32_t>("./data/conv2_3_bias.bin", &c23b_shape));
        Buffer<> bn31m(load_bin<int32_t>("./data/bn3_1_mean.bin", &c23b_shape));
        Buffer<> bn31v(load_bin<int32_t>("./data/bn3_1_variance.bin", &c23b_shape));
        Buffer<> bn31w(load_bin<int32_t>("./data/bn3_1_weight.bin", &c23b_shape));
        Buffer<> bn31b(load_bin<int32_t>("./data/bn3_1_bias.bin", &c23b_shape));
        Buffer<> c31w(load_bin<bool>("./data/conv3_1_weight.bin", &c31w_shape));
        Buffer<> c31a(load_bin<int32_t>("./data/conv3_1_alpha.bin", &c31b_shape));
        Buffer<> c31b(load_bin<int32_t>("./data/conv3_1_bias.bin", &c31b_shape));
        Buffer<> bn32m(load_bin<int32_t>("./data/bn3_2_mean.bin", &c31b_shape));
        Buffer<> bn32v(load_bin<int32_t>("./data/bn3_2_variance.bin", &c31b_shape));
        Buffer<> bn32w(load_bin<int32_t>("./data/bn3_2_weight.bin", &c31b_shape));
        Buffer<> bn32b(load_bin<int32_t>("./data/bn3_2_bias.bin", &c31b_shape));
        Buffer<> c32w(load_bin<bool>("./data/conv3_2_weight.bin", &c32w_shape));
        Buffer<> c32a(load_bin<int32_t>("./data/conv3_2_alpha.bin", &c32b_shape));
        Buffer<> c32b(load_bin<int32_t>("./data/conv3_2_bias.bin", &c32b_shape));
        Buffer<> bn33m(load_bin<int32_t>("./data/bn3_3_mean.bin", &c32b_shape));
        Buffer<> bn33v(load_bin<int32_t>("./data/bn3_3_variance.bin", &c32b_shape));
        Buffer<> c33w(load_bin<int32_t>("./data/conv3_3_weight.bin", &c33w_shape));
        Buffer<> c33wq(load_bin<uint8_t>("./data/conv3_3_weight_q.bin", &c33w_shape));
        Buffer<> c33b(load_bin<int32_t>("./data/conv3_3_bias.bin", &c33b_shape));

        // TODO: Tune up FPS
        // schedule_burst(c11wq, c11w_shape[0]*c11w_shape[1]*c11w_shape[2]);
        // schedule_burst(c12w, c12w_shape[0]*c12w_shape[1]*c12w_shape[2]);
        // schedule_burst(c13w, c13w_shape[0]*c13w_shape[1]*c13w_shape[2]);
        // schedule_burst(c21w, c21w_shape[0]*c21w_shape[1]*c21w_shape[2]);
        // schedule_burst(c22w, c22w_shape[0]*c22w_shape[1]*c22w_shape[2]);
        // schedule_burst(c23w, c23w_shape[0]*c23w_shape[1]*c23w_shape[2]);
        // schedule_burst(c31w, c31w_shape[0]*c31w_shape[1]*c31w_shape[2]);
        // schedule_burst(c32w, c32w_shape[0]*c32w_shape[1]*c32w_shape[2]);
        // schedule_burst(c33wq, c33w_shape[0]*c33w_shape[1]*c33w_shape[2]);

        constexpr uint32_t FB = 20;
        Var x{"x"}, y{"y"}, c{"c"}, n{"n"};

        Func input("input");
        Fixed<int32_t, FB> mean = to_fixed<int32_t, FB>(select(c == 0, 0.4914f, c == 1, 0.4822f, 0.4465f));
        Fixed<int32_t, FB> std = to_fixed<int32_t, FB>(select(c == 0, 0.2023f, c == 1, 0.1994f, 0.2010f));
        input(c, x, y, n) = static_cast<Expr>((Fixed<int32_t, FB>{in(c, x, y, n)} - mean) / std);

        // Lq1_1
        Func lq1_1("lq1_1");
        std::vector<int32_t> lq1_1_top_shape;
        lq1_1(c, x, y, n) = log_quant_fixed32<FB>(input, input_shape, lq1_1_top_shape)(c, x, y, n);
        schedule(lq1_1, to_expr(lq1_1_top_shape));

        // Conv1_1(3x5x5x192): (3, 32, 32, n) -> (192, 32, 32, n)
        Func conv1_1("conv1_1");
        std::vector<int32_t> conv1_1_top_shape;
        conv1_1(c, x, y, n) = conv_qq_fixed32<Buffer<>, FB>(lq1_1, c11wq, c11b, c11w_shape, lq1_1_top_shape, conv1_1_top_shape)(c, x, y, n);

        // Bn1_1
        Func bn1_1("bn1_1");
        std::vector<int32_t> bn1_1_top_shape;
        bn1_1(c, x, y, n) = bn_fixed32<Buffer<>, FB>(conv1_1, bn11m, bn11v, conv1_1_top_shape, bn1_1_top_shape)(c, x, y, n);

        // ReLU1_1:
        Func relu1_1("relu1_1");
        std::vector<int32_t> relu1_1_top_shape;
        relu1_1(c, x, y, n) = relu(bn1_1, bn1_1_top_shape, relu1_1_top_shape)(c, x, y, n);

        // Module1_2(192x1x1x160): (192, 32, 32, n) -> (160, 32, 32, n)
        std::vector<int32_t> conv1_2_top_shape;
        Func conv1_2 = binconv_module_fixed32<Buffer<>, FB>(relu1_1, relu1_1_top_shape, "1_2",
                                                            bn12m, bn12v, bn12w, bn12b, c12w, c12a, c12b, c12w_shape, conv1_2_top_shape);

        // Module1_3(160x1x1x96): (160, 32, 32, n) -> (96, 32, 32, n)
        std::vector<int32_t> conv1_3_top_shape;
        Func conv1_3 = binconv_module_fixed32<Buffer<>, FB>(conv1_2, conv1_2_top_shape, "1_3",
                                                            bn13m, bn13v, bn13w, bn13b, c13w, c13a, c13b, c13w_shape, conv1_3_top_shape);
        schedule(conv1_3, to_expr(conv1_3_top_shape));

        // Pool1(3x3, 2): (96, 32, 32, n) -> (96, 16, 16, n)
        Func pool1("pool1");
        std::vector<int32_t> pool1_top_shape;
        pool1(c, x, y, n) = pool_fixed32<FB>(conv1_3, {3, 3}, 2, 1, conv1_3_top_shape, pool1_top_shape)(c, x, y, n);


        // Module2_1(96x5x5x192): (96, 16, 16, n) -> (192, 16, 16, n)
        std::vector<int32_t> conv2_1_top_shape;
        Func conv2_1 = binconv_module_fixed32<Buffer<>, FB>(pool1, pool1_top_shape, "2_1",
                                                            bn21m, bn21v, bn21w, bn21b, c21w, c21a, c21b, c21w_shape, conv2_1_top_shape);

        // Module2_2(192x1x1x192): (192, 16, 16, n) -> (192, 16, 16, n)
        std::vector<int32_t> conv2_2_top_shape;
        Func conv2_2 = binconv_module_fixed32<Buffer<>, FB>(conv2_1, conv2_1_top_shape, "2_2",
                                                            bn22m, bn22v, bn22w, bn22b, c22w, c22a, c22b, c22w_shape, conv2_2_top_shape);

        // Module2_3(192x1x1x160): (192, 16, 16, n) -> (192, 16, 16, n)
        std::vector<int32_t> conv2_3_top_shape;
        Func conv2_3 = binconv_module_fixed32<Buffer<>, FB>(conv2_2, conv2_2_top_shape, "2_3",
                                                            bn23m, bn23v, bn23w, bn23b, c23w, c23a, c23b, c23w_shape, conv2_3_top_shape);
        schedule(conv2_3, to_expr(conv2_3_top_shape));

        // Pool2(3x3, 2): (192, 16, 16, n) -> (192, 8, 8, n)
        Func pool2("pool2");
        std::vector<int32_t> pool2_top_shape;
        pool2(c, x, y, n) = avgpool_fixed32<FB>(conv2_3, {3, 3}, 2, 1, conv2_3_top_shape, pool2_top_shape)(c, x, y, n);


        // Module3_1(192x3x3x192): (192, 8, 8, n) -> (192, 8, 8, n)
        std::vector<int32_t> conv3_1_top_shape;
        Func conv3_1 = binconv_module_fixed32<Buffer<>, FB>(pool2, pool2_top_shape, "3_1",
                                                            bn31m, bn31v, bn31w, bn31b, c31w, c31a, c31b, c31w_shape, conv3_1_top_shape);

        // Module3_2(192x1x1x192): (192, 8, 8, n) -> (192, 8, 8, n)
        std::vector<int32_t> conv3_2_top_shape;
        Func conv3_2 = binconv_module_fixed32<Buffer<>, FB>(conv3_1, conv3_1_top_shape, "3_2",
                                                            bn32m, bn32v, bn32w, bn32b, c32w, c32a, c32b, c32w_shape, conv3_2_top_shape);

        // Bn3_3
        Func bn3_3("bn3_3");
        std::vector<int32_t> bn3_3_top_shape;
        bn3_3(c, x, y, n) = bn_fixed32<Buffer<>, FB>(conv3_2, bn33m, bn33v, conv3_2_top_shape, bn3_3_top_shape)(c, x, y, n);

        // Lq3_3
        Func lq3_3("lq3_3");
        std::vector<int32_t> lq3_3_top_shape;
        lq3_3(c, x, y, n) = log_quant_fixed32<FB>(bn3_3, bn3_3_top_shape, lq3_3_top_shape)(c, x, y, n);
        schedule(lq3_3, to_expr(lq3_3_top_shape));

        // Conv3_3(192x1x1x10): (192, 8, 8, n) -> (10, 8, 8, n)
        Func conv3_3("conv3_3");
        std::vector<int32_t> conv3_3_top_shape;
        conv3_3(c, x, y, n) = conv_qq_fixed32<Buffer<>, FB>(lq3_3, c33wq, c33b, c33w_shape, lq3_3_top_shape, conv3_3_top_shape)(c, x, y, n);

        // ReLU3_3:
        Func relu3_3("relu3_3");
        std::vector<int32_t> relu3_3_top_shape;
        relu3_3(c, x, y, n) = relu(conv3_3, conv3_3_top_shape, relu3_3_top_shape)(c, x, y, n);
        schedule(relu3_3, to_expr(relu3_3_top_shape));

        // Pool3(8x8, 1): (10, 8, 8, n) -> (10, 1, 1, n)
        Func pool3("pool3");
        std::vector<int32_t> pool3_top_shape;
        pool3(c, n) = global_avgpool_fixed32<FB>(relu3_3, relu3_3_top_shape, pool3_top_shape)(c, n);

        // tofloat:
        Func tof("tof");
        std::vector<int32_t> tof_top_shape;
        tof(c, n) = tofloat<FB>(pool3, pool3_top_shape, tof_top_shape)(c, n);
        schedule(tof, to_expr(tof_top_shape));

        return tof;
    }
};
HALIDE_REGISTER_GENERATOR(CIFAR10, cifar10)

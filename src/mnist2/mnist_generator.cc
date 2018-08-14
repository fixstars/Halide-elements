#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <Halide.h>
#include <Element2.h>

using namespace Halide;
using namespace Halide::Element;

typedef float ElemT;

class MNIST : public Halide::Generator<MNIST> {
    ImageParam in{Int(32), 4, "in"};

    GeneratorParam<int32_t> batch_size{"batch_size", 1};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        const std::string param_name = "data/LeNet.bin";

        Net net({
                Conv<ElemT>("conv1", 5, 20),
                BatchNorm<ElemT>("bn1"),
                Relu("relu1"),
                Pool("pool1", 2, 2),
                Conv<ElemT>("conv2", 5, 50),
                BatchNorm<ElemT>("bn2"),
                Relu("relu2"),
                Pool("pool2", 2, 2),
                Linear<ElemT>("ip3", 500),
                BatchNorm<ElemT>("bn3"),
                Relu("relu3"),
                Linear<ElemT>("ip4", 10)
            });

        net.setup(in, input_shape);
        net.load(param_name);

        return net.output();
    }
};
HALIDE_REGISTER_GENERATOR(MNIST, mnist)

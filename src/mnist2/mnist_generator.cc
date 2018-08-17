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
    ImageParam in{Float(32), 4, "in"};

    GeneratorParam<int32_t> batch_size{"batch_size", 1};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        const std::string param_name = "data/LeNet.bin";

        Net net("LeNet-5");

        net.add_layer("Conv", "conv1", 5, 20);
        net.add_layer("BatchNorm", "bn1");
        net.add_layer("Relu", "relu1");
        net.add_layer("Pool", "pool1", 2, 2);
        net.add_layer("BatchNorm", "bn2");
        net.add_layer("Conv", "bn1", 5, 50);
        net.add_layer("Relu", "relu2");
        net.add_layer("Pool", "pool2", 2, 2);
        net.add_layer("BatchNorm", "bn3");
        net.add_layer("Linear", "ip3", 500);
        net.add_layer("Relu", "relu3");
        net.add_layer("Linear", "ip4", 10);

        net.setup(in, input_shape);
        net.load(param_name);

        return net.output();
    }
};

HALIDE_REGISTER_GENERATOR(MNIST, mnist)

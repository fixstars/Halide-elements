#include <fstream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <Halide.h>
#include <Element2.h>

using namespace Halide;
using namespace Halide::Element;
using namespace Halide::Element::Cnn;

typedef float ElemT;

class MNIST : public Halide::Generator<MNIST> {
    ImageParam in{Float(32), 4, "in"};

    GeneratorParam<int32_t> batch_size{"batch_size", 128};

public:
    Func build()
    {
        const std::vector<int32_t> input_shape{1, 28, 28, batch_size};
        // const std::string param_name = "data/LeNet.bin";
        const std::string param_name = "data/LeNet_binarize.bin";

        Net net("LeNet-5");
        Type type = Float(32);

        net.add_layer("Conv", "conv1", type, 5, 20, 1, 0, true);
        net.add_layer("BatchNorm", "bn1", type);
        net.add_layer("Relu", "relu1", type);
        net.add_layer("Pool", "pool1", type, 2, 2);
        net.add_layer("BatchNorm", "bn2", type);
        net.add_layer("Scale", "scale2", type);
        net.add_layer("BinActive", "active2", type);
        net.add_layer("BinConv", "conv2", type, 5, 50, 1, 0, true);
        net.add_layer("Relu", "relu2", type);
        net.add_layer("Pool", "pool2", type, 2, 2);
        net.add_layer("BatchNorm", "bn3", type);
        net.add_layer("Scale", "scale3", type);
        net.add_layer("BinActive", "active3", type);
        net.add_layer("BinLinear", "ip3", type, 500);
        net.add_layer("Relu", "relu3", type);
        net.add_layer("Linear", "ip4", type, 10);
        net.add_layer("Softmax", "prob", type);

        net.setup(in, input_shape);
        net.auto_schedule();
        net.load(param_name);
        net.print_info();

        return net.output();
    }
};

HALIDE_REGISTER_GENERATOR(MNIST, mnist)

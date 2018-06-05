#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Sobel : public Generator<Sobel<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam input{type_of<T>(), 2, "input"};

    Func build() {
        Func output{"output"};

        output = Element::sobel<T>(input, width, height);

        schedule(input, {width, height});
        schedule(output, {width, height});

        return output;
    }

};

HALIDE_REGISTER_GENERATOR(Sobel<uint8_t>, sobel_u8);
HALIDE_REGISTER_GENERATOR(Sobel<uint16_t>, sobel_u16);


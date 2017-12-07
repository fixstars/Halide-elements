#include "Halide.h"
#include "Element.h"
#include "ImageProcessing.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Sobel : public Generator<Sobel<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam input{type_of<T>(), 2, "input"};

    Func build() {
        return sobel<T>(input, width, height);
    }

};

RegisterGenerator<Sobel<uint8_t>> sobel_u8{"sobel_u8"};
RegisterGenerator<Sobel<uint16_t>> sobel_u16{"sobel_u16"};


#include "Halide.h"
#include "Element.h"
#include "ImageProcessing.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Prewitt : public Generator<Prewitt<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam input{type_of<T>(), 2, "input"};

    Func build() {
        return prewitt<T>(input, width, height);
    }

};

RegisterGenerator<Prewitt<uint8_t>> prewitt_u8{"prewitt_u8"};
RegisterGenerator<Prewitt<uint16_t>> prewitt_u16{"prewitt_u16"};


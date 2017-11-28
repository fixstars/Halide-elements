#include <cstdint>

#include "Halide.h"
#include "Element.h"
#include "ImageProcessing.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Prewitt : public Generator<Prewitt<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func output{"output"};

        output = Element::prewitt<T>(input, width, height);

        schedule(input, {width, height});
        schedule(output, {width, height});

        return output;
    }

};

RegisterGenerator<Prewitt<uint8_t>> prewitt_u8{"prewitt_u8"};
RegisterGenerator<Prewitt<uint16_t>> prewitt_u16{"prewitt_u16"};

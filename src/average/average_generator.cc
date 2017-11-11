#include <iostream>
#include <limits>
#include "Halide.h"

#include "Element.h"

using namespace Halide;

template<typename T>
class Average : public Halide::Generator<Average<T>> {
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> window_width{"window_width", 3};
    GeneratorParam<int32_t> window_height{"window_height", 3};
    ImageParam src0{type_of<T>(), 2, "src0"};

public:
    Func build() {
        return Element::average<T>(src0, window_width, window_height);
    }
};

RegisterGenerator<Average<uint8_t>> average_u8{"average_u8"};
RegisterGenerator<Average<uint16_t>> average_u16{"average_u16"};
//RegisterGenerator<Average<uint32_t>> average_u32{"average_u32"};




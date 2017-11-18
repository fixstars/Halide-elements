#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Equal : public Halide::Generator<Equal<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {
        return equal<T>(src0, src1);
    }
};

RegisterGenerator<Equal<uint8_t>> equal_u8{"equal_u8"};
RegisterGenerator<Equal<uint16_t>> equal_u16{"equal_u16"};
RegisterGenerator<Equal<uint32_t>> equal_u32{"equal_u32"};

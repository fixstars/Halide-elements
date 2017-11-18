#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Cmpge : public Halide::Generator<Cmpge<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Func build() {
        return cmpge<T>(src0, src1);
    }
};

RegisterGenerator<Cmpge<uint8_t>> cmpge_u8{"cmpge_u8"};
RegisterGenerator<Cmpge<uint16_t>> cmpge_u16{"cmpge_u16"};
RegisterGenerator<Cmpge<uint32_t>> cmpge_u32{"cmpge_u32"};

#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Nand : public Halide::Generator<Nand<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src0{type_of<T>(), 2, "src0"}, src1{type_of<T>(), 2, "src1"};

    Var x, y;

    Func build() {
        return nand<T>(src0, src1);
    }
};

RegisterGenerator<Nand<uint8_t>> nand_u8{"nand_u8"};
RegisterGenerator<Nand<uint16_t>> nand_u16{"nand_u16"};
RegisterGenerator<Nand<uint32_t>> nand_u32{"nand_u32"};

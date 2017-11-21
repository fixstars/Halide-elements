#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Merge4 : public Halide::Generator<Merge4<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};
    ImageParam src3{type_of<T>(), 2, "src3"};

    Func build() {
        Func dst = merge4(src0, src1, src2, src3, width, height);
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(src3, {width, height});
        schedule(dst, {4, width, height});

        return dst;
    }
};

RegisterGenerator<Merge4<uint8_t>> merge4_u8{"merge4_u8"};
RegisterGenerator<Merge4<uint16_t>> merge4_u16{"merge4_u16"};
RegisterGenerator<Merge4<int8_t>> merge4_i8{"merge4_i8"};
RegisterGenerator<Merge4<int16_t>> merge4_i16{"merge4_i16"};

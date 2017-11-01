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
        Var x{"x"}, y{"y"}, c{"c"};
        Func dst{"dst"};
        dst(x, y, c) = cast<T>(0);
        dst(x, y, 0) = src0(x, y);
        dst(x, y, 1) = src1(x, y);
        dst(x, y, 2) = src2(x, y);
        dst(x, y, 3) = src3(x, y);
        
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(src3, {width, height});
        schedule(dst, {width, height, 4});
        
        return dst;
    }
};

RegisterGenerator<Merge4<uint8_t>> merge4_u8{"merge4_u8"};
RegisterGenerator<Merge4<uint16_t>> merge4_u16{"merge4_u16"};
RegisterGenerator<Merge4<int8_t>> merge4_i8{"merge4_i8"};
RegisterGenerator<Merge4<int16_t>> merge4_i16{"merge4_i16"};

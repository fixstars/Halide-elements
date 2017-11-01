#include <iostream>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Merge3 : public Halide::Generator<Merge3<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    
    ImageParam src0{type_of<T>(), 2, "src0"};
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};

    Func build() {
        Var x{"x"}, y{"y"}, c{"c"};
        Func dst{"dst"};
        dst(x, y, c) = cast<T>(0);
        dst(x, y, 0) = src0(x, y);
        dst(x, y, 1) = src1(x, y);
        dst(x, y, 2) = src2(x, y);
        
        schedule(src0, {width, height});
        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(dst, {width, height, 3});
        
        return dst;
    }
};

RegisterGenerator<Merge3<uint8_t>> merge3_u8{"merge3_u8"};
RegisterGenerator<Merge3<uint16_t>> merge3_u16{"merge3_u16"};
RegisterGenerator<Merge3<int8_t>> merge3_i8{"merge3_i8"};
RegisterGenerator<Merge3<int16_t>> merge3_i16{"merge3_i16"};

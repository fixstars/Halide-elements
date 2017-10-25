#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Multiply : public Halide::Generator<Multiply<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src1{type_of<T>(), 2, "src1"};
    ImageParam src2{type_of<T>(), 2, "src2"};
    
    Func build() {
        Var x{"x"}, y{"y"};

        Func dst("dst");
        dst(x, y) = src1(x, y) * src2(x, y);

        schedule(src1, {width, height});
        schedule(src2, {width, height});
        schedule(dst, {width, height});
        
        return dst;
    }
};

RegisterGenerator<Multiply<uint8_t>> multiply_u8{"multiply_u8"};
RegisterGenerator<Multiply<uint16_t>> multiply_u16{"multiply_u16"};
RegisterGenerator<Multiply<uint32_t>> multiply_u32{"multiply_u32"};

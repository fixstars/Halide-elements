#include <Halide.h>
#include <Element.h>

using namespace Halide;
using namespace Halide::Element;
   
class Multiply : public Halide::Generator<Multiply> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam src1{UInt(8), 2, "src1"};
    ImageParam src2{UInt(8), 2, "src2"};
    
    Var x{"x"}, y{"y"};
    
    Func build() {
        Func dst("dst");
        dst(x, y) = src1(x, y) * src2(x, y);
        
        return dst;
    }
};

RegisterGenerator<Multiply> multiply{"multiply"};

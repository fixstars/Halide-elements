#include <cstdint>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class Bilateral : public Halide::Generator<Bilateral<T>> {
public:
    ImageParam src{type_of<T>(), 2, "src"};
    Param<int32_t> window_size{"window_size", 1};
    Param<double> sigma_color{"sigma_color", 1};
    Param<double> sigma_space{"sigma_space", 1};


    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};

    Func build() {
        Func test{"test"};
        test = Element::bilateral<T>(src, width, height, window_size, sigma_color, sigma_space);
        schedule(src, {width, height});
        schedule(test, {width, height});

        ////////////////////////////////////////////////////////////////////////
        // To detect where "dst" causes the error, temporary dst name here is test_common
        // IT NEEDS TO BE CHANGED BEFORE MERGE REQUEST
        ////////////////////////////////////////////////////////////////////////
        return test;
    }
};

RegisterGenerator<Bilateral<uint8_t>> bilateral_u8{"bilateral_u8"};
RegisterGenerator<Bilateral<uint16_t>> bilateral_u16{"bilateral_u16"};

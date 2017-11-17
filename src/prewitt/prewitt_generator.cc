#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

template<typename T>
class Prewitt : public Generator<Prewitt<T>> {
public:
    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    ImageParam input{type_of<T>(), 2, "input"};

    Var x{"x"}, y{"y"};

    Func build() {
        Func input_f("input_f");
        input_f(x, y) = cast<float>(input(x, y));

        Func clamped = BoundaryConditions::repeat_edge(input_f, {{0, cast<int32_t>(width)}, {0, cast<int32_t>(height)}});

        Func diff_x("diff_x"), diff_y("diff_y");
        diff_x(x, y) = -clamped(x-1, y-1) + clamped(x+1, y-1) +
                       -clamped(x-1, y  ) + clamped(x+1, y  ) +
                       -clamped(x-1, y+1) + clamped(x+1, y+1);

        diff_y(x, y) = -clamped(x-1, y-1) + clamped(x-1, y+1) +
                       -clamped(x  , y-1) + clamped(x  , y+1) +
                       -clamped(x+1, y-1) + clamped(x+1, y+1);

        Func output("output");
        output(x, y) = cast<T>(hypot(diff_x(x, y), diff_y(x, y)));

        schedule(input_f, {width, height});
        schedule(diff_x, {width, height});
        schedule(diff_y, {width, height});
        schedule(output, {width, height});

        return output;
    }

};

RegisterGenerator<Prewitt<uint8_t>> prewitt_u8{"prewitt_u8"};
RegisterGenerator<Prewitt<uint16_t>> prewitt_u16{"prewitt_u16"};


#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class CloseCross : public Halide::Generator<CloseCross<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

	Func build() {
		Func dilate_cross{"dilate_cross"}, erode_cross{"erode_cross"};

		// Run dilate
		dilate_cross = Element::dilate_cross<T>(input, width, height, window_width, window_height, iteration);

		// Run erode
		erode_cross = Element::erode_cross<T>(dilate_cross, width, height, window_width, window_height, iteration);

		schedule(input, {width, height});
		schedule(dilate_cross, {width, height});
		schedule(erode_cross, {width, height});

		return erode_cross;
	}
};

HALIDE_REGISTER_GENERATOR(CloseCross<uint8_t>, close_cross_u8);
HALIDE_REGISTER_GENERATOR(CloseCross<uint16_t>, close_cross_u16);

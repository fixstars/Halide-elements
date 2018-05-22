#include <cstdint>
#include <Halide.h>

#include "Element.h"

using namespace Halide;
using Halide::Element::schedule;

template<typename T>
class CloseRect : public Halide::Generator<CloseRect<T>> {
public:
    ImageParam input{type_of<T>(), 2, "input"};

    GeneratorParam<int32_t> width{"width", 1024};
    GeneratorParam<int32_t> height{"height", 768};
    GeneratorParam<int32_t> iteration{"iteration", 2};
    GeneratorParam<int32_t> window_width{"window_width", 3, 3, 17};
    GeneratorParam<int32_t> window_height{"window_height", 3, 3, 17};

	Func build() {
		Func dilate_rect{"dilate_rect"}, erode_rect{"erode_rect"};

		// Run dilate
		dilate_rect = Element::dilate_rect<T>(input, width, height, window_width, window_height, iteration);

		// Run erode
		erode_rect = Element::erode_rect<T>(dilate_rect, width, height, window_width, window_height, iteration);

		schedule(input, {width, height});
		schedule(dilate_rect, {width, height});
		schedule(erode_rect, {width, height});

		return erode_rect;
	}
};

HALIDE_REGISTER_GENERATOR(CloseRect<uint8_t>, close_rect_u8);
HALIDE_REGISTER_GENERATOR(CloseRect<uint16_t>, close_rect_u16);

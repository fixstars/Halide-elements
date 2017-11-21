#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "add_u8.h"
#include "add_u16.h"
#include "add_u32.h"

#include "test_common.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer0, struct halide_buffer_t *_src_buffer1, struct halide_buffer_t *_dst_buffer))
{
    try {
        using upper_t = typename Halide::Element::Upper<T>::type;

        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input0 = mk_rand_buffer<T>(extents);
        auto input1 = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input0, input1, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                upper_t f = static_cast<upper_t>(input0(x, y)) + static_cast<upper_t>(input1(x, y));
                f = std::min(f, static_cast<upper_t>(std::numeric_limits<T>::max()));
                T expect = f;
                T actual = output(x, y);

                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, expect, x, y, actual).c_str());
                }
            }

        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(add_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(add_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(add_u32);
#endif
}

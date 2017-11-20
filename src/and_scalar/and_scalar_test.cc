#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "and_scalar_u8.h"
#include "and_scalar_u16.h"
#include "and_scalar_u32.h"

#include "test_common.h"

using namespace Halide::Runtime;

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, T _value, struct halide_buffer_t *_dst_buffer)) {
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const T value = mk_rand_scalar<T>();
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, value, output);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t expect = input(x, y) & value;
                uint8_t actual = output(x, y);

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


int main(int argc, char **argv) {
#ifdef TYPE_u8
    test<uint8_t>(and_scalar_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(and_scalar_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(and_scalar_u32);
#endif
}

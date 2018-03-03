#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "set_scalar_u8.h"
#include "set_scalar_u16.h"

#include "test_common.h"

using namespace Halide::Runtime;

template<typename T>
int test(int (*func)(T _value, struct halide_buffer_t *_dst_buffer)) {
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        const T value = mk_rand_scalar<T>(); //input scalar
        auto output = mk_null_buffer<T>(extents);

        func(value, output);
        //for each x and y
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                if (value != output(x, y)) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, value, x, y, output(x, y)).c_str());
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
    test<uint8_t>(set_scalar_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(set_scalar_u16);
#endif

}

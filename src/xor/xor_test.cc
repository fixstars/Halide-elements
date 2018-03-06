#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "xor_u8.h"
#include "xor_u16.h"
#include "xor_u32.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src0_buffer,
                        struct halide_buffer_t *_src1_buffer,
                        struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input0 = mk_rand_buffer<T>(extents);
        auto input1 = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input0, input1, output);
        //for each x and y
        for (int j=0; j<height; ++j) {
            for (int i=0; i<width; ++i) {
                T expect = (input0(i, j) ^ input1(i, j));
                T actual = output(i, j);
                if (expect != actual) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                i, j, expect, i, j, actual));
                }
            }
        }

    } catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}

int main()
{
#ifdef TYPE_u8
    test<uint8_t>(xor_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(xor_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(xor_u32);
#endif
}

#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "subimage_u8.h"
#include "subimage_u16.h"
#include "subimage_u32.h"

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     uint32_t _origin_x, uint32_t _origin_y,
                     struct halide_buffer_t *_dst_buffer))
{
    try {
        const int32_t in_width = 1024;
        const int32_t in_height = 768;

        const int32_t out_width = 500;
        const int32_t out_height = 500;

        uint32_t xLimit = in_width - out_width;
        uint32_t yLimit = in_height - out_height;
        const uint32_t origin_x = mk_rand_scalar<int32_t>() % xLimit;
        const uint32_t origin_y = mk_rand_scalar<int32_t>() % yLimit;

        const std::vector<int32_t> in_extents{in_width, in_height};
        const std::vector<int32_t> out_extents{out_width, out_height};
        auto input = mk_rand_buffer<T>(in_extents);
        auto output = mk_null_buffer<T>(out_extents);
        auto expect = mk_null_buffer<T>(out_extents);

        for(int i = 0; i < out_height; i++){
            for (int j = 0; j < out_width; j++){
                expect(j, i) = input(origin_x + j, origin_y + i);
            }
        }

        func(input, origin_x, origin_y, output);
        //for each x and y
        for (int i=0; i<out_height; ++i) {
            for (int j=0; j<out_width; ++j) {
                if (expect(j, i) != output(j, i)) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                j, i, expect(j, i), j, i, output(j, i)));
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
    test<uint8_t>(subimage_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(subimage_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(subimage_u32);
#endif
}

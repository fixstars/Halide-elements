#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "min_pos_u8.h"
#include "min_pos_u16.h"
#include "min_pos_u32.h"
#include "min_pos_f32.h"
#include "min_pos_f64.h"
#include "test_common.h"

using std::string;
using std::vector;

template <typename T>
std::tuple<uint32_t, uint32_t>
min_pos_ref(const Halide::Runtime::Buffer<T>& src, const int width, const int height) {
    T min = std::numeric_limits<T>::has_infinity
        ? std::numeric_limits<T>::infinity()
        : std::numeric_limits<T>::max();
    uint32_t min_x = 0, min_y = 0;
    
    for (int i = 0; i < height; i++) {
        T cur_min = min;
        int k = -1;
        for (int j = width - 1; j >= 0; j--) {
            if (src(j, i) <= cur_min) {
                cur_min = src(j, i);
                k = j;
            }
        }
        {
            if (k >= 0 && (cur_min < min ||
                           (cur_min == min && static_cast<uint32_t>(i) < min_y))) {
                min = cur_min;
                min_x = k;
                min_y = i;
            }
        }
    }
    return std::make_tuple(min_x, min_y);
}


template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_dst_buffer))
{
    try {
        int ret = 0;

        //
        // Run
        //
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<uint32_t>({2});

        func(input, output);

        uint32_t expect_x, expect_y;
        std::tie(expect_x, expect_y) = min_pos_ref<T>(input, width, height);
        uint32_t actual_x = output(0);
        uint32_t actual_y = output(1);
        if (expect_x != actual_x || expect_y != actual_y) {
            throw std::runtime_error(format("Error: expect = (%u, %u), actual = (%u, %u)",
                                            expect_x, expect_y, actual_x, actual_y));
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
    test<uint8_t>(min_pos_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(min_pos_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(min_pos_u32);
#endif
#ifdef TYPE_f32
    test<float>(min_pos_f32);
#endif
#ifdef TYPE_f64
    test<double>(min_pos_f64);
#endif
}

#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "min_value_u8.h"
#include "min_value_u16.h"
#include "min_value_u32.h"
#include "test_common.h"

using std::string;
using std::vector;

template <typename T>
T min_value_ref(const Halide::Runtime::Buffer<T>& src, const Halide::Runtime::Buffer<uint8_t>& roi, const int width, const int height) {

    T min = std::numeric_limits<T>::has_infinity
        ? std::numeric_limits<T>::infinity()
        : (std::numeric_limits<T>::max)();
    int count = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (roi(j, i) != 0) {
                T val = src(j, i);
                min = val < min ? val : min;
                count++;
            }
        }
    }
    return (count == 0) ? 0 : min;
}

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer, struct halide_buffer_t *_roi_buffer, struct halide_buffer_t *_dst_buffer))
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
        auto roi = mk_rand_buffer<uint8_t>(extents);
        auto output = mk_null_buffer<T>({1});

        func(input, roi, output);

        T expect = min_value_ref<T>(input, roi, width, height);
        T actual = output(0);
        if (expect != actual) {
            throw std::runtime_error(format("Error: expect = %u, actual = %u\n",
                                            static_cast<uint64_t>(expect), static_cast<uint64_t>(actual)));
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
    test<uint8_t>(min_value_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(min_value_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(min_value_u32);
#endif
}

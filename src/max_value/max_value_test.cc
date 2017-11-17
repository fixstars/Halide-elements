#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "max_value_u8.h"
#include "max_value_u16.h"
#include "max_value_u32.h"
#include "test_common.h"

using std::string;
using std::vector;

template <typename T, bool has_infinity>
struct initial {};

template <typename T>
struct initial<T, true> {
  static T value(void) { return -std::numeric_limits<T>::infinity(); }
};

template <typename T>
struct initial<T, false> {
  static T value(void) { return (std::numeric_limits<T>::min)(); }
};

template <typename T>
T max_value_ref(const Halide::Runtime::Buffer<T>& src, const Halide::Runtime::Buffer<uint8_t>& roi, const int width, const int height) {

    T max = initial<T, std::numeric_limits<T>::has_infinity>::value();
    int count = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (roi(j, i) != 0) {
                T val = src(j, i);
                max = val > max ? val : max;
                count++;
            }
        }
    }
    return (count == 0) ? 0 : max;
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

        T expect = max_value_ref<T>(input, roi, width, height);
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
    test<uint8_t>(max_value_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(max_value_u16);
#endif
#ifdef TYPE_u32
    test<uint32_t>(max_value_u32);
#endif
}

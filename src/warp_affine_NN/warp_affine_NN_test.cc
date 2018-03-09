#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <climits>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "warp_affine_NN_u8.h"
#include "warp_affine_NN_u16.h"

#include "test_common.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

template<typename T>
T interpolateNN(const Halide::Runtime::Buffer<T>& data, const int width, const int height,
                float x, float y, T border_value, const int border_type)
{
    if(x != x){x=0;}
    if(y != y){y=0;}

    int i = static_cast<int>(floor(y));
    int j = static_cast<int>(floor(x));

    if (i>=0 && j >=0 && i < height && j < width){
        return data(j, i);
    }else if(border_type == 1){
        i = BORDER_INTERPOLATE(i, height);
        j = BORDER_INTERPOLATE(j, width);
        return data(j, i);
    }else{
        assert(border_type==0);
        return border_value;
    }
}


template<typename T>
Halide::Runtime::Buffer<T>& NN_ref(Halide::Runtime::Buffer<T>& dst,
                                const Halide::Runtime::Buffer<T>& src,
                                const int32_t width, const int32_t height,
                                const T border_value, const int32_t border_type,
                                const Halide::Runtime::Buffer<double>& transform)
{
    /* avoid overflow from X-1 to X+2 */
    float imin = static_cast<float>((std::numeric_limits<int>::min)() + 1);
    float imax = static_cast<float>((std::numeric_limits<int>::max)() - 2);

    for(int i = 0; i < height; ++i){
        float org_y = static_cast<float>(i) + 0.5f;
        float src_x0 = static_cast<float>(transform(2)) +
                       static_cast<float>(transform(1)) * org_y;
        float src_y0 = static_cast<float>(transform(5)) +
                       static_cast<float>(transform(4)) * org_y;
        for(int j = 0; j < width; ++j){
            float org_x = static_cast<float>(j) + 0.5f;
            float src_x = src_x0 + static_cast<float>(transform(0)) * org_x;
            float src_y = src_y0 + static_cast<float>(transform(3)) * org_x;

            src_x = std::max(imin, std::min(imax, src_x));
            src_y = std::max(imin, std::min(imax, src_y));

            dst(j, i) = interpolateNN(src, width, height, src_x, src_y, border_value, border_type);
        }
    }

    return dst;
}


template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     T _border_value, struct halide_buffer_t *_transform,
                     struct halide_buffer_t *_dst_buffer))

{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        const std::vector<int32_t> tableSize{6};
        const T border_value = mk_rand_scalar<T>();
        const int32_t border_type = 1; // 0 or 1
        auto transform = mk_rand_buffer<double>(tableSize);
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, border_value, transform, output);
        auto expect = mk_null_buffer<T>(extents);
        expect = NN_ref(expect, input, width, height, border_value, border_type, transform);

        //for each x and y
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                if (expect(x, y) != output(x, y)) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d",
                                                    x, y, expect(x, y), x, y, output(x, y)).c_str());
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
    test<uint8_t>(warp_affine_NN_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(warp_affine_NN_u16);
#endif

}

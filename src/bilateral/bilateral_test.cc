#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>

#include "HalideRuntime.h"
#include "HalideBuffer.h"
#include "test_common.h"

#include "bilateral_u8.h"
#include "bilateral_u16.h"

#define BORDER_INTERPOLATE(x, l) (x < 0 ? 0 : (x >= l ? l - 1 : x))

static double get_weight_d(double dx, double dy, const double sigma_space){
    return static_cast<double>(std::exp(-0.5 * (dx * dx + dy * dy) / (sigma_space * sigma_space)));
}

static double get_weight_r(double fo, double f, const double sigma_color){
    return static_cast<double>(std::exp(-0.5 * (fo - f) * (fo - f) / (sigma_color * sigma_color)));
}

template<typename T>
Halide::Runtime::Buffer<T>& bilateral_ref(Halide::Runtime::Buffer<T>& dst,
                                const Halide::Runtime::Buffer<T>& src,
                                const int32_t width, const int32_t height,
                                const int32_t window_size,
                                const double sigma_color, const double sigma_space)
{
    const int wRadius = window_size/2;

    int buffsize = window_size*window_size;
    if(width*height > (std::numeric_limits<T>::max)()){
        buffsize += (std::numeric_limits<T>::max)();
    }
    double* kernel_d = new double[buffsize];

    for(int i =0; i<window_size; i++){
        for(int j=0; j<window_size; j++){
            double r = std::sqrt(
                (double)(i-wRadius) * (i-wRadius) +
                (double)(j-wRadius) * (j-wRadius)
            );
            if(r > wRadius){
                kernel_d[i * window_size + j] = 0;
            }else{
                kernel_d[i * window_size + j] = get_weight_d(
                                                    static_cast<double>(i-wRadius),
                                                    static_cast<double>(j-wRadius),
                                                    sigma_space);
            }
        }
    }

    double* kernel_r = NULL;
    if(width*height > (std::numeric_limits<T>::max)()){
        kernel_r = kernel_d + window_size * window_size;
        const int kernel_size = (std::numeric_limits<T>::max)();
        for(int i = 0; i<=kernel_size; i++){
            kernel_r[i] = get_weight_r(i, 0, sigma_color);
        }
    }

    for(int i = 0; i<height; i++){
        for(int j = 0; j<width; j++){
            double sum_nume = 0;
            double sum_deno = 0;
            T original = src(j, i);
            for(int k =0; k< window_size; k++){
                int index_y = BORDER_INTERPOLATE(i + k - wRadius, height);
                for(int l =0; l<window_size; l++){
                    int index_x = BORDER_INTERPOLATE(j + l - wRadius, width);

                    T bri = src(index_x, index_y);

                    double weight_d = kernel_d[k * window_size + l];
                    double weight_r = kernel_r == NULL
                                        ? get_weight_r(static_cast<double>(original),
                                                       static_cast<double>(bri),
                                                       sigma_color)
                                        : kernel_r[original > bri ? (original - bri)
                                                                  : (bri - original)];

                    sum_nume += weight_d * weight_r * bri;
                    sum_deno += weight_d * weight_r;
                }
            }
            double num = sum_nume / sum_deno;

            if((num-floor(num)- 0.5) > (std::numeric_limits<float>::epsilon)()
                || (static_cast<T>(num))%2==1)
            {
                num += 0.5;
            }
            dst(j, i) = static_cast<T>(num);
        }
    }
    return dst;
}

template<typename T>
int test(int (*func)(struct halide_buffer_t *_src_buffer,
                     int32_t _window_size, double _color, double _space,
                     struct halide_buffer_t *_dst_buffer))
{
    try {
        const int width = 1024;
        const int height = 768;
        const std::vector<int32_t> extents{width, height};
        const int32_t window_size = 5;
        const double sigma_color = 2.0;
        const double sigma_space = mk_rand_scalar<double>();
        auto input = mk_rand_buffer<T>(extents);
        auto output = mk_null_buffer<T>(extents);

        func(input, window_size, sigma_color, sigma_space, output);

        auto expect = mk_null_buffer<T>(extents);
        expect = bilateral_ref(expect, input, width, height, window_size, sigma_color, sigma_space);
        // for each x and y
        for (int j=0; j<width; ++j) {
            for (int i=0; i<height; ++i) {
                if (abs(expect(j, i) - output(j, i)) > 0) {
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
    test<uint8_t>(bilateral_u8);
#endif
#ifdef TYPE_u16
    test<uint16_t>(bilateral_u16);
#endif
}

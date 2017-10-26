#include "affine.h"

#include "HalideBuffer.h"
#include "halide_image_io.h"
#include <string>
#include <cstdio>


int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: affine_test2 input.png degrees scale_x scale_y shift_x shift_y\n");
        printf("ex)    affine_test2 input.png 30 1 1 10 30\n");
        return 1;
    }

    std::string in_fname = std::string(argv[1]);
    float degrees = atof(argv[2]);
    float scale_x = atof(argv[3]);
    float scale_y = atof(argv[4]);
    float shift_x = atof(argv[5]);
    float shift_y = atof(argv[6]);
    float skew_x  = atof(argv[7]);

    Halide::Runtime::Buffer<uint8_t> input = Halide::Tools::load_image(in_fname.c_str());
    Halide::Runtime::Buffer<uint8_t> output(input.width(), input.height());

    affine(input, degrees, scale_x, scale_y, shift_x, shift_y, skew_x, output);

    Halide::Tools::save_image(output, "affine.png");

    return 0;
}


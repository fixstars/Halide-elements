#include <iostream>

#include "HalideRuntime.h"
#include "HalideBuffer.h"

#include "test_common.h"
#include "run_common.h"

#include "sgm.h"

using namespace Halide::Runtime;

int main(int argc, char **argv) {
    try {
        Buffer<uint8_t> in_l = load_pgm("data/left.pgm");
        Buffer<uint8_t> in_r = load_pgm("data/right.pgm");

        const int width = in_l.extent(0);
        const int height = in_l.extent(1);

        Buffer<uint8_t> out(width, height);

        sgm(in_l, in_r, out);
        
        Buffer<uint8_t> disp = load_pgm("data/disp.pgm");

        save_pgm("out_test.pgm", out.data(), width, height);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                uint8_t ev = disp(x, y);
                uint8_t av = out(x, y);
                if (ev != av) {
                    throw std::runtime_error(format("Error: expect(%d, %d) = %d, actual(%d, %d) = %d", x, y, ev, x, y, av).c_str());
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

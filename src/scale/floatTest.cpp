// On linux, you can compile and run it like so:
// g++ floatTest.cpp -g -I /home/momokok/halide/include -L /home/momokok/halide/bin -lHalide -lpthread -ldl -o floatTest -std=c++11
// LD_LIBRARY_PATH=/home/momokok/halide/bin ./floatTest


// The only Halide header file you need is Halide.h. It includes all of Halide.
#include "Halide.h"

// We'll also include stdio for printf.
#include <stdio.h>


int main(int argc, char **argv) {

    Halide::Func gradient;

    Halide::Var x, y;

    Halide::Expr e =  (Halide::cast<float>(x)+Halide::cast<float>(y))/Halide::cast<float>(0.312913f);
    Halide::Func test;
    Halide::Expr reint = Halide::reinterpret<int>(e);
    test(x, y) = print_when(x ==3&&y==2,reint);
    gradient(x, y) = e;
    //  test result
        // (x + y) * a  ->  (x + y) * a
        // (x + y) / a -> (x + y) * (1/a)
        // (x * a) + (y * a) -> (x + y) * a
        // (x + y) * b / c -> (x * b / c) + (x * b / c)
        // (x + a) * b -> (x * b) + (a * b)
        // (x + a) / b -> (x / b) + (a / b)

///////////////////////////////////////////////////////////////////////////////
// memo
///////////////////////////////////////////////////////////////////////////////
        // THe following code is from Simpligy.cpp
        // in void visit (const Div)
        //
        //   else if (const_float(a, &fa) &&
        //              const_float(b, &fb) &&
        //              fb != 0.0f) {
        //       expr = FloatImm::make(op->type, fa / fb);}
        //   //
        //   // in void visit (const Div *op)
        //   else if (b.type().is_float() && is_simple_const(b)) {
        //      // Convert const float division to multiplication
        //      // x / 2 -> x * 0.5
        //      expr = mutate(a * (make_one(b.type()) / b));
        //  }
        //  //
        //  else if (add_a &&
        //             !(add_a->b.as<Ramp>() && ramp_b) &&
        //             is_simple_const(add_a->b) &&
        //             is_simple_const(b)) {
        //      expr = mutate(add_a->a * b + add_a->b * b);
        //   //
        //   else if (const_float(a, &fa) && const_float(b, &fb)) {
        //      expr = FloatImm::make(a.type(), fa * fb);
        //  }
        //  //
        //  // the definition of const_float
        //  bool const_float(const Expr &e, double *f) {
        //     if (e.type().is_vector()) {
        //         return false;
        //     } else if (const double *p = as_const_float(e)) {
        //         *f = *p;
        //         return true;
        //     } else {
        //         return false;
        //     }
        // }




    test.realize(10, 10);
    Halide::Buffer<float> output = gradient.realize(4, 4);


    for (int j = 0; j < output.height(); j++) {
        for (int i = 0; i < output.width(); i++) {
            float result = (static_cast<float>(i)+static_cast<float>(j))/static_cast<float>(0.312913f);
            // We can access a pixel of an Buffer object using similar
            // syntax to defining and using functions.
            if (output(i, j) != result) {
                printf("Something went wrong!\n"
                       "Pixel %d, %d was supposed to be %f, but instead it's %f, ... %d\n",
                       i, j, result , output(i, j), reinterpret_cast<int&>(result));
                //return -1;
            }
        }
    }


    printf("memo from Simplify.cpp and test result is commented out!\n");

    return 0;
}

#include <iostream>
#include <cmath>
#include "Halide.h"
#include "Element.h"

using namespace Halide;
using namespace Halide::Element;

class Affine : public Generator<Affine> {
public:
    GeneratorParam<int32_t> width{"width", 768};
    GeneratorParam<int32_t> height{"height", 1280};
    ImageParam input{UInt(8), 2, "input"};
    Param<float> degrees{"degrees", 0.0f};
    Param<float> scale_x{"scale_x", 1.0f};
    Param<float> scale_y{"scale_y", 1.0f};
    Param<float> shift_x{"shift_x", 0.0f};
    Param<float> shift_y{"shift_y", 0.0f};
    Param<float> skew_y{"skew_y", 0.0f};

    Var x, y;

    Func build() {
        Expr cos_deg = cos(degrees * (float)M_PI / 180.0f);
        Expr sin_deg = sin(degrees * (float)M_PI / 180.0f);
        Expr tan_skew_y = tan(skew_y * (float)M_PI / 180.0f);
        Expr det = scale_x * scale_y;
        Expr a00 = scale_y * cos_deg;
        Expr a10 = scale_y * sin_deg;
        Expr a20 = - (a00 * shift_x + a10 * shift_y);
        Expr a01 = - scale_x * (sin_deg + cos_deg * tan_skew_y);
        Expr a11 =   scale_x * (cos_deg - sin_deg * tan_skew_y);
        Expr a21 = - (a01 * shift_x + a11 * shift_y);

        Func tx("tx"), ty("ty");
        tx(x,y) = cast<int>((a00*x + a10*y + a20) / det);
        ty(x,y) = cast<int>((a01*x + a11*y + a21) / det);

        Func affine("affine");
        Func limited = BoundaryConditions::constant_exterior(input, 255);
        affine(x, y) = limited(tx(x, y), ty(x, y));

        schedule(input, {width, height});
        schedule(tx, {width, height});
        schedule(ty, {width, height});
        schedule(affine, {width, height});
        
        return affine;
    }
};

RegisterGenerator<Affine> affine{"affine"};


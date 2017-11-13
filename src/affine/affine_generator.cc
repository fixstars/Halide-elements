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
        // This affine transformation applies these operations below for an input image.
        //   1. scale about the origin
        //   2. shear in y direction
        //   3. rotation about the origin
        //   4. translation
        //
        // The applied matrix M consists of multiplied matrix by each operation:
        //
        //     M = M_translation * M_rotation * M_shear_y * M_scale
        //
        // So we can write a formula using Y, X, and M for affine transformation,
        //
        //     Y = M * X
        //
        //   where Y is the output image and X is the input image.
        //
        // But in Halide, we consider the formula from the output coordinates, so
        // calculate the inverse of matrix M, then rewrite as follows:
        //
        //     X = M^(-1) * Y
        //
        // Now let's take a look at the detail as below.
        // At first, matrix M is divided to translation and the others, and decompose it like this.
        //
        //         [ A  b ]
        //     Y = [      ] * X
        //         [ 0  1 ]
        //
        //   where A is M_rotation * M_shear_y * M_scale, and b = M_translation.
        //
        // Then, rewrite to the inverse formula.
        //
        //         [ A^(-1) -A^(-1)*b ]
        //     X = [                  ] * Y
        //         [   0         1    ]
        //
        // Next A is originally the matrix as follows:
        //
        //     A = M_rotation * M_shear_y * M_scale
        //
        //         [ cos_deg -sin_deg ]   [  1          0 ]   [ scale_x   0     ]
        //       = [                  ] * [               ] * [                 ]
        //         [ sin_deg  cos_deg ]   [ tan_skew_y  1 ]   [  0      scale_y ]
        //
        //         [ scale_x*(cos_deg-sin_deg*tan_skew_y)  -scale_y*sin_deg ]
        //       = [                                                        ]
        //         [ scale_x*(sin_deg+cos_deg*tan_skew_y)   scale_y*cos_deg ]
        //
        // Now we can get A^(-1) as follows:
        //
        //               1    [  scale_y*cos_deg                      scale_y*sin_deg                      ]
        //     A^(-1) = --- * [                                                                            ]
        //              det   [ -scale_x*(sin_deg+cos_deg*tan_skew_y) scale_x*(cos_deg-sin_deg*tan_skew_y) ]
        //
        //  where det = scale_x*scale_y.
        //
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

        // Here X can be described by Y, A, and b like this.
        //
        //         [ A^(-1) -A^(-1)*b ]
        //     X = [                  ] * Y
        //         [   0         1    ]
        //
        //   where Y = (x,y) and X = (tx, ty).
        //
        Func tx("tx"), ty("ty");
        tx(x,y) = cast<int>((a00*x + a10*y + a20) / det);
        ty(x,y) = cast<int>((a01*x + a11*y + a21) / det);

        // CAUTION: the coordinates of the input image cannot be out of the original width and height.
        //          so they are limited and the outside is set to 255 which is a white color.
        //
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


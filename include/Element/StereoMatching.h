#pragma once

#include <Halide.h>
#include "Schedule.h"

namespace Halide {
namespace Element {
namespace {

Func addCost3(Func cost_ul, Func cost_u, Func cost_ur)
{
    Var d, x, y;
    Func f;
    f(d, x, y) = cost_ul(d, x, y) + cost_u(d, x, y) + cost_ur(d, x, y);
    return f;
}

Func disparity(Func cost, int32_t disp)
{
    Var x("x"), y("y");
    RDom r(0, disp);

    Expr e = cost(r, x, y);

    Func g("argmin");
    g(x, y) = Tuple(0, e.type().max());
    g(x, y) = tuple_select(e < g(x, y)[1], Tuple(r, e), g(x, y));

    g.unroll(x).unroll(y)
        .update().unroll(x).unroll(y).unroll(r[0]);

    Func f("disparity");
    f(x, y) = cast<uint8_t>(g(x, y)[0] * (UINT8_MAX+1) / disp);

    return f;
}

Func census(Func input, int32_t width, int32_t height, int32_t hori, int32_t vert)
{
  Var x("x"), y("y");
  const int32_t radh = hori/2, radv = vert/2;

  Func f("census");
  RDom rh(-radh, hori);
  RDom rv(-radv, vert);
  Expr rX = radh - rh;
  Expr rY = radv - rv;
  Expr vrX = select(rX > radh, rX - 1, rX);
  Expr vrY = select(rY > radh, rY - 1, rY);
  Expr shift = cast<uint64_t>(vrY * (hori-1) + vrX);
  Expr inside = x >= radh && x < width-radh && y >= radh && y < height-radh;

  Func in = BoundaryConditions::constant_exterior(input, 0, 0, width, 0, height);

  f(x, y) = select(inside,
                   sum_unroll(rh,
                              sum_unroll(rv, select(rh == 0 || rv == 0,
                                                    cast<uint64_t>(0),
                                                    select(in(x, y) > in(x+rh, y+rv),
                                                           cast<uint64_t>(1) << shift,
                                                           cast<uint64_t>(0))))),
                              cast<uint64_t>(0));

  return f;
}
 
Func census(Func input, int32_t width, int32_t height)
{
    return census(input, width, height, 9, 7);
}

Func matchingCost(Func left, Func right, int32_t width, int32_t height)
{
    Var x("x"), y("y"), d("d");

    Func f;
    Func r = BoundaryConditions::constant_exterior(right, 0, 0, width, 0, height);
    f(d, x, y) = cast<uint8_t>(popcount(left(x, y) ^ select((x-d) > 0, r(x-d, y), cast<uint64_t>(0))));

    return f;
}

template <int32_t RX, int32_t RY, bool FORWARD>
Func scanCost(Func cost, int32_t width, int32_t height, int32_t disp)
{
    Var x("x"), y("y"), d("d");

    Func lcost, f;
    lcost(d, x, y) = Tuple(
        cast<uint16_t>(0),
        cast<uint16_t>(0)
        );

    Expr PENALTY1 = cast<uint16_t>(20);
    Expr PENALTY2 = cast<uint16_t>(100);

    RDom r(0, disp, 0, width, 0, height);
    RVar rd = r[0];
    RVar rx = r[1];
    RVar ry = r[2];

    Expr bx = FORWARD ? rx : width-1 - rx;
    Expr by = FORWARD ? ry : height-1 - ry;
    Expr px = bx - RX;
    Expr py = by - RY;
    Expr inside = py >= 0 && py < height && px >= 0 && px < width;
    Expr outside_x = px < 0 || px >= width;
    Expr outside_y = py < 0 || py >= height;

    Expr minCost = select(outside_x, 0, outside_y, 0, likely(lcost(disp-1, px, py)[1]));

    Expr cost0 = select(outside_x, 0, outside_y, 0, lcost(rd, px, py)[0]);
    Expr cost1 = select(rd-1 < 0, INT32_MAX, outside_x, 0, outside_y, 0, likely(lcost(rd-1, px, py)[0]) + PENALTY1);
    Expr cost2 = select(rd+1 >= disp, INT32_MAX, outside_x, 0, outside_y, 0, likely(lcost(rd+1, px, py)[0]) + PENALTY1);
    Expr cost3 = minCost + PENALTY2;
    Expr pen = min( min(cost0, cost1), min(cost2, cost3) );

    Expr newCost = select(inside,
                            cast<uint16_t>(cost(rd, bx, by) + pen - minCost),
                            cast<uint16_t>(cost(rd, bx, by))
                            );

    lcost(rd, bx, by) = Tuple(
        newCost,
        cast<uint16_t>(select(rd-1 < 0,
                                newCost,
                                likely(lcost(rd-1, bx, by)[1]) > newCost, newCost, likely(lcost(rd-1, bx, by)[1])
                                )
                        )
        );

    schedule(lcost, {disp, width, height})
        .unroll(d)
        .update().unroll(rd).allow_race_conditions();

    f(d, x, y) = lcost(d, x, y)[0];

    return f;
}

Func semi_global_matching(Func in_l, Func in_r, int32_t width, int32_t height, int32_t disp)
{
    Var d, x, y;
    Func f0_l("census_left");
    f0_l(x, y) = census(lambda(_, in_l(_)), width, height)(x, y);

    Func f0_r("census_right");
    f0_r(x, y) = census(lambda(_, in_r(_)), width, height)(x, y);

    Func f1("matching_cost");
    f1(d, x, y) = matchingCost(f0_l, f0_r, width, height)(d, x, y);

    Func f2_ul("scan_cost_ul");
    f2_ul(d, x, y) = scanCost<1, 1, true>(f1, width, height, disp)(d, x, y);

    Func f2_u("scan_cost_u");
    f2_u(d, x, y) = scanCost<0, 1, true>(f1, width, height, disp)(d, x, y);

    Func f2_ur("scan_cost_ur");
    f2_ur(d, x, y) = scanCost<-1, 1, true>(f1, width, height, disp)(d, x, y);

    Func f3("add_cost");
    f3(d, x, y) = addCost3(f2_ul, f2_u, f2_ur)(d, x, y);

    Func f4("disparity");
    f4(x, y) = disparity(f3, disp)(x, y);

    Func out("out");
    out(x, y) = f4(x, y);

    schedule(f0_l, {width, height});
    schedule(f0_r, {width, height});
    schedule(f1, {disp, width, height}).unroll(d);
    schedule(f3, {disp, width, height}).unroll(d);

    return out;
}

} // anonymous
} // Element
} // Halide

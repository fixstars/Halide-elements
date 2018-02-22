## note about RDom

### basic code
```
Halide::Func gradient;
Halide::Var x, y;
Halide::Expr e = x + y;
gradient(x, y) = e;

Halide::Buffer<int32_t> output = gradient.realize(10, 10);

//print
for (int j = 0; j < output.height(); j++) {
    for (int i = 0; i < output.width(); i++) {
             printf("%2d ",output(i, j));

    }
    printf("\n");
}
return 0;
```

output:
```
0  1  2  3  4  5  6  7  8  9
1  2  3  4  5  6  7  8  9 10
2  3  4  5  6  7  8  9 10 11
3  4  5  6  7  8  9 10 11 12
4  5  6  7  8  9 10 11 12 13
5  6  7  8  9 10 11 12 13 14
6  7  8  9 10 11 12 13 14 15
7  8  9 10 11 12 13 14 15 16
8  9 10 11 12 13 14 15 16 17
9 10 11 12 13 14 15 16 17 18

```

### Add rdom that has same size as input (10x10)
Add tjos before printing part
```
Halide::RDom r{0, 10, 0, 10, "r"};
r.where(r. y == 3);
gradient(r.x, r.y) = gradient(r.x, r.y) * 2;
```

output:
```
0  1  2  3  4  5  6  7  8  9
1  2  3  4  5  6  7  8  9 10
2  3  4  5  6  7  8  9 10 11
6  8 10 12 14 16 18 20 22 24
4  5  6  7  8  9 10 11 12 13
5  6  7  8  9 10 11 12 13 14
6  7  8  9 10 11 12 13 14 15
7  8  9 10 11 12 13 14 15 16
8  9 10 11 12 13 14 15 16 17
9 10 11 12 13 14 15 16 17 18
```
Only y = 3 has changed

### Update rdom
removed the previous code and insert this
```
Halide::RDom r{0, 10, 0, 10, "r"};
r.where(r.y == 3);
r.where(r.x == 3);
gradient(r.x, r.y) = gradient(r.x, r.y) * 2;
```
output:
```
0  1  2  3  4  5  6  7  8  9
1  2  3  4  5  6  7  8  9 10
2  3  4  5  6  7  8  9 10 11
3  4  5 12  7  8  9 10 11 12
4  5  6  7  8  9 10 11 12 13
5  6  7  8  9 10 11 12 13 14
6  7  8  9 10 11 12 13 14 15
7  8  9 10 11 12 13 14 15 16
8  9 10 11 12 13 14 15 16 17
9 10 11 12 13 14 15 16 17 18
```
only (x, 3) = (3, 3) has been updated<br>
**Once you update the definition of functions, you cannot give a new predicate for r** <br>
Otherwise, you'll get this error message
```
cannot be given a new predicate, because it has already been used in the update definition of some function.
Aborted (core dumped)
```

#### An Example pf WRONG code
```
r.where(r.y == 3);
gradient(r.x, r.y) = gradient(r.x, r.y) * 2; //updated
r.where(r.x == 3); //can't define predicate again
gradient(r.x, r.y) = 0;
```

### Add new function and rdom s
```
Halide::RDom s{0, 10, 0, 10, "s"};
s.where(s.x == 2);
Halide:: Func another;
another(x, y) = x - y;
gradient(s.x, s.y) = gradient(s.x, s.y) * another(s.x, s.y);
```

output:
```
0   1   4   3   4   5   6   7   8   9
1   2   3   4   5   6   7   8   9  10
2   3   0   5   6   7   8   9  10  11
3   4  -5  12   7   8   9  10  11  12
4   5 -12   7   8   9  10  11  12  13
5   6 -21   8   9  10  11  12  13  14
6   7 -32   9  10  11  12  13  14  15
7   8 -45  10  11  12  13  14  15  16
8   9 -60  11  12  13  14  15  16  17
9  10 -77  12  13  14  15  16  17  18
 ```

### Add smaller rdom (4x4)
```
Halide::RDom t{0, 4, 0, 4, "t"};
gradient(t.x, t.y) = gradient(t.x, t.y)  + 10;
```
output:
```
10  11  14  13   4   5   6   7   8   9
11  12  13  14   5   6   7   8   9  10
12  13  10  15   6   7   8   9  10  11
13  14   5  22   7   8   9  10  11  12
 4   5 -12   7   8   9  10  11  12  13
 5   6 -21   8   9  10  11  12  13  14
 6   7 -32   9  10  11  12  13  14  15
 7   8 -45  10  11  12  13  14  15  16
 8   9 -60  11  12  13  14  15  16  17
 9  10 -77  12  13  14  15  16  17  18
```

### Add smaller rdom which starts non-zero place
```
Halide::RDom u{4, 5, 4, 5, "u"};
gradient(u.x, u.y) = 0;
```
outut
```
10  11  14  13   4   5   6   7   8   9
11  12  13  14   5   6   7   8   9  10
12  13  10  15   6   7   8   9  10  11
13  14   5  22   7   8   9  10  11  12
 4   5 -12   7   0   0   0   0   0  13
 5   6 -21   8   0   0   0   0   0  14
 6   7 -32   9   0   0   0   0   0  15
 7   8 -45  10   0   0   0   0   0  16
 8   9 -60  11   0   0   0   0   0  17
 9  10 -77  12  13  14  15  16  17  18
```
Notice that rdom accessed [4, 4] to (9, 9).<br>
**The second argument of the vector is length and relative value, not absolute**

### Use AOI
This time, aoi is not the specific area, it moves as a mask, so define it (0, width, 0, height) and access it by +v.x and +v.y<br>
**Switch the output function from gradient to dst**
```
Halide::RDom v{0, 2, 0, 2, "v"};
Halide::Func clamped = Halide::BoundaryConditions::repeat_edge(gradient, 0, 10, 0, 10);
Halide::Expr aoi;
aoi = clamped(v.x+x, v.y+y);
Halide::Expr total = sum(aoi);

Halide::Func dst;
dst(x, y) = total;

//Halide::Buffer<int32_t> output = gradient.realize(10, 10);
Halide::Buffer<int32_t> output = dst.realize(10, 10);
```
output:
```
44  50  54  36  20  24  28  32  36  38
48  48  52  40  24  28  32  36  40  42
52  42  52  50  28  32  36  40  44  46
36  12  22  36  15  17  19  21  36  50
20 -22 -18  15   0   0   0   0  27  54
24 -40 -36  17   0   0   0   0  29  58
28 -62 -58  19   0   0   0   0  31  62
32 -88 -84  21   0   0   0   0  33  66
36 -118 -114  36  27  29  31  33  52  70
38 -134 -130  50  54  58  62  66  70  72
```
For each (x, y), it has the sum of 2x2 of gradient(x,y)

### This DOESN'T work
```
Halide::RDom v{0, 2, 0, 2, "v"};
Halide::Func clamped = Halide::BoundaryConditions::repeat_edge(gradient, 0, 10, 0, 10);
Halide::Func aoi;
Halide::Var m, n;
aoi(m,n) = m+n;
aoi(v.x, v.y) = clamped(v.x+x, v.y+y);
Halide::Expr total = sum(aoi(v.x, v.y));

Halide::Func dst;
dst(x, y) = total;
```
output (Error message):
```
Undefined variable "x" in definition of Func "aoi"
Aborted (core dumped)
```

 **Feb 21, 2018**

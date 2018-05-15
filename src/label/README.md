## Implementing Label filter on Halide

Since Halide does not have dynamically terminating loop (while loop), The first pass and the second pass cannot reach the smallest equivalent label.

This test case uses 2 functions for:
* the part for the first pass
* the part for merging the equivalent labels

Between these functions, there is the C based function to get the buffer containing equivalent labels.

Time for each function on 1024x768 takes
* 1st halide part: ~ 0.04 sec
* non-halide part: 3 ~ 4 sec
* 2nd halide part: 1.5 ~ 1.9 sec

Also, the buffer size form non-halide part is 4000x2

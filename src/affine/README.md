# Overview

Using Halide, we have implemented affine transformation which is one of the most fundamental processes in image processing.
Given an input image and parameters of affine transformation, this function makes an output image tranformed by the affine.

Operations are applied in the following order:
1. scale about the origin
2. shear in y direction
3. rotation about the origin
4. translation

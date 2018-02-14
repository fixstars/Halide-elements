# Temporary the largest error value is now 9

## In ImageProcessing.h
The following comment is the copy from ImageProcessing.h
```
Expr srcx = cast<float>(cast<float>(x)+cast<float>(0.5f))
            *cast<float>(in_width)
            /cast<float>(out_width);
Expr srcy = cast<float>(cast<float>(y)+cast<float>(0.5f))
            *cast<float>(in_height)
            /cast<float>(out_height);
```
 * At this point, Simplify changes the float arithmetic order <br>
 They seems expand the equation even though the variables are float <br>
 (if the flaot numbers are both constant, they do not expand. Check the following test result)

 * This might make the error and the largest difference between actual and expected is **9 where 1024x768->500x500**

 * test result
     * (x + y) \* a  ->  (x + y) \* a
     * (x + y) / a -> (x + y) \* (1/a)
     * (x \* a) + (y \* a) -> (x + y) \* a
     * (x + y) \* b / c -> (x \* b / c) + (x \* b / c)
     * (x + a) \* b -> (x \* b) + (a \* b)
     * (x + a) / b -> (x / b) + (a / b)
     * (a + b) / c -> (a + b) \* c


 * Guessing that srcx and srcy are passing:<br>
    ( X + a ) \* b / c <br>
    -> ( X + a ) \* b \* 1/c <br>
    -> ( X + a) \* (b \* 1/c) <br>
    -> (X \* (b \* 1/c)) + (a \* (b \* 1/c))

* Check Simplify.cpp
     * void visit(const Mul \*op)
     * void visit(const Div \*op)

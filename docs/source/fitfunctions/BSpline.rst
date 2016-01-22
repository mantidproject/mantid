.. _func-BSpline:

=======
BSpline
=======

.. index:: BSpline

Description
-----------

This function creates spline using the set of points and interpolates
the input between them.

First and second derivatives from the spline can be calculated by using
the derivative1D function.

BSpline function takes a set of attributes and a set of parameters. The
first attrbiute is 'n' which has integer type and sets the number of
interpolation points. The parameter names have the form 'yi' where 'y'
is letter 'y' and 'i' is the parameter's index starting from 0 and have
the type double. Likewise, the attribute names have the form 'xi'.

A BSpline is a function :math:`f(x)` of order N, defined between an interval :math:`a \leqslant x \leqslant b`.
When using BSplines for interpolation or for fitting, we essentially chain BSplines together so that each
spline passes through the breakpoints in that interval.

There are conditions at each breakpoint that need to be fulfilled for the overall BSpline to be piecewise-smooth.

To demonstrate these conditions we can set up a basic BSpline of order 2 with 3 breakpoints:

Breakpoints : :math:`x_0, x_1, x_2`

Our BSpline will be defined as the following: 

.. math::

   B(x) = 
                               \begin{cases}
                                 f_1(x)& x_0 \leq x \leq x_1 \\
                                 f_2(x)& x_1 \leq x \leq x_2 \\
                               \end{cases}
                             
To make our BSpline piecewise-smooth we must ensure that these conditions are satisfied:

.. math::

    \frac{df_1}{dx}(x_1) = \frac{df_2}{dx}(x_1)\\
    \frac{d^2 f_1}{dx^2}(x_1) = \frac{d^2 f_2}{dx^2}(x_1)\\
    
This point of smoothness is represented by the red circle in the graph below of our BSpline function :math:`B(x)`
      
.. image:: ../images/BSplineQuadraticExample.jpg
    :width: 600px
    :align: center
    :height: 400px
    :alt: quadratic example of BSpline

BSplines and Interpolation
--------------------------

BSplines and Fitting
--------------------

.. attributes::

   Uniform;Boolean;true;If set to true, all breakpoints will be evenly spaced between startX and endX
   Order;Integer;3;The order of the spline you wish to use i.e Order = 2 will use Quadratic Splines
   NBreak;Integer;\-;The number of breakpoints you wish to have (must be greater than 1)
   StartX;Double;0.0;Minimum value of X
   EndX;Double;1.0;Maximum value of X
   BreakPoints;Double list;\-;If Uniform is set to false, you must supply the breakpoints as a comma-separated list

.. properties::

.. categories::

.. sourcelink::

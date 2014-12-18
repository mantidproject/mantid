.. _FitConstraint:

Fit Constraint
==============



How constraints on parameters work
----------------------------------

Consider the scenario where the aim is to fit a lorenzian function to a
1D dataset but a constraint applied on the peak centre parameter. Assume
the 1D dataset consists of :math:`N` data points
:math:`(x_1,y_1^{obs}), (x_2,y_2^{obs}), ... (x_N,y_N^{obs})`, where
:math:`x_i` is the ith x-value and :math:`y_i^{obs}` is the ith observed
value for that x-value. Write the lorentzian function as:

.. math:: y_i^{cal}(h, x0, w) = h \left( \frac{w^2}{(x_i-x0)^2+w^2} \right)

where he lorentzian fitting parameters here are

-  :math:`h` - height of peak (at maximum)
-  :math:`x0` - centre of peak
-  :math:`w` - half-width at half-maximum

:math:`x_i` is the x-value of the ith data point and :math:`y_i^{cal}`
is the lorentzian calculated value at that data point.

We want to apply a constraint on the x0 parameter, i.e. the centre of
the peak. For example, apply the constraint that :math:`x0` should be in
between :math:`x0_{min}` and :math:`x0_{max}`. If this is not satisfied
we then add the following penalty function to :math:`y_i^{cal}` if
:math:`x0 < x0_{min}</`:

.. math:: p_i = C(x0_{min}-x0)*R(x_i)

where :math:`C` is a constant (default 1000) and :math:`R(x_i)` a spiky
function which takes the value 1 for the first and last data point and
for every 10th data point from the 1st data point, but is otherwise
zero. The penalty function when :math:`x0 > x0_{min}</` takes the form:

.. math:: p_i = C(x0-x0_{max})*R(x_i)

.

If more than one constraint is defined, then for each violated
constraint a penalty of the type defined above is added to the
calculated fitting function.



.. categories:: Concepts
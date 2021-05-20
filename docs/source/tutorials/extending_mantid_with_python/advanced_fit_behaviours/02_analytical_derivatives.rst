.. _02_analytical_derivatives:

======================
Analytical Derivatives
======================

By default, currently for ``IFunction1D`` types, a numerical derivative
is calculated. An analytical derivative can be supplied by defining a
``functionDeriv1D`` method, which takes three arguments: ``self``,
``xvals`` and ``jacobian``. The jacobian matrix *(notice how it is not square)*
stores the values of the partial derivatives with respect to each of the
parameter values at each of the x points.

This is most easily understood with an example:

.. code-block:: python

    class Example1DFunction(IFunction1D):

        def init(self):
            self.declareParameter("A0", 0.0)
            self.declareParameter("A1", 0.0)

        def function1D(self, xvals):
            a0 = self.getParameterValue("A0")
            a1 = self.getParameterValue("A1")

            # Use numpy arithmetic to compute new array
            return a0 + a1*xvals

        def functionDeriv1D(self, xvals, jacobian):
            for i, x in enumerate(xvals):
                jacobian.set(i, 0, 1) # parameter at index 0
                jacobian.set(i, 1, x) # parameter at index 1

    FunctionFactory.subscribe(Example1DFunction)

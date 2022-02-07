.. _05_emwp_sol:

====================
Exercise 5 Solutions
====================

The aim of this exercise is to implement a simple linear fitting function
defined as:

.. math::

    \LARGE y = A_0 + A_1x

.. code-block:: python

    from mantid.api import *

    class PyLinearFunction(IFunction1D):

        def init(self):
            # Tell Mantid about the 2 parameters
            self.declareParameter("A0", 1.0)
            self.declareParameter("A1", 1.0)

        def function1D(self, xvals):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.
            a0 = self.getParameterValue("A0")
            a1 = self.getParameterValue("A1")

            y = a0 + a1*xvals
            return y

    # Register with Mantid
    FunctionFactory.subscribe(PyLinearFunction)

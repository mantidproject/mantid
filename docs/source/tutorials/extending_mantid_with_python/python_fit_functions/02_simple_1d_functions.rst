.. _02_simple_1d_functions:

===================
Simple 1D Functions
===================

The 1D function type, ``IFunction1D`` is the simplest function type. It takes
a 1D set of x values and requires the function values to be returned at those
points as a numpy array. For the Mantid Fit framework to recognise the
function the method that computes the values must be called ``function1D`` and
takes two arguments: ``self`` and ``xvals``.

As an example take a simple function, :math:`\Large y = C\sqrt{x}`. The class would
look something like:

.. code-block:: python

    from mantid.api import *
    import numpy

    class Example1DFunction(IFunction1D):

        def init(self):
            self.declareParameter("C", 0.0)

        def function1D(self, xvals):
            # Access current values during the fit
            c = self.getParameterValue("C")

            return c*numpy.sqrt(xvals)

    FunctionFactory.subscribe(Example1DFunction)

The current value of a named parameter is accessed using the
``getParameterValue`` method and simply returns a float value.
``function1D`` should return a numpy array of the same length as the input
xvals array. This is now a working example and when loaded into Mantid it will
show as an extra function named *Example1DFunction* in the function menus.

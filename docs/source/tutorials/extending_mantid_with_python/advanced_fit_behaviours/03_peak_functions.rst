.. _03_peak_functions:

==============
Peak Functions
==============

The Peak function type, ``IPeakFunction``, is a specialized kind of 1D
function. It is used when, for the given function, approximate values of
*height*, *fwhm* & *peak centre* can be determined from the function
parameters. Their main use is to improve the choosing of starting values
for these types of function from the GUI.

The function calculation also only occurs around a given *peak radius*
(defined in Settings->Fitting->CurveFitting menu). Any function
values outside this radius are automatically zeroed. The best description
here is to walk through defining an example. Here will implement a ``Gaussian``
(called PyGaussian so as not interfere with Mantid's Gaussian).

Parameters & Attributes
=======================

The parameters & attributes are defined in exactly the same manner as for the
``IFunction1D`` case:

.. code-block:: python

    from mantid.api import *
    import math
    import numpy as np

    class PyGaussian(IPeakFunction):

        def init(self):
            self.declareParameter("Height")
            self.declareParameter("PeakCentre")
            self.declareParameter("Sigma")
            # We don't use any attributes in this example

    FunctionFactory.subscribe(PyGaussian) # Registration is identical

Evaluating the Function & Derivative
====================================

Function evaluation proceeds in a similar manner to ``IFunction1D`` with the
exception that the method is now called ``functionLocal``. Python
``PeakFunctions`` can also provide an analytical derivative by defining a
``functionDerivLocal`` method. This is optional and default is to be
calculated numerical derivatives:

.. code-block:: python

    def functionLocal(self, xvals):
        height = self.getParameterValue("Height")
        peak_centre = self.getParameterValue("PeakCentre")
        sigma = self.getParameterValue("Sigma")
        weight = math.pow(1./sigma,2)

        offset_sq=np.square(xvals-peak_centre)
        out=height*np.exp(-0.5*offset_sq*weight)
        return out

    # the following method is optional
    def functionDerivLocal(self, xvals, jacobian):
        height = self.getParameterValue("Height")
        peak_centre = self.getParameterValue("PeakCentre")
        sigma = self.getParameterValue("Sigma")
        weight = math.pow(1./sigma, 2)

        # X index
        i = 0
        for x in xvals:
            diff = x-peak_centre
            exp_term = math.exp(-0.5*diff*diff*weight)
            jacobian.set(i, 0, exp_term)
            jacobian.set(i, 1, diff*height*exp_term*weight)
            # derivative with respect to weight not sigma
            jacobian.set(i, 2, -0.5*diff*diff*height*exp_term)
            i += 1

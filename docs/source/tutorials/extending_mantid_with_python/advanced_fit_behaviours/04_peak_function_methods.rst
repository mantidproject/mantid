.. _04_peak_function_methods:

=====================
Peak Function Methods
=====================

``IPeakFunction`` defines 6 special methods for dealing with the peak shape.
The first 3 all return an *estimate* for the values of the ``centre``,
``height`` & ``fwhm`` for the current parameter values.

The second 3: ``setCentre``, ``setHeight``, ``setFwhm``, all pass along
the current value, picked from the GUI, for the ``centre``, ``height`` &
``fwhm`` and update the starting values of the function accordingly.

All of these functions are meant to be called automatically by Mantid and
you will not need to call them yourself. It should be stressed that these
values picked from the GUI just provide better starting values, during the
fitting procedure these values are no longer used and the real function
evaluation is performed. An example for the Gaussian follows:

.. code-block:: python

    def centre(self):
        # Easy here as there is a direct parameter
        return self.getParameterValue("PeakCentre")

    def height(self):
        # Easy here as there is a direct parameter
        return self. getParameterValue("Height")

    def fwhm(self):
        # Computes the FWHM from sigma
        return 2.0*math.sqrt(2.0*math.log(2.0))*self.getParameterValue("Sigma")

    def setCentre(self, new_centre):
        # User picked point new_centre
        self.setParameter("PeakCentre", new_centre)

    def setHeight(self, new_height):
        # User set new height for peak
        self.setParameter("Height", new_height)

    def setFwhm(self, new_fwhm):
        # User had a guess at the width using the range picker bar
        sigma = new_fwhm/(2.0*math.sqrt(2.0*math.log(2.0)))
        self.setParameter("Sigma", sigma)

Complete Example
================

For completeness the complete example PyGaussian is presented here:

.. code-block:: python

    from mantid.api import *
    import math
    import numpy as np

    class PyGaussian(IPeakFunction):

        def category(self):
            return "Examples"

        def init(self):
            self.declareParameter("Height")
            self.declareParameter("PeakCentre")
            self.declareParameter("Sigma")

        def functionLocal(self, xvals):
            height = self.getParameterValue("Height")
            peak_centre = self.getParameterValue("PeakCentre")
            sigma = self.getParameterValue("Sigma")
            weight = math.pow(1./sigma, 2)

            offset_sq=np.square(xvals-peak_centre)
            out=height*np.exp(-0.5*offset_sq*weight)
            return out

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

        def activeParameter(self, index):
            param_value = self.getParameterValue(index)
            if index == 2: #Sigma. Actually fit to 1/(sigma^2) for stability
                return 1./math.pow(param_value, 2)
            else:
                return param_value

        def setActiveParameter(self, index, value):
            param_value = value
            explicit = False
            if index == 2:
                param_value = math.sqrt(math.fabs(1.0/value))
            else:
                param_value = value
                # Final explicit argument is required to be false here
                self.setParameter(index, param_value, False)

        def centre(self):
            return self.getParameterValue("PeakCentre")

        def height(self):
            return self.getParameterValue("Height")

        def fwhm(self):
            return 2.0*math.sqrt(2.0*math.log(2.0))*self.getParameterValue("Sigma")

        def setCentre(self, new_centre):
            self.setParameter("PeakCentre", new_centre)

        def setHeight(self, new_height):
            self.setParameter("Height", new_height)

        def setFwhm(self, new_fwhm):
            sigma = new_fwhm/(2.0*math.sqrt(2.0*math.log(2.0)))
            self.setParameter("Sigma", sigma)

    # Required to have Mantid recognise the new function
    FunctionFactory.subscribe(PyGaussian)

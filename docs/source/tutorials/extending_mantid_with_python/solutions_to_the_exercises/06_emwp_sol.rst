.. _06_emwp_sol:

====================
Exercise 6 Solutions
====================

Simple 1D
=========

The aim of this exercise is to implement a Lorentz function, that has no
derivative.

.. code-block:: python

    from mantid.api import *
    import math
    import numpy as np

    INVERSE_PI = 1.0/math.pi

    class Lorentz(IFunction1D):

        def init(self):
            # Tell Mantid about the 3 parameters that are involved in the function
            self.declareParameter("Amplitude", 0.0)
            self.declareParameter("PeakCentre", 0.0)
            self.declareParameter("Gamma", 0.0)

        def function1D(self, xvals):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.

            # Get the current values of the 3 parameters
            amp = self.getParameterValue("Amplitude") # Access current value during the fit
            half_gamma = 0.5*self.getParameterValue("Gamma")
            c = self.getParameterValue("PeakCentre")

            denom = (xvals - c)**2 + half_gamma*half_gamma
            return amp*INVERSE_PI*half_gamma/denom

    # Register with Mantid
    FunctionFactory.subscribe(Lorentz)

Analytical Derivative
=====================

The aim of this exercise is to implement a Lorentz function, that contains a
derivative.

.. code-block:: python

    from mantid.api import *
    import math
    import numpy as np

    INVERSE_PI = 1.0/math.pi

    class LorentzWithDeriv(IFunction1D):

        def init(self):
            # Tell Mantid about the 3 parameters that are involved in the function
            self.declareParameter("Amplitude", 1.0)
            self.declareParameter("PeakCentre", 0.0)
            self.declareParameter("Gamma", 0.1)

        def function1D(self, xvals):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.

            # Get the current values of the 3 parameters
            amp = self.getParameterValue("Amplitude") # Access current value during the fit
            half_gamma = 0.5*self.getParameterValue("Gamma")
            peak_centre = self.getParameterValue("PeakCentre")

            denom = (xvals - peak_centre)**2 + half_gamma*half_gamma
            return amp*INVERSE_PI*half_gamma/denom

        def functionDeriv1D(self, xvals, out):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.
            # out is a Jacobian matrix object. Mantid expects the partial derivatives
            # w.r.t the parameters and x values to be stored here
            # Get the current values of the 3 parameters

            amplitude = self.getParameterValue("Amplitude")
            peakCentre = self.getParameterValue("PeakCentre")
            gamma = self.getParameterValue("Gamma")
            halfGamma = 0.5*gamma

            for i, xval in enumerate(xvals):
                diff = xval-peakCentre
                invDen1 = 1.0/(gamma*gamma + 4.0*diff*diff)
                dfda = 2.0*INVERSE_PI*gamma*invDen1
                out.set(i, 0, dfda)

                invDen2 =  1/(diff*diff + halfGamma*halfGamma)
                dfdxo = amplitude*INVERSE_PI*gamma*diff*invDen2*invDen2
                out.set(i, 1, dfdxo);

                dfdg = -2.0*amplitude*INVERSE_PI*(gamma*gamma - 4.0*diff*diff)*invDen1*invDen1
                out.set(i, 2, dfdg)


    # Register with Mantid
    FunctionFactory.subscribe(LorentzWithDeriv)

Peak Function
=============

The aim of this exercise is to implement a peak fit function function to fit.

.. code-block:: python

    from mantid.api import *
    import numpy as np
    import math

    INVERSE_PI = 1.0/math.pi

    class LorentzPeak(IPeakFunction):

        def init(self):
            # Tell Mantid about the 3 parameters that are involved in the function
            self.declareParameter("Amplitude", 0.0)
            self.declareParameter("PeakCentre", 0.0)
            self.declareParameter("Gamma", 0.0)

        def functionLocal(self, xvals):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.
            half_gamma = 0.5*self.getParameterValue("Gamma")
            denom = (xvals - self.getParameterValue("PeakCentre"))**2 + half_gamma*half_gamma
            return self.getParameterValue("Amplitude")*INVERSE_PI*half_gamma/denom

        def functionDerivLocal(self, xvals, out):
            # xvals is a 1D numpy array that contains the X values for the defined fitting range.
            # out is a Jacobian matrix object. Mantid expects the partial derivatives
            # w.r.t the parameters and x values to be stored here

            # Get the current parameter values
            amplitude = self.getParameterValue("Amplitude")
            peakCentre = self.getParameterValue("PeakCentre")
            gamma = self.getParameterValue("Gamma")
            halfGamma = 0.5*gamma

            for i, xval in enumerate(xvals):
                diff = xval-peakCentre
                invDen1 = 1.0/(gamma*gamma + 4.0*diff*diff)
                dfda = 2.0*INVERSE_PI*gamma*invDen1
                out.set(i, 0, dfda)

                invDen2 =  1/(diff*diff + halfGamma*halfGamma)
                dfdxo = amplitude*INVERSE_PI*gamma*diff*invDen2*invDen2
                out.set(i, 1, dfdxo);

                dfdg = -2.0*amplitude*INVERSE_PI*(gamma*gamma - 4.0*diff*diff)*invDen1*invDen1
                out.set(i, 2, dfdg)

        def centre(self):
            # Return a guess at the centre
            return self.getParameterValue("PeakCentre")

        def height(self):
            # Return a guess at the height
            return self.getParameterValue("Amplitude")

        def fwhm(self):
            # Return a guess at the FWHM
            return 2*self.getParameterValue("Gamma")

        def setCentre(self, new_centre):
            # Update centre guess when a new value is chosen from GUI
            self.setParameter("PeakCentre", new_centre)

        def setHeight(self, new_height):
            # Update Amplitude guess when a new height is chosen from GUI
            self.setParameter("Amplitude", new_height)

        def setFwhm(self, new_fwhm):
            # Update Gamma guess when a new width is chosen from GUI
            self.setParameter("Gamma", new_fwhm/2.0)

    # Register function with Mantid
    FunctionFactory.subscribe(LorentzPeak)

.. _01_fit_stability:

=============
Fit Stability
=============

In most cases, a fit will be performed over the parameters exactly as they
are declared. In some cases, however, the fit can be unstable in one or more
of the parameters. As an example consider a Gaussian:

.. math::
    Ae^{-\frac{(x-c)^2}{2\sigma^2}}

The simple function fits over 3 parameters:

#. ``A`` - Amplitude
#. ``c`` - Peak centre
#. ``\sigma`` - Measure of the width

However, the :math:`\frac{1}{\sigma^2}` dependence causes the fit to become
unstable if the \sigma parameter is varied by the minimization routine. A more
stable fit can be achieved by fitting in :math:`\frac{1}{\sigma}`.

We could just naively change the **\sigma** parameter to :math:`\frac{1}{\sigma}`
and fit over this. The function is now much less user-friendly though as we
still want to think terms of the values of **A**, **c**, **\sigma** and not **A**,
**c**, :math:`\frac{1}{\sigma}`.

Our optimisation framework actually works with the concept of active
parameters, where we allow the fitting to proceed over different parameter
than that declared by ``init``.

In order to translate between the two representations, the function class
must provide two further methods:

#. ``activeParameter(self, index)`` - Should return the value of the parameter
   at the given index that should take part in the fit.
#. ``setActiveParameter(self, index, value)`` - Called by the framework when a
   parameter value is updated. The function should then translate the value to
   its original form.

The active parameter methods are optional and are only required in the
situations where the fitting parameter is not equal to the declared one. This
is best illustrated by an example of a Gaussian (cut-down to show the relevant
parts):

.. code-block:: python

    from mantid.api import *
    import math
    import numpy as np

    class PyGaussian(IFunction1D):

        def init(self):
            self.declareParameter("Height")
            self.declareParameter("PeakCentre")
            # Parameter is declared as you want it to be seen by the user
            self.declareParameter("Sigma")

        def function1D(self, xvals):
            # Can also be retrieve by index self.getParameterValue(0)
            height = self.getParameterValue("Height")
            peak_centre = self.getParameterValue("PeakCentre")
            sigma = self.getParameterValue("Sigma")
            weight = math.pow(1./sigma, 2)

            offset_sq = np.square(xvals-peak_centre)
            out = height*np.exp(-0.5*offset_sq*weight)
            return out

        def activeParameter(self, index):
            # Return the value of the parameter at the given index
            # (ordered by the order in init)
            param_value = self.getParameterValue(index)

            if index == 2: #Sigma. Actually fit to 1/(sigma^2) for stability
                # param_value contains value of sigma
                return 1./math.pow(param_value, 2)
            else:
                # Deal with other cases. In this case we just want value as is
                return param_value

        def setActiveParameter(self, index, value):
            # The framework minimizer wants to update the value of the parameter
            param_value = value
            explicit = False
            if index == 2: #sigma parameter index
                # value passed in is actually 1/sigma^2 so we need to translate
                # back to sigma
                param_value = math.sqrt(math.fabs(1.0/value))
            else:
                param_value = value # others are 1:1

            # Finally, actually update the values stored in function
            # so that the next call to function1D sees them
            self.setParameter(index, param_value, False)

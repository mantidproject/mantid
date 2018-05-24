.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs `Le Bail Fit <Le Bail Fit>`__ to powder
diffraction data, and also supports pattern calculation. This algorithm
will refine a specified set of the powder instrumental profile
parameters with a previous refined background model.

Peak profile function for fit
#############################

Back to back exponential convoluted with pseudo-voigt
#####################################################

Here is the list of the peak profile function supported by this
algorithm.

-  Thermal neutron back-to-back exponential convoluted with pseudo-voigt

   -  geometry-related parameters: Dtt1, Dtt2, Zero
   -  back-to-back exponential parameters: Alph0, Alph1, Beta0, Beta1
   -  pseudo-voigt parameters: Sig0, Sig1, Sig2, Gam0, Gam1, Gam2

Thermal neutron back to back exponential convoluted with pseudo-voigt
#####################################################################

Here is the list of the peak profile function supported by this
algorithm.

-  Thermal neutron back-to-back exponential convoluted with pseudo-voigt

   -  geometry-related parameters: Dtt1, Zero, Dtt1t, Dtt2t, Width,
      Tcross
   -  back-to-back exponential parameters: Alph0, Alph1, Beta0, Beta1,
      Alph0t, Alph1t, Beta0t, Beta1t
   -  pseudo-voigt parameters: Sig0, Sig1, Sig2, Gam0, Gam1, Gam2

Optimization
############

*LeBailFit* supports a tailored simulated annealing optimizer (using
Monte Carlo random walk algorithm). In future, regular minimizes in GSL
library might be supported.

Supported functionalities
#########################

| ``* LeBailFit: fit profile parameters by Le bail algorithm; ``
| ``* Calculation: pattern calculation by Le bail algorithm; ``
| ``* MonteCarlo: fit profile parameters by Le bail algorithm with Monte Carlo random wal; ``
| ``* RefineBackground: refine background parameters``

Further Information
###################

See `Le Bail Fit <Le Bail Fit>`__.

.. categories::

.. sourcelink::

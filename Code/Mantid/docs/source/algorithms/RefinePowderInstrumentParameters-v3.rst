.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm refines the instrumental geometry parameters for powder
diffractomers. The parameters that can be refined are Dtt1, Zero, Dtt1t,
Dtt2t, Zerot, Width and Tcross.

It serves as the second step to fit/refine instrumental parameters that
will be introduced in Le Bail Fit. It uses the outcome from algorithm
FitPowderDiffPeaks().

Mathematics
-----------

The function to fit is

TOF\_h = n(Zero + Dtt1\*d) + (1-n)(Zerot + Dtt1t\*d + Dtt2t/d) n = 1/2
erfc(W\*(1-Tcross/d))

The coefficients in this function are strongly correlated to each other.

Refinement Algorithm
--------------------

Two refinement algorithms, DirectFit and MonteCarlo, are provided.

DirectFit
#########

This is a simple one step fitting. If there is one parameter to fit,
Levenberg Marquart minimizer is chosen. As its coefficients are strongly
correlated to each other, Simplex minimizer is used if there are more
than 1 parameter to fit.

MonteCarlo
##########

This adopts the concept of Monte Carlo random walk in the parameter
space. In each MC step, one parameter will be chosen, and a new value is
proposed for it. A constraint fitting by Simplex minimizer is used to
fit the coefficients in new configuration.

Simulated annealing will be tried as soon as it is implemented in
Mantid.

Constraint
##########

How to use algorithm with other algorithms
------------------------------------------

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in the wiki page of
:ref:`algm-LeBailFit`.

.. categories::

.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to import Fullprof .irf file (peak parameters) and
.hkl file (reflections) and record the information to TableWorkspaces,
which serve as the inputs for algorithm LeBailFit.

Format of Instrument parameter TableWorkspace
#############################################

Instrument parameter TableWorkspace contains all the peak profile
parameters imported from Fullprof .irf file.

Presently these are the peak profiles supported

``* Thermal neutron back to back exponential convoluted with pseudo-voigt (profile No. 10 in Fullprof)``

Each row in TableWorkspace corresponds to one profile parameter.

Columns include Name, Value, FitOrTie, Min, Max and StepSize.

Format of reflection TableWorkspace
###################################

Each row of this workspace corresponds to one diffraction peak. The
information contains the peak's Miller index and (local) peak profile
parameters of this peak. For instance of a back-to-back exponential
convoluted with Gaussian peak, the peak profile parameters include
Alpha, Beta, Sigma, centre and height.

How to use algorithm with other algorithms
------------------------------------------

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in the wiki page of
:ref:`algm-LeBailFit`.

.. categories::

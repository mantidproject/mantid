==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

Improved
########

Vesuvio
-------

New
###

Improved
########
- A pre-loaded runs workspace can now be passed to the fit_tof VESUVIO routine, which will skip the loading subroutine given this input

Bugfixes
########

Data Analysis Interfaces
------------------------

New
###
- ConvFit, IqtFit, MSDFit and JumpFit now have a second mini-plot for the difference. The sample and calculated fit are found in the top mini-plot, the difference is found in the bottom mini-plot.

Improved
########
- The Plot Guess Feature in the ConvFit Interface is now enabled for the diffusion functions.
- The Plot Guess Feature in the MSDFit Interface is now implemented for the three models introduced in release v3.11 (MsdGauss, MsdPeters and MsdYi).

Bugfixes
########

Data Reduction Interfaces
-------------------------

New
###

Improved
########

Bugfixes
########

Corrections Interfaces
----------------------

New
###

Improved
########

Bugfixes
########

Abins
-----

Improved
########
- Performance of Abins routines significantly improved (a factor of 10-20 times for data size of 4000).

:ref:`Release 3.12.0 <v3.12.0>`

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
- :ref:`LoadVesuvio <algm-LoadVesuvio>` can now accept a discontinuous range of sample runs - uses the same syntax as the Load algorithm.

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

:ref:`Release 3.12.0 <v3.12.0>`

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

- BASISReduction now permits the user to exclude a contiguous time segment from the reduction process

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
- The X-Limits for all of the Indirect Data Analysis interfaces are now correctly updated when data is loaded.
- In the IqtFit interface, the 'AO' parameter now defaults to 0.
- The mini preview plot now updates correctly in the Indirect Data Analysis interfaces, when the fit function is changed; when changed to a function that wasn't used in the most recent fit, will plot only the sample, else will plot the sample, fit and difference.
- Plotting individual parameters of the fit in the interface ('Plot Options'), will no longer produce an error in the ConvFit interface, when plotting 'FWHM'.

Data Reduction Interfaces
-------------------------

New
###

Improved
########

Bugfixes
########

Indirect Diffraction
--------------------

Improved
########
- Manual D-Range option removed from the indirect diffraction reduction interface; D-Ranges are now automatically calculated for sample, container and vanadium runs.

Corrections Interfaces
----------------------

New
###

Improved
########

Bugfixes
########

:ref:`Release 3.12.0 <v3.12.0>`

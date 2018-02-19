==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

- :ref:`algm-ExtractQENSMembers` can be used to extract the fit members from any QENS fit.

Improved
########

- :ref:`algm-ApplyPaalmanPingsCorrection` now accepts a corrections group containing only an :math:`A_{s,s}` and an :math:`A_{c,c}` workspace (produced by :ref:`algm-CalculateMonteCarloAbsorption`).
- :ref:`BASISReduction  <algm-BASISReduction>` now permits the user to exclude a contiguous time segment from the reduction process.
- :ref:`BASISReduction <algm-BASISReduction>` option *noMonitorNorm* changed to *MonitorNorm*.
- :ref:`BASISReduction <algm-BASISReduction>` now contains log entry *asString* storing the options passed to to the algorithm.

Vesuvio
-------

New
###

Improved
########
- A pre-loaded runs workspace can now be passed to the fit_tof VESUVIO routine, which will skip the loading subroutine given this input
- Loading the sample log files into a workspace can be disabled when calling the :ref:`LoadVesuvio <algm-LoadVesuvio>` algorithm by supplying `LoadLogFiles=False` to the algorithm call
- Loading the sample log files into a workspace can be disabled within a Vesuvio Driver Script, by setting the 'load_log_files' flag to False

Bugfixes
########
- Previously, running a script which only applied a single correction (e.g. container subtraction), would produce an error 'f0.Scaling', this has now been fixed.

Data Analysis Interfaces
------------------------

New
###
- ConvFit, IqtFit, MSDFit and JumpFit now have a second mini-plot for the difference. The sample and calculated fit are found in the top mini-plot, the difference is found in the bottom mini-plot.

Improved
########
- The Plot Guess Feature in the ConvFit Interface is now enabled for the diffusion functions.
- The Plot Guess Feature in the MSDFit Interface is now implemented for the three models introduced in release v3.11 (MsdGauss, MsdPeters and MsdYi).
- The ExtractMembers option in the ConvFit Interface will now function correctly for diffusion functions.

Bugfixes
########
- The X-Limits for all of the Indirect Data Analysis interfaces are now correctly updated when data is loaded.
- In the IqtFit interface, the 'AO' parameter now defaults to 0.
- The mini preview plot now updates correctly in the Indirect Data Analysis interfaces, when the fit function is changed; when changed to a function that wasn't used in the most recent fit, will plot only the sample, else will plot the sample, fit and difference.
- Plotting individual parameters of the fit in the interface ('Plot Options'), will no longer produce an error in the ConvFit interface, when plotting 'FWHM'.
- In the Calculate Paalman Pings tab of the Indirect Correction interface the container back thickness is now set correctly.

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
- The Apply Paalman Pings interface has been renamed to Apply Absorption Correction.
- The Apply Absorption Correction interface no longer requires workspaces to be in units of wavelength (this is done within :ref:`algm-ApplyPaalmanPingsCorrection`).

Bugfixes
########

Abins
-----

Improved
########
- Performance of Abins routines significantly improved (a factor of 10-20 times for data size of 4000).

:ref:`Release 3.12.0 <v3.12.0>`

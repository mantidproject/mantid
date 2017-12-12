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

- BASISReduction now permits the user to exclude a contiguous time segment from the reduction process.
- BASISReduction option noMonitorNorm changed to MonitorNorm.

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

=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

Improvements
############

- An issue with IndirectDataReduction causing it to slow down processing when open has been resolved.
- Scientific constraints have been added to all fitting tabs in IndirectDataAnalysis.
- The Abins python library in *scripts* has been substantially
  re-organised, including a re-name from ``AbinsModules`` to
  ``abins``. These changes should make the library more approachable and maintainable.
  They should not impact functionality of the Abins Algorithm, but will break any user python scripts
  that import ``AbinsModules``.
- Vibrational calculations performed within the VASP quantum chemistry
  package are now supported by Abins. When the IBRION parameter of
  VASP is set to an integer from 5-8, Gamma-point vibrational
  frequencies are written to the vasprun.xml output file which may be
  loaded into Abins. Results for crystalline benzene were validated
  against data from CASTEP and found to give fairly similar
  frequencies and intensities.
- :ref:`CalculateMonteCarloAbsorption <algm-CalculateMonteCarloAbsorption>` will now work also for ILL fixed window scan reduced data, in which case the correction will be calculated for elastic wavelength.
- The centre parameter has been added to delta function in the ConvFit tab of Indirect Data Analysis.
- :ref- :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` will produce energy transfer axis which takes into account that doppler channels are linear in velocity (not in time, neither energy as was assumed before). This will affect doppler mode QENS only.
- Added docking and undocking to the plot window and function browser window for the fit tabs in Indirect Data Analysis on workbench.
- Update the Indirect, Corrections user interface to use the :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>` algorithm and make it work for elastic datasets
- OutputCompositeMembers and ConvolveOutputs have been added as options in the ConvFit tab of Indirect Data Analysis.
- Improved the responsiveness of the function browsers within the Indirect Data Analysis interface.
- Raw data will now be plotted from the input workspace, rather than the fit workspace, allowing the full range of data to be seen irrespective of the fitting bounds.

Bug Fixes
#########

- :ref:`CalculateMonteCarloAbsorption <algm-CalculateMonteCarloAbsorption>` will now work correctly for annular sample in a container.
- FQ and Msd tabs now label output workspaces with the fitting function.
- Fixed a crash when switching between linear and flat backgrounds in the ConvFit tab.
- Disabled the plot guess checkbox within the ConvFit tab if a resolution file has not been loaded. This prevents a hard crash which can occur.

:ref:`Release 5.1.0 <v5.1.0>`

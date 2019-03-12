==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New Algorithms
##############

- :ref:`BASISCrystalDiffraction <algm-BASISCrystalDiffraction>` replaces :ref:`BASISDiffraction <algm-BASISDiffraction>`, now deprecated.
- :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>` obtains scattered elastic intensity versus momentum transfer and versus scattering angle.
- TOF correction for neutrons incorrectly interpreted as slow neutrons in :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>`.
- Deprecated algorithm BASISReduction311 has been removed.
- :ref:`LoadEMU <algm-LoadEMU>` loader for an ANSTO EMU backscattering event file.
- :ref:`IndirectReplaceFitResult <algm-IndirectReplaceFitResult>` will replace the results of a fit found in one workspace with the
  single fit data provided within a second workspace.

:ref:`Release 4.0.0 <v4.0.0>`

All Indirect Interfaces
-----------------------

New Features
############
- Added a *Settings* GUI which can be opened using the button next to the help *?* button on
  the bottom left of the interfaces.

.. figure:: ../../images/Settings_Data_Analysis.png
  :class: screenshot
  :align: center
  :figwidth: 70%
  :alt: The settings GUI for the Indirect Data Analysis interface.

Improvements
############

- Moved the Run button to be above the output options.
- Disabled the Run, Plot and Save buttons while processing or plotting is taking place.


Data Analysis Interface
-----------------------

New Features
############

.. figure:: ../../images/Indirect_Replace_Fit_Result.png
  :class: screenshot
  :align: right
  :figwidth: 50%
  :alt: The dialog used to replace a fit result in the Indirect Data Analysis interface.

- Added an option to edit the fit results on the IqtFit and ConvFit tabs. This option allows
  the selection of two *_Result* workspaces (one containing the results of a sequential fit
  for multiple spectra (1), and the other containing the results of a fit for a single
  spectrum(2)). The algorithm :ref:`IndirectReplaceFitResult <algm-IndirectReplaceFitResult>`
  will replace the relevant data found in workspace (1) with the data found in workspace (2).
- Added an option to skip the calculation of Monte Carlo Errors on the I(Q,t) tab.
- Added an option to the settings GUI allowing you to turn off the restriction of input data
  based on their name.
- Added an option to the settings GUI allowing you to turn on error bars for the output plots.

Improvements
############

- Disabled the *Fit Single Spectrum* button on MSDFit, I(Q,t)Fit, ConvFit and F(Q)Fit while fitting.
- Disabled the Plot buttons in MSDFit, I(Q,t)Fit, ConvFit and F(Q)Fit after a Run when the result
  workspace only has one data point to plot.
- Added a progress bar to track the progress of the calculation of Monte Carlo Errors on Iqt.
- Added an option to choose which spectrum to plot for *Plot Spectrum* in the I(Q,t) tab.
- Added an option to select a range of spectra for a *Tiled Plot* in the I(Q,t) tab. There is a
  maximum of 18 plots allowed in the tiled plot.

.. figure:: ../../images/Iqt_Output_Plot_Options.png
  :class: screenshot
  :align: center
  :figwidth: 100%
  :alt: The options used for plotting output on the Iqt tab.

- Disabled the Fit and Fit Sequential options in the Fit combobox above the FitPropertyBrowser while
  fitting is taking place.
- Added an option to choose which workspace index to *Plot Spectrum* for and from which output workspace
  in the Elwin tab.
- Added the ability to load Dave ASCII files which end with *_sqw.dave* into the ConvFit tab.
- Changed the AddWorkspace windows (opened from the Multiple Input tab) so that they now stay open after
  adding a workspace to the data table. This is found on the MSDFit, I(Q,t)Fit, ConvFit and F(Q)Fit
  interfaces.
- Added the ability to load a Nexus file without it's history on the Elwin tab by unchecking
  the Load History checkbox.
- Added the ability to undock the mini-plots on the MSDFit, IqtFit, ConvFit and F(Q)Fit interfaces.

.. figure:: ../../images/Undock_Mini_Plots_Data_Analysis.png
  :class: screenshot
  :align: center
  :figwidth: 80%
  :alt: The undocked miniplots on Indirect Data Analysis.

Bugfixes
########

- Fixed an issue when the :ref:`InelasticDiffSphere <func-InelasticDiffSphere>`, 
  :ref:`InelasticDiffRotDiscreteCircle <func-InelasticDiffRotDiscreteCircle>`,
  :ref:`ElasticDiffSphere <func-ElasticDiffSphere>` and 
  :ref:`ElasticDiffRotDiscreteCircle <func-ElasticDiffRotDiscreteCircle>` functions are selected in
  the ConvFit tab. The Q values were not being retrieved from the input workspace, which caused a
  crash when plotting a guess.
- Fixed an issue where the WorkspaceIndex and Q value in the FitPropertyBrowser were not updating
  when the *Plot Spectrum* number is changed. This improvement can be seen in ConvFit when functions
  which depend on Q value are selected.
- Fixed an unexpected crash when workspace(s) are loaded into F(Q) Fit, which do not have EISF or Width
  values. An error message is displayed if neither are present.
- Fixed an issue where the parameter values for a selected spectrum were not being updated when using
  the Fit String option in ConvFit.
- Fixed an unexpected crash caused by clicking *Plot Current Preview* when no data is loaded. A
  meaningful error message is now displayed.
- Fixed an issue where the Probability Density Functions (PDF) workspaces for the FABADA minimiser were
  overwriting each other in ConvFit.
- Fixed an unexpected error which was caused by loading a resolution file before a reduced file in
  ConvFit.
- Fixed a bug where fixed parameters didn't remain fixed when using the FABADA minimizer in ConvFit.
- Updated the expression for the Fit type :ref:`MSDYi <func-MSDYi>` in MSDFit.
- Fixed the x-axis labels in the output plots for MSDFit.
- Fixed an unexpected error caused by clicking *Plot Guess* from the *Display* combo box in ConvFit
  without first loading a reduced file.
- Fixed a bug where the output workspaces from a fit did not have the fit function used in their name.
- Fixed an unexpected error when selecting multiple data using the All Spectra checkbox without first
  selecting a sample file. Meaningful error messages are also displayed when a sample or resolution
  file are not selected.
- Fixed an issue where the errors were not being propagated through to the workspace with extension
  *_elt* (produced on the Elwin tab).
- Updated the :ref:`HallRoss <func-Hall-Ross>` fit function to have :math:`\hbar` in its formula.
  The :ref:`TeixeiraWater <func-TeixeiraWater>` and :ref:`ChudleyElliot <func-ChudleyElliot>` fit
  functions now have a functionDeriv1D method.
- Fixed a bug causing the output *_Results* workspace from a single fit to have an incorrect name.
- Fixed a bug causing the preview plot in Elwin not to update when changing the selected workspace.


Data Corrections Interface
--------------------------

New Features
############
- Added an option to the settings GUI allowing you to turn off the restriction of input data
  based on their name.
- Added an option to the settings GUI allowing you to turn on error bars for the output plots.

Improvements
############

- Added an 'Interpolation' combobox to Calculate Monte Carlo Absorption. This allows the method of
  interpolation to be selected. Allowed values: ['Linear', 'CSpline'].
- Added an 'MaxScatterPtAttempts' spinbox to Calculate Monte Carlo Absorption. This sets the maximum
  number of tries to be made to generate a scattering point.
- Updated the Calculate Monte Carlo Absorption tab so that all of the options in the Monte Carlo
  section are now read from an instrument parameter files (IPF) once a file has been loaded.
- Added an option to choose which spectrum to *Plot Spectrum* for in the ContainerSubtraction tab
  and ApplyAbsorptionCorrections tab.


Data Reduction Interface
------------------------

New Features
############
- Added an option to the settings GUI allowing you to turn off the restriction of input data
  based on their name.
- Added an option to the settings GUI allowing you to turn on error bars for the output plots.

Improvements
############

- Added 'Default' detector grouping option back into ISISEnergyTransfer for TOSCA, to allow a
  default grouping using the grouping specified in the Instrument Parameter File.
- ISISEnergyTransfer now allows overlapping detector grouping.
- Added an option to choose which spectrum to *Plot Output* for in the S(Q, w) tab.
- Added an automatic contour plot of *rqw* in the S(Q, w) tab. This is displayed when a sample is
  loaded.

.. figure:: ../../images/Automatic_Contour_Plot_Sqw.png
  :class: screenshot
  :align: center
  :figwidth: 80%
  :alt: The automatic contour plot which is plotted on the S(Q, w) tab.

Bugfixes
########
- Fixed a bug where the output reduced files had large file sizes depending on the size of the batch
  being reduced from the :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`
  algorithm on the ISISEnergyTransfer interface. The
  :ref:`ISISIndirectEnergyTransferWrapper <algm-ISISIndirectEnergyTransferWrapper>` algorithm
  should be used instead in order to avoid these large file sizes.
- Fixed a bug caused by a cropped calibration file (from a previous run) on the ISISEnergyTransfer
  tab.
- Fixed a bug for custom detector grouping when the OSIRIS instrument is selected.
- Fixed a bug caused by incorrect masked detectors during a reduction of an individual runs. This
  could sometimes cause unexpected peaks in the output plots.

.. figure:: ../../images/TOSCA_Individual_Runs_Bug.png
  :class: screenshot
  :align: center
  :figwidth: 100%
  :alt: The individual runs TOSCA bug before and after being fixed.

- Fixed a bug caused by incorrect masked detectors during a reduction for a summed run. This would
  cause the summed runs intensity to be higher than that of the individual runs with the same run
  numbers.

.. figure:: ../../images/TOSCA_Summed_Run_Bug.png
  :class: screenshot
  :align: center
  :figwidth: 100%
  :alt: The summed run TOSCA bug before and after being fixed.


Bayes Interface
---------------

New Features
############

- Added an option to produce a contour plot from the output on the Stretch tab.
- Added an option to the settings GUI allowing you to turn off the restriction of input data
  based on their name.
- Added an option to the settings GUI allowing you to turn on error bars for the output plots.

Improvements
############

- Updated the plot output options in the Quasi tab by removing an unwanted 'Fit' option. The
  graph 'Fit.2' is also now plotted when you click *Plot Current Preview*.
- Fixed an issue where the sample logs were not being copied over to the result workspace in the
  ResNorm tab.
- Added the ability to load files with extension *_sqw* as Vanadium in the ResNorm tab.
- Updated the Quasi tab so that fit. 3 and diff. 3 are now stored in the fit workspaces. The
  probabilities for 3 peaks is now available in the probability workspace.

Bugfixes
########

- Fixed an issue where an unwanted 'Fit' plot was available in ResNorm when you click *Plot* in
  the output options.


Diffraction Interface
---------------------

Improvements
############

- Fixed an issue in :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>`
  by replacing any zeros within the vanadium file with a substitute value of 10% the minimum y value
  found within that file. This prevents infinity values being produced when dividing the input file
  by the vanadium file.

Bugfixes
########
- Fixed an unexpected error when using manual grouping.

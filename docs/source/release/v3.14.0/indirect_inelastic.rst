==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------

New Algorithms
##############
- :ref:`BASISCrystalDiffraction <algm-BASISCrystalDiffraction>` replaces :ref:`BASISDiffraction <algm-BASISDiffraction>`, now deprecated.
- :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>` obtains scattered elastic intensity versus momentum transfer and versus scattering angle.
- TOF correction for neutrons incorrectly interpreted as slow neutrons in :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>`
- Deprecated algorithm BASISReduction311 has been removed.
- :ref:`LoadEMU <algm-LoadEMU>` loader for an ANSTO EMU backscattering event file.

:ref:`Release 3.14.0 <v3.14.0>`

Data Analysis Interface
-----------------------

Improvements
############

- The Run button is now above the output options, and is disabled during fitting along with the output buttons.
- The Run button in the Data Analysis tabs is now above the output options, and is disabled during fitting.
- The Fit Single Spectrum buttons in the Data Analysis tabs MSDFit, ConvFit, I(Q,t)Fit and F(Q)Fit are now disabled
  during fitting.
- When the InelasticDiffSphere, InelasticDiffRotDiscreteCircle, ElasticDiffSphere or ElasticDiffRotDiscreteCircle
  Fit Types are selected in the ConvFit Tab, the Q values are retrieved from the workspaces, preventing a crash
  when plotting a guess.
- The Plot buttons in MSDFit, I(Q,t)Fit, ConvFit and F(Q)Fit are disabled after a Run when the result workspace only 
  has one data point to plot.
- There is now an option to choose which output parameter to plot in MSDFit.
- An option to skip the calculation of Monte Carlo Errors on the I(Q,t) Tab has been added.
- During the calculation of Monte Carlo Errors, a progress bar is now shown.
- In the I(Q,t) Tab, it is now possible to select which spectrum you want to plot for Plot Spectrum.
- In the I(Q,t) Tab, it is now possible to select a range of spectra for a Tiled Plot. The interface allows a
  maximum of 18 plots.
- The WorkspaceIndex and Q value in the FitPropertyBrowser are now updated when the Plot Spectrum number is changed.
  This improvement can be seen in ConvFit when functions which depend on Q value are selected.

Bugfixes
########

- The workspace(s) loaded into F(Q) Fit are checked for EISF or Width values, and an error message is displayed
  if neither are present. This prevents an unexpected crash.
- The parameter values for a selected spectrum are now updated properly when a Fit is run using the Fit String
  option in ConvFit.
- An unexpected crash is prevented when Plot Current Preview is clicked when no data is loaded. A meaningful error
  message is now displayed.
- The Probability Density Functions (PDF) workspaces for the FABADA minimiser in ConvFit no longer overwrite each other.
  Various other improvements in the display of the FABADA PDF's have also been finished.
- Loading a resolution file before a reduced file in ConvFit no longer causes an unexpected error.
- A bug where fixed parameters don't remain fixed when using the FABADA minimizer in ConvFit has been corrected.
- The expression for the Fit type Yi in MSDFit was incorrect and has now been corrected.
- The x-axis labels in the output plots for MSDFit are now correct.
- An unexpected error is now prevented when clicking Plot Guess from the Display combo box in ConvFit without first loading 
  a reduced file.


Data Corrections Interface
--------------------------

Improvements
############

- Added 'Interpolation' combobox to Calculate Monte Carlo Absorption. This allows the method of interpolation
  to be selected. Allowed values: ['Linear', 'CSpline'].
- Added 'MaxScatterPtAttempts' spinbox to Calculate Monte Carlo Absorption. This sets the maximum number of
  tries to be made to generate a scattering point.
- In the Calculate Monte Carlo Absorption Tab, all of the options in the Monte Carlo section are now read from
  an instrument parameter files once a file has been loaded.
- The option to choose which spectrum to Plot Spectrum for is now available in the ContainerSubtraction Tab and
  ApplyAbsorptionCorrections Tab.


Data Reduction Interface
------------------------

Improvements
############

- Added 'Default' detector grouping option in ISISEnergyTransfer for TOSCA, to allow a default grouping
  using the grouping specified in the Instrument Parameter File.
- ISISEnergyTransfer now allows overlapping detector grouping.
- The Run button has been moved to be above the output options. The run button, save button and plotting options
  are now disabled while a tab is running or plotting.
- It is now possible to choose which spectrum to Plot Output for in the S(Q,w) Tab.


Bayes Interface
---------------

New Features
############
- It is now possible to produce a contour plot from the output on the Stretch Tab.

Improvements
############

- The Run button is now above the output options.
- The Run, Plot and Save buttons are now disabled while running and plotting is taking place.
- There is no longer a plot output option for 'Fit' in the Quasi Tab. The graph 'Fit.2' is also now plotted when you click
  Plot Current Preview.
- The sample logs are now copied over properly for the result workspace in the ResNorm tab.
- Sqw files can now be loaded as Vanadium in the ResNorm interface.

Bugfixes
########

- An unwanted 'Fit' plot is no longer plotted in ResNorm when you click `Plot` in the output options.


Simulations Interface
---------------------

Improvements
############

- The Run button is now above the output options.
- The Run, Plot and Save buttons are now disabled while running and plotting is taking place.


Diffraction Interface
---------------------

Improvements
############

- The Run button is now above the output options.
- The Run, Plot and Save buttons are now disabled while running and plotting is taking place.


Tools Interface
---------------

Improvements
############

- The Run button has been moved in each of the Tools tabs, and is disabled while running.

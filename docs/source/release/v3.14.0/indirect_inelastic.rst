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
- The Fit Single Spectrum buttons in the Data Analysis tabs MSDFit, ConvFit, I(Q,t)Fit and F(Q)Fit are now disabled
  during fitting.
- When the InelasticDiffSphere, InelasticDiffRotDiscreteCircle, ElasticDiffSphere or ElasticDiffRotDiscreteCircle
  Fit Types are selected in the ConvFit Tab, the Q values are retrieved from the workspaces, preventing a crash 
  when plotting a guess.
- The Plot buttons in MSDFit and F(Q)Fit are disabled after a Run when the result workspace only has one
  data point to plot.
- There is now an option to choose which output parameter to plot in MSDFit.
- An option to skip the calculation of Monte Carlo Errors on the I(Q,t) Tab has been added.
- During the calculation of Monte Carlo Errors, a progress bar is now shown.

Bugfixes
########

- The parameter values for a selected spectrum are now updated properly when a Fit is run using the Fit String 
  option in ConvFit.
- An unexpected crash is prevented when Plot Current Preview is clicked when no data is loaded. A meaningful error
  message is now displayed.
- The Probability Density Functions (PDF) workspaces for the FABADA minimiser in ConvFit no longer overwrite each other. 
  Various other improvements in the display of the FABADA PDF's have also been finished.
- The expression for the Fit type Yi in MSDFit was incorrect and has now been corrected.
- The x-axis labels in the output plots for MSDFit are now correct.


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

Improvements
############

- The Run button is now above the output options.
- The Run, Plot and Save buttons are now disabled while running and plotting is taking place.

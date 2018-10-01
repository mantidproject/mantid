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
- Deprecated algorithm BASISReduction311 has been removed.

:ref:`Release 3.14.0 <v3.14.0>`

Data Analysis Interface
-----------------------

Improvements
############

- The Run button in the Data Analysis tabs is now above the output options, and is disabled during fitting.
- The Fit Single Spectrum buttons in the Data Analysis tabs MSDFit, ConvFit, I(Q,t)Fit and F(Q)Fit are now disabled
  during fitting.
- When the InelasticDiffSphere, InelasticDiffRotDiscreteCircle, ElasticDiffSphere or ElasticDiffRotDiscreteCircle
  Fit Types are selected in the ConvFit Tab, the Q values are retrieved from the workspaces, preventing a crash 
  when plotting a guess.
- The Plot Result buttons in MSDFit and F(Q)Fit are disabled after a Run when the result workspace only has one
  data point to plot.
- An option to skip the calculation of Monte Carlo Errors on the I(Q,t) Tab has been added.
- During the calculation of Monte Carlo Errors, a progress bar is now shown.

Bugfixes
########

- The parameter values for a selected spectrum are now updated properly when a Fit is run using the Fit String 
  option in ConvFit.
- An unexpected crash is prevented when Plot Current Preview is clicked when no data is loaded. A meaningful error
  message is now displayed.


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

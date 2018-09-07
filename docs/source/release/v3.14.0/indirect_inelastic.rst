==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 3.14.0 <v3.14.0>`

Data Corrections Interface
--------------------------

Improvements
############

- Added 'Interpolation' combobox to Calculate Monte Carlo Absorption. This allows the method of interpolation 
  to be selected. Allowed values: ['Linear', 'CSpline'].
- Added 'MaxScatterPtAttempts' spinbox to Calculate Monte Carlo Absorption. This sets the maximum number of 
  tries to be made to generate a scattering point.

Data Analysis Interfaces
-------------------------

Bugfixes
########

- The parameter values for a selected spectrum are now updated properly when a Fit is run using the Fit String 
  option in ConvFit.

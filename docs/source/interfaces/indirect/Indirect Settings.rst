﻿.. _interface-indirect-settings:

Indirect Settings
=================

.. contents:: Table of Contents
  :local:

Overview
--------

Provides options which allow you to customise and save the behaviour of features on the
Indirect interfaces.

.. interface:: Settings
  :width: 450

Action Buttons
--------------

Apply
  This will apply the changes made to the interface. The settings window will not be exited.

OK
  This will apply the changes made to the interface and then exits the settings window.

Cancel
  This disregards any changes which have not been applyed and exits the window.


General
-------

This section contains options which are widespread.

Options
~~~~~~~

Facility
  Allows you to choose the selected facility.


Interface Settings
------------------

This section contains settings which are more specific to the Indirect interfaces.

Options
~~~~~~~

Restrict allowed input files by name
  This will allow you to load input files with any names if unticked. Previously, only
  input files with certain end extensions such as *_red* were allowed to be loaded.
  **Keeping this turned on is highly recommended**. See the glossary below to understand
  the restrictions on data when this option is ticked.

Plot error bars for external plots
  If ticked, this will ensure that error bars are plotted on any plots which are plotted
  externally (i.e. in a separate window) from the indirect interfaces.


Glossary of Allowed Suffixes
----------------------------

+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| Suffix       | X axis                       | Histogram axis                                 | Produced on                                                                                                        |
+==============+==============================+================================================+====================================================================================================================+
| _red         | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISEnergyTransfer                                    |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _res         | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISCalibration                                       |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _calib       | Single Point                 | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISCalibration                                       |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _sqw         | EnergyTransfer (:math:`meV`) | Q (:math:`A^-1`)                               | :ref:`Data Reduction <interface-indirect-data-reduction>` in S(Q, w)                                               |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _Corrections | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Corrections <interface-indirect-corrections>` in CalculatePaalmanPings or CalculateMonteCarloAbsorption |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _eq          | Q (:math:`A^-1`)             | Sample Environment variable (e.g. Temperature) | :ref:`Data Analysis <interface-indirect-data-analysis>` in Elwin                                                   |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _elf         | Intensity                    | Q (:math:`A^-1`)                               | :ref:`Data Analysis <interface-indirect-data-analysis>` in Elwin                                                   |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _iqt         | Time (:math:`ns`)            | Spectrum Number or Q (:math:`A^-1`)            | :ref:`Data Analysis <interface-indirect-data-analysis>` in I(Q,t)                                                  |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| _Result      | Q (:math:`A^-1`)             | Fit Parameter Name                             | :ref:`Data Analysis <interface-indirect-data-analysis>` in MSDFit, IqtFit, ConvFit or F(Q)Fit                      |
+--------------+------------------------------+------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+

When **Restrict allowed input files by name** is ticked, the input data is restricted by name
according to the suffixes below.

Indirect Bayes Suffixes
~~~~~~~~~~~~~~~~~~~~~~~
ResNorm
#######

Vanadium Suffixes
  _red, _sqw

Resolution Suffixes
  _res

Quasi
#####

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res, _red, _sqw

Stretch
#######

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res

Data Analysis Suffixes
~~~~~~~~~~~~~~~~~~~~~~
Elwin
#####

Input Suffixes
  _red, _sqw

MSDFit
######

Sample Suffixes
  _eq

I(Q,t)
######

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res, _red, _sqw

I(Q,t)Fit
#########

Sample Suffixes
  _iqt

ConvFit
#######

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res, _red, _sqw

F(Q)Fit
#######

Sample Suffixes
  _Result

Data Corrections Suffixes
~~~~~~~~~~~~~~~~~~~~~~~~~
Container Subtraction
#####################

Sample Suffixes
  _red, _sqw, _elf

Container Suffixes
  _red, _sqw, _elf

Calculate Paalman Pings
#######################

Sample Suffixes
  _red, _sqw

Container Suffixes
  _red, _sqw

Calculate Monte Carlo Absorption
################################

Sample Suffixes
  _red, _sqw

Container Suffixes
  _red, _sqw

Apply Absorption Corrections
############################

Sample Suffixes
  _red, _sqw

Container Suffixes
  _red, _sqw

Corrections Suffixes
  _Corrections

Data Reduction Suffixes
~~~~~~~~~~~~~~~~~~~~~~~
ISIS Energy Transfer
####################

Calibration Suffixes
  _calib

ILL Energy Transfer
###################
No restrictions.

ISIS Calibration
################
No restrictions.

ISIS Diagnostics
################

Calibration Suffixes
  _calib

Transmission
############
No restrictions.

Symmetrise
##########

Input Suffixes
  _red

S(Q,w)
######

Input Suffixes
  _red

Moments
#######

Input Suffixes
  _sqw

Diffraction Suffixes
~~~~~~~~~~~~~~~~~~~~
No restriction of input data by name takes place.

Simulation Suffixes
~~~~~~~~~~~~~~~~~~~
No restriction of input data by name takes place.

Tools Suffixes
~~~~~~~~~~~~~~
No restriction of input data by name takes place.


.. categories:: Interfaces Indirect

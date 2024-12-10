.. _inelastic-interface-settings:

Indirect Settings
=================

.. contents:: Table of Contents
  :local:

Overview
--------

Provides options which allow you to customise and save the behaviour of features on the
Indirect interfaces.

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


Basic
-----

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

Load Workspace History
  If this option is unticked, the history of the workspace won't be loaded, resulting in faster
  workspace loading speeds.
  For many Indirect/Inelastic interfaces, workspaces to be processed by the interface are preloaded into the ADS.
  The history of a workspace is loaded by default when a workspace is loaded into the ADS from the interface.
  In some cases, knowledge of the history may not be necessary for later analysis.


Advanced
--------

This section contains settings which are for use by more advanced users of the software.

Options
~~~~~~~

Developer Feature Flags
  This will allow you to provide a "development flag" that serves as a toggle for specific
  features which are currently under development. This will facilitate rapid prototyping
  of new features, and will also shorten the feedback loop for developers. Note that this
  option should only be used for testing features which are still under development. When
  users are satisfied with the new feature, the "development flag" in question should be
  removed from the software, and the feature should be made permanently available.
  **The interface must be restarted for changes to take effect**.


Glossary of Allowed Suffixes
----------------------------

+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| Suffix       | X axis                       | Histogram axis                                 | Produced on                                                                                                         |
+==============+==============================+================================================+=====================================================================================================================+
| _red         | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISEnergyTransfer                                     |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _res         | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISCalibration                                        |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _calib       | Single Point                 | Spectrum Number                                | :ref:`Data Reduction <interface-indirect-data-reduction>` in ISISCalibration                                        |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _sqw         | EnergyTransfer (:math:`meV`) | Q (:math:`A^-1`)                               | :ref:`Data Reduction <interface-indirect-data-reduction>` in S(Q, w)                                                |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _Corrections | EnergyTransfer (:math:`meV`) | Spectrum Number                                | :ref:`Data Corrections <interface-inelastic-corrections>` in CalculateMonteCarloAbsorption                          |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _eq          | Q (:math:`A^-1`)             | Sample Environment variable (e.g. Temperature) | :ref:`Data Processor <interface-inelastic-data-processor>` in Elwin                                                 |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _elf         | Intensity                    | Q (:math:`A^-1`)                               | :ref:`Data Processor <interface-inelastic-data-processor>` in Elwin                                                 |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _iqt         | Time (:math:`ns`)            | Spectrum Number or Q (:math:`A^-1`)            | :ref:`Data Processor <interface-inelastic-data-processor>` in I(Q,t)                                                |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| _Result      | Q (:math:`A^-1`)             | Fit Parameter Name                             | :ref:`QENS Fitting <interface-inelastic-qens-fitting>` in MSD, I(Q,t), Convolution or Function(Q)                   |
+--------------+------------------------------+------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+

When **Restrict allowed input files by name** is ticked, the input data is restricted by name
according to the suffixes below.

Bayes Fitting Suffixes
~~~~~~~~~~~~~~~~~~~~~~
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

QENS Fitting Suffixes
~~~~~~~~~~~~~~~~~~~~~

MSD
###

Sample Suffixes
  _eq

I(Q,t)
######

Sample Suffixes
  _iqt

Convolution
###########

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res, _red, _sqw

Function(Q)
###########

Sample Suffixes
  _Result

Corrections Suffixes
~~~~~~~~~~~~~~~~~~~~
Container Subtraction
#####################

Sample Suffixes
  _red, _sqw, _elf

Container Suffixes
  _red, _sqw, _elf

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

Data Processor Suffixes
~~~~~~~~~~~~~~~~~~~~~~~
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

Elwin
#####

Input Suffixes
  _red, _sqw

I(Q,t)
######

Sample Suffixes
  _red, _sqw

Resolution Suffixes
  _res, _red, _sqw

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

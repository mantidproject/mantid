=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- Algorithm :ref:`algm-SaveISISReflectometryORSO` has been added to allow reduced Reflectometry data from experiments at the ISIS facility to be saved into the ORSO ASCII format.

Bugfixes
--------
- Fix an error where live data reduction via the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` would not work if ROI Detector IDs had been specified in the interface.
- It is now once again possible to provide a negative ``dQ/Q`` value on the Experiment Settings and Runs tab of the
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>` to indicate that linear binning should be performed.
- Fixes a regression introduced in Mantid version 6.8 where creating new batch tabs or deleting all batch tabs in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` causes Mantid to crash.
- Workbench will no longer crash if invalid inputs are given in the options column of the Runs Table in the
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>`.
- Fixes a bug with live data on OFFSPEC where the alternative method for looking up the vertical gap for slit 2 in :ref:`algm-ReflectometryReductionOneLiveData` was incorrectly returning the vertical gap for slit 1.
- Workbench will no longer crash if an attempt is made to save a batch to an invalid location.

:ref:`Release 6.9.0 <v6.9.0>`
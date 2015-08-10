Engineering Diffraction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

This custom interface integrates several tasks related to engineeering
diffraction. The interface is under heavy development, and at the
moment it only provides calibration related functionality. The
following sections will describe the different sections or tabs of the
interface.

.. interface:: Engineering Diffraction
  :align: center
  :width: 400

General options
^^^^^^^^^^^^^^^
Instrument
 Select the instrument. Only ENGIN-X (ISIS) is supported in this version.

?
  Shows this documentation page.

Close
  Close the interface

Calibration
-----------

This tab provides a graphical interface to calculate calibrations and
visualize them.

It is possible to load an existing calibration (as a CSV file) and to
generate a new calibration file (which becomes the new current
calibration).

Parameters
^^^^^^^^^^

These parameters are required to generate new calibrations:

Vanadium#
  Number of the vanadium run used to correct calibration and experiment
  runs.

CeO2#
  Number of the Ceria (short) run used to calibrate experiment runs.

Settings
--------

Controls several settings, including the input folders where the
instrument run files can be found. Other advanced options can also be
controlled to customize the way the underlying calculations are
performed.

Calibration Parameters
^^^^^^^^^^^^^^^^^^^^^^

The calibration settings are organized in three blocks:
1. Input directories
2. Pixel calibration
3. Advanced settings

The input directories will be used when looking for run files
(Vanadium and Ceria). They effectivel ybecome part of the search path
of Mantid when using this interface.

The pixel calibration file contains the calibration of every pixel of
all banks, as produced by the algorithm :ref:`EnggCalibrateFull
<algm-EnggCalibrateFull>`.

The Following advanced settings are available to customize the
behavior of this interface:

Force recalculate
  Vanadium corrections will be recalculated even if previous correction
  results are available for the current Vanadium run number.

Tempalte .prm file
  Use a different template file for the output GSAS IPAR that is
  generated in the Calibration tab.

Rebin for Calibrate
  This sets a rebin width parameter that can be used by underlying
  algorithms such as :ref:`EnggCalibrate <algm-EnggCalibrate>` and
  :ref:`EnggFocus <algm-EnggFocus>`

Algorithms
----------

Most of the functionality provided by this interface is based on the
engineering diffraction Mantid algorithms (which are named with the
prefix *Engg*. This includes :ref:`EnggCalibrate
<algm-EnggCalibrate>`, :ref:`EnggCalibrateFull
<algm-EnggCalibrateFull>`, :ref:`Engg <algm-EnggCalibrateFull>`,
explained in detail in the Mantid algorithm description.

.. categories:: Interfaces Diffraction

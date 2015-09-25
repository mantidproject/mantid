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

Vanadium #
  Number of the vanadium run used to correct calibration and experiment
  runs.

Calibration sample #
  Number of the calibration sample run (for example Ceria run) used to
  calibrate experiment runs.

Focus
-----

Here it is possible to focus run files, provided a run number or run
file. The focusing process uses the algorithm :ref:`EnggFocus
<algm-EnggFocus>`. In the documentation of the algorithm you can find
the details on how the input runs are focused.

The interface will also create workspaces that can be inspected in the
workspaces window:

1. The *engg_focusing_input_ws workspace* for the data being focused
2. The *engg_focusing_input_ws workspace* for the corresponding
   focused data

Three focusing alternatives are provided:

1. Normal focusing, which includes all the spectra from the input run.
2. Cropped focusing, where several spectra or ranges of spectra can
   be specified, as a list separated by commas.
3. Texture focusing, where the *texture* group of detectors is given
   in a Detector Gropuing File.

Depending on the alternative chosen, the focusing operation will
include all the selected banks and all the spectra present in the
input runs (first alternative: normal focusing), all the banks but
only a list of spectra provided manually (second alternative: cropped
focusing), or a user-defined list of banks provided in a file (third
alternative: texture focusing)

For texture focusing, the detector grouping file is a text (csv) file
with one line per bank. Each line must contain at least two numeric
fields, where the first one specifies the bank ID, and the second and
subsequent ones different spectrum numbers or ranges of spectrum
numbers. For example:
::
   # Bank ID, spectrum numbers
   1, 205-210
   2, 100, 102, 107
   3, 300, 310, 320-329, 350-370

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
  If this is enabled, Vanadium corrections will be recalculated even
  if previous correction results are available for the current Vanadium
  run number. This is not required unless a modification is done to the
  original Vanadium run file, or there is a change in the algorithms
  that calculate the corrections

Tempalte .prm file
  By changing this option you can Use a different template file for the
  output GSAS IPAR that is generated in the Calibration tab.

Rebin for Calibrate
  This sets a rebin width parameter that can be used by underlying
  algorithms such as :ref:`EnggCalibrate <algm-EnggCalibrate>` and
  :ref:`EnggFocus <algm-EnggFocus>`

Algorithms
----------

Most of the functionality provided by this interface is based on the
engineering diffraction Mantid algorithms (which are named with the
prefix *Engg*). This includes :ref:`EnggCalibrate
<algm-EnggCalibrate>`, :ref:`EnggCalibrateFull
<algm-EnggCalibrateFull>`, :ref:`EnggVanadiumCorrections
<algm-EnggVanadiumCorrections>`, :ref:`EnggFocus <algm-EnggFocus>`,
and several other algorithms, explained in detail in the Mantid
algorithms documentation.

.. categories:: Interfaces Diffraction

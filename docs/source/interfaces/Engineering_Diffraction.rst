Engineering Diffraction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

This custom interface integrates several tasks related to engineeering
diffraction. It provides calibration and focusing functionality which
can be expected to expand for next releases as it is under active
development. The following sections describe the different tabs or
functionality areas of the interface.

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
2. The *engg_focusing_output_ws... workspace* for the corresponding
   focused data (where the ... denotes a suffix explained below).

Three focusing alternatives are provided:

1. Normal focusing, which includes all the spectra from the input run.
2. Cropped focusing, where several spectra or ranges of spectra can
   be specified, as a list separated by commas.
3. Texture focusing, where the *texture* group of detectors is given
   in a Detector Gropuing File.

Depending on the alternative chosen, the focusing operation will
include different banks and/or combinations of spectra (detectors). In
the firs option, normal focusing, all the selected banks and all the
spectra present in the input runs are considered. In the second
alternative, cropped focusing, all the banks are considered in
principle but only a list of spectra provided manually are
processed. In the third option, *texture focusing*, the banks are
defined by a user-defined list of banks and corresponding spectrum IDs
provided in a file. For these alternatives, the output focused
workspace will take different suffixes: *_bank_1, _bank_2*, and so on
for normal focusing, *_cropped* for cropped focusing, and
*_texture_bank_1, _texture_bank_2*, and so on for texture focusing
(using the bank IDs given in the detector grouping file).

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

Output
^^^^^^

Under the output section, the user is provided with an option of
plotting data in three different formats. One Window - Replacing
Plots: will replace the previous graph and plot a new graph on top.
One Window - Waterfall: will plot all the generated focused
workspace graphs in one window which can be useful while comparing
various graphs. The Multiple Windows: will plot graph in
separate windows. However, user may also change the Plot Data
Representation drop-down box while a run is being carried out. This
will update the interface and plot workspace according to the new
given input. For example, if a user has selected One Window -
Replacing Plots and then decides to change it to One Window -
Waterfall during a run, the interface will carry on by plotting
Waterfall within the same window.

The user also has an option of generated GSS, XYE and OpenGenie
formatted file by clicking the Output Files checkbox. This will
generated three different files for each focused output workspace
in Mantid. These files can be found with appropriate name at location:
C:\EnginX_Mantid\User\236516\Focus on Windows, the
EnginX_Mantid folder can be found on Desktop/Home on other platforms.

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

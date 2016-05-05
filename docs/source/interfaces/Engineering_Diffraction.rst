Engineering Diffraction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

This custom interface integrates several tasks related to engineering
diffraction. It provides functionality for calibration, focusing, and
pre-processing of event mode data. Further extensions can be expected
for next releases as it is under active development. Feedback is very
much welcome. The following sections describe the different tabs or
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

RB Number
  To enable the GUI specify a RB Number, the RB number will be used for the
  output paths, so that files from different users and/or experiments can
  be kept separate.

* Red Star Sign
  If a red star sign is displayed next to the Browse Button, it is mostly
  likely because the file specified has not been found. Error message
  can be viewed by hovering over the red star sign.


Calibration
-----------

This tab provides a graphical interface to calculate calibrations and
visualize them.

It is possible to load an existing calibration (as a CSV file) and to
generate a new calibration file (which becomes the new current
calibration).

With the help of Cropped Calibration user can also calibrate according
to bank or by setting the Spectrum Numbers once the Cropped Calibration group
box has been enabled.

The plot Calibrated Workspace check-box will enable user to plot
vanadium curves and Ceria peaks. For Ceria peaks there will be two
workspace generated and plotted, one for each bank, whereas for
cropped calibration there will only be only one workspace generate
and plotted, depending on the selected bank or provided Spectrum
IDs. The workspace contains difc and tzero data which is then
utilised to plot the Ceria peaks per bank, the graph will plot Peaks
Fitted and Difc/TZero Straight Line for comparison. More information
regarding the fit peaks can be found on the
:ref:`EnggFitPeaks<algm-EnggFitPeaks>` documentation.

Parameters
^^^^^^^^^^

These parameters are required to generate new calibrations:

Vanadium #
  Number of the vanadium run used to correct calibration and experiment
  runs.

Calibration sample #
  Number of the calibration sample run (for example Ceria run) used to
  calibrate experiment runs.

Bank Name:
  This parameter is only required when Cropped Calibration is being
  carried out. The bank name can be selected from a drop down list with
  option of "North" and "South", which are equivalently to 1 and 2
  respectively. However the Bank Name drop down list is set to
  "Enable Spectrum-Nos" by default. This option cannot be used together
  with Spectrum Nos, as they overlap. For the convenience of users, the
  GUI will only enable Spectrum Numbers and Customise Bank Name text-fields
  when Bank Name drop down list is set to "Enable Spectrum-Nos"

Spectrum Numbers:
  This parameter is only required when Cropped Calibration is being
  carried out, the parameter will set the spectrum numbers of the
  detectors, that should be considered in the calibration while all
  others will be ignored. This option cannot be used together with
  Bank Name, as they overlap. You may also give multiple ranges, for
  example: "0-100", or "0-9", "150-750".

Customise Bank Name:
  This parameter is only required when Cropped Calibration is being
  carried out with Spectrum Numbers, the parameter will set the workspace
  and .his file name according to this Bank Name provided by the user.
  However if the user does not provide a personalised name, the
  interface will use "cropped" as a default bank name.

The calibration process depends on several additional parameters and
settings which can be modified in the *Settings* section (tab), see
below for details.

.. _focus-Engineering_Diffraction-ref:

Focus
-----

Here it is possible to focus run files, by providing a run number or a
range of run number to enable multi-run focusing, along with that the
user may also select the files with the help of Browse button.

The focusing process uses the algorithm :ref:`EnggFocus
<algm-EnggFocus>`. In the documentation of the algorithm you can find
the details on how the input runs are focused.

The interface will also create workspaces that can be inspected in the
workspaces window:

1. The *engggui_focusing_input_ws workspace* for the data being focused
2. The *engggui_focusing_output_ws... workspace* for the corresponding
   focused data (where the ... denotes a suffix explained below).

Three focusing alternatives are provided:

1. Normal focusing, which includes all the spectra from the input run.
2. Cropped focusing, where several spectra or ranges of spectra can
   be specified, as a list separated by commas.
3. Texture focusing, where the *texture* group of detectors is given
   in a Detector Grouping File.

Depending on the alternative chosen, the focusing operation will
include different banks and/or combinations of spectra (detectors). In
the firs option, normal focusing, all the selected banks and all the
spectra present in the input runs are considered. In the second
alternative, cropped focusing, all the banks are considered in
principle but only a list of spectra provided manually are
processed. In the third option, *texture focusing*, the banks are
defined by a user-defined list of banks and corresponding spectrum Nos
provided in a file. For these alternatives, the output focused
workspace will take different suffixes: *_bank_1, _bank_2*, and so on
for normal focusing, *_cropped* for cropped focusing, and
*_texture_bank_1, _texture_bank_2*, and so on for texture focusing
(using the bank IDs given in the detector grouping file).

Cropped focusing and Texture focusing have been disabled by default to
declutter the interface, but each section can be enabled simply by
ticking the check-box next to Focus Cropped and Focus Texture.

For texture focusing, the detector grouping file is a text (csv) file
with one line per bank. Each line must contain at least two numeric
fields, where the first one specifies the bank ID, and the second and
subsequent ones different spectrum numbers or ranges of spectrum
numbers. For example::

   # Bank ID, spectrum numbers
   1, 205-210
   2, 100, 102, 107
   3, 300, 310, 320-329, 350-370

When a focus run process is being carried out, Focus Stop button will
be enabled. Focus Stop button will allow the user to abort once the
current focus run process has been completed. Inside the *Result Log*
a warning message will be displayed with last successful run and total
number of focus runs that could not be processed.

Run Number
^^^^^^^^^^
The run provided to focus can be for example 228061-228063, this will
run all the files within the given range as long as the file
directories are included in the
`User Directories <http://www.mantidproject.org/SplittersWorkspace>`_.
The user may also provide an input of 228061-3 or 228061, 228062,
2280623 which should work the same way.

If a red star sign is displayed next to the Browse Button, it is mostly
likely because the file specified has not been found. Error message
can be viewed by hovering over the red star sign.

Checking the availability of all the files can take some time, for this
reason it is also possible that a file may not have been found but the
red star sign has not been displayed. If you manage to click Focus
before red sign is displayed, the interface will process the last valid
focus run instead.

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
`C:\EnginX_Mantid\User\236516\Focus` on Windows, the
EnginX_Mantid folder can be found on `Desktop/Home` on other platforms.

The Multiple Runs Focus Mode combo-box enables two alternative
focus mode. `Focus Individual Run Files Separately` is the default
option set, which allows user to run focus with multi-run files.
Whereas the `Focus Sum Of Files` option merges all the multi-run
number files together and applies the Focus Process to the merged
file.

Pre-processing
--------------

.. warning:: This is a new capability that is currently in a very
             early stage of definition and implementation. Not all
             options may be supported and/or consistent at the moment.

The focusing options can be applied directly to histogram data. For
event mode experiments, the event data (which would be loaded as event
workspaces in Mantid) need to be pre-processed.

The simplest pre-processing option is "regular time binning" which
will produce a histogram data workspace (as a :ref:`Workspace2D
<Workspace2D>`). The only parameter required is the bin width. The
workspace will be named with the following convention:

- *engggui_preproc_time_ws*

When the input run file contains multiple workspaces (it would be
loaded by :ref:`Load <algm-Load>` as multiple :ref:`EventWorkspace
<EventWorkspace>` workspaces) the output workspace will be a group
with the corresponding number of histogram workspaces, binned
separately. This is the case when the input run file comes from a
multi-period experiment. Note that the time bin can be a multiple of
the pulse time.

A different way of pre-processing event data is by rebinning
multi-period data by pulse times. In this case the input required is
the time step for the binning (the x axis of the output will be time
instead of time-of-flight). It is also possible to specify the number
of periods that will be processed (starting from the first one). This
type of pre-processing produces workspaces with the following naming
convention:

- *engggui_preproc_by_pulse_time_ws*

This tab uses the algorithms :ref:`Rebin <algm-Rebin>` and :ref:`Rebin
<algm-RebinByPulseTimes>` to bin the data in different ways when
converting event data into histogram data.

Fitting
-------

.. warning:: This is a new capability that is currently in a very
             early stage of definition and implementation. Not all
             options may be supported and/or consistent at the moment.

The Fitting tab provides a graphical interface which fits an expected
diffraction pattern and visualises them. The pastern is specified by
providing a list of dSpacing values where Bragg peaks are expected.
The algorithm :ref:`EnggFitPeaks<algm-EnggFitPeaks>` used in the
background fit peaks in those areas using a peak fitting function.

To use the Fitting tab, user is required to provide:

1. A focused file as Focus Run input by browsing or entering run number
2. List of expected peaks which can be either by browsing a (*CSV*) file
   or entering within the text-field simply click on the Fit button.

Parameters
^^^^^^^^^^

These parameters are required to process Fitting successfully:

Focused Run #:
  Focused workspace directory or selected with the help of browse button.
  User may also select the file by simply entering the file run number,
  which is located within the focused output directory.
  Focused workspace can be generated with the help of
  :ref:`focus-Engineering_Diffraction-ref` tab, the output folder
  directory can be set in the :ref:`setting-Engineering_Diffraction-ref`
  tab under the *Focusing settings* section.
  The interface will automatically select all the bank files found with the
  same run-number and update the Plot Bank combo-box and Bank list
  accordingly.

.. _ExpectedPeaks-Engineering_Diffraction-ref:

Peaks:
  A list of dSpacing values to be translated into TOF to find expected
  peaks. These peaks can be manually written or imported by selecting a
  (*CSV*) file.

Plot Bank/Bank List:
  These GUI widgets will only be enabled when multiple focused bank
  files are found within the working directory or focused output directory.
  This would enable user to select the desired bank which they would like to
  plot with the help of Plot Bank combo-box or Bank List.

Output
^^^^^^

Once the Fit button has been clicked, wait until the Fitting process has
completed and upon completion you should be able to view on the Fitting
tab plots the focused workspace in the background in black, whereas the
expected peaks plotted in various colours over lapping the focused
workspace peaks.

Within the :ref:`Preview-Engineering_Diffraction-ref` section user is
able to zoom-in or zoom-out as well as select, add and save peaks.

The interface will also generate workspaces that can be inspected in the
workspaces window:

1. The *engggui_fitting_fitpeaks_param* Table workspace
   with the parameters of the peaks found and fitted.
2. The *engggui_fitting_focused_ws* Focused workspace also loaded
   so the fitted data can be compared with focused data
3. The *engggui_fitting_single_peaks* workspace within each workspace
   index representing individual expected peak.

.. _Preview-Engineering_Diffraction-ref:

Preview
^^^^^^^
Once the fitting process has completed and you are able to view a
focused workspace with listed expected peaks on the data plot, Select
Peak button should should also be enabled.
By clicking Select Peak button the peak picker tool can be activated.
To select a peak simply hold *Shift* key and left-click on the graph
near the peak's center.

To get help selecting the center of the peak, you may set the peak
width by left-click and drag horizontally, while holding *Ctrl* key
as well. User may also zoom-in to the graph by holding left-click
and dragging on the plot, whereas zoom-out by simple left-click on
the plot.

When user is satisfied with the center position of the peak, you may
add the selected peak to :ref:`ExpectedPeaks-Engineering_Diffraction-ref`
list by clicking Add Peak button. User may rerun Fit process with
the new selected peaks or save the peaks list as *CSV* file by clicking
Save Peak List button.

.. _setting-Engineering_Diffraction-ref:

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
2. Pixel (full) calibration
3. Advanced settings

The input directories will be used when looking for run files
(Vanadium and Ceria). They effectively become part of the search path
of Mantid when using this interface.

The pixel (full) calibration file contains the calibration details of
every pixel of all banks, as produced by the algorithm
:ref:`EnggCalibrateFull <algm-EnggCalibrateFull>`. A default pixel
calibration file is provided with Mantid packages. This calibration
has been produced for the Vanadium and calibration sample (Ceria) runs
indicated in the name of the calibration file. Note that this
calibration is currently subject to changes, as the fitting of peaks
is being refined.

The Following advanced settings are available to customize the
behavior of this interface:

Force recalculate
  If this is enabled, Vanadium corrections will be recalculated even
  if previous correction results are available for the current Vanadium
  run number. This is not required unless a modification is done to the
  original Vanadium run file, or there is a change in the algorithms
  that calculate the corrections

Template .prm file
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
:ref:`EnggFitPeaks<algm-EnggFitPeaks>`
and several other algorithms, explained in detail in the Mantid
algorithms documentation.

.. categories:: Interfaces Diffraction

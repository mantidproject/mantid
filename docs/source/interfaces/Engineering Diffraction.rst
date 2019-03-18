.. _Engineering_Diffraction-ref:

Engineering Diffraction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------
This custom interface integrates several tasks related to engineering
diffraction. It provides functionality for calibration, focusing, and
pre-processing of event mode data. Further extensions can be expected
for future releases as it is under active development. Feedback is very
much welcome. The following sections describe the different tabs or
functionality areas of the interface.

.. interface:: Engineering Diffraction
  :align: center
  :width: 400

General options
^^^^^^^^^^^^^^^
RB Number
  To enable the GUI specify a RB Number (where "RB Number" usually
  denotes the experiment reference number at ISIS). This reference
  will be used for the output paths, so that files from different
  users and/or experiments can be kept separate.

Instrument
 Select the instrument. Only ENGIN-X (ISIS) is supported in this version.

?
  Shows this documentation page.

Close
  Close the interface

Status at the bottom of the interface
  A short message will be displayed which indicates whether the last
  important calculations finished successfully, and when the interface
  is busy calculating (calibrating, focusing, fitting, etc.).

* Red Star Sign
  If a red star sign is displayed next to the Browse Button, it is mostly
  likely because the file specified has not been found. Error message
  can be viewed by hovering over the red star sign.

.. _ui engineering calibration:

Calibration
-----------

This tab provides a graphical interface to calculate calibrations and
visualize them.

It is possible to:

- Generate a new calibration file (which becomes the new current
  calibration)
- Load an existing calibration from a GSAS instrument
  parameters file previously generated

For the current calibration, the following parameters are displayed:

- The vanadium run number
- The calibration sample run number
- The path to the output calibration file. 

This calibration file output is a GSAS instrument parameters file (IPARM/PAR/PRM). The interface
produces a calibration file containing all banks and in addition a calibration
file for every individual bank. All the calibration files are written
in the same directory.

With the help of Cropped Calibration user can also calibrate according
to specific banks or by setting the Spectrum Numbers once the Cropped Calibration group
box has been enabled.

The plot Calibrated Workspace check-box will enable user to plot
vanadium curves and Ceria peaks. For Ceria peaks there will be two
workspaces generated and plotted, one for each bank, whereas for a
cropped calibration there will only be one workspace generated
and plotted, depending on the selected bank or provided Spectrum
IDs. The workspace contains difc and tzero data which is then
utilised to plot the Ceria peaks per bank, the graph will plot Peaks
Fitted and Difc/TZero Straight Line for comparison. More information
regarding the fit peaks can be found on the
:ref:`EnggFitPeaks<algm-EnggFitPeaks>` documentation.

The calibration files are written into two different output
directories. First, they are written to a user specific directory
which for the ENGIN-X instrument on Windows systems is:

`C:\\EnginX_Mantid\\<Username>\\<RBNumber>\\Calibration`

On UNIX based platforms this path is:

`~/EnginX_Mantid/<Username>/<RBNumber>/Calibration`

They are also copied into a general (all) output directory:

`C:\\EnginX_Mantid\\Calibration` on Windows or

`~/EnginX_Mantid/Calibration` on UNIX platforms.

The calibration parameters for each bank are made available for user
inspection in a workspace named
**engggui_calibration_banks_parameters** which is updated when new
calibrations are loaded or calculated.

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
  option of "North" and "South", which are equivalent to 1 and 2
  respectively. 
  Custom bank mappings can be created by setting the Bank Name option
  to `Use spectrum numbers`. When the option *Use Spectrum Numbers* is 
  set a bank name must be specified in *Customise Bank Name*.
  
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
  and `.his` file name according to this Bank Name provided by the user.
  However if the user does not provide a personalised name, the
  interface will use "cropped" as a default bank name.

The calibration process depends on several additional parameters and
settings which can be modified in the *Settings* tab, see :ref:`setting-Engineering_Diffraction-ref` for details.

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
include different banks and/or combinations of spectra (detectors). 
The behavior for each option is as follows:

1. *Normal focusing* - All the selected banks and spectra present
in the input runs are considered. The output focused workspace will 
be named with suffixes such as *_bank_1, _bank_2*, and so on
 
2. *Cropped Focusing* - All the banks are considered in
principle but only a list of spectra provided manually are
processed. The output focused workspace will be named with 
the suffix *_cropped*.

3. *Texture Focusing* - The banks are selected by a user-defined
list of banks and corresponding spectrum numbers provided in a file. 
The output workspaces will be named with suffixes such as *_texture_bank_1,
_texture_bank_2*, and so on. These suffixes are determined by the 
bank IDs given in the detector grouping file.

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

The focused data files are saved in NeXus format into the user
specific and general directories (as with the calibration output
files). That is the files are written into
`C:\\EnginX_Mantid\\User\\<RBNumber>\\Calibration` and
`C:\\EnginX_Mantid\\Calibration` on Windows, or
`~/EnginX_Mantid/User/<RBNumber>/Calibration` and
`~/EnginX_Mantid/Calibration` on UNIX platforms.  See below for
additional, optional outputs.

Run Number
^^^^^^^^^^
The run provided to focus can be for example 228061-228063, this will
run all the files within the given range as long as the file
directories are included in the
`User Directories <http://www.mantidproject.org/ManageUserDirectories>`_.
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
plotting data in three different formats. 

- One Window - Replacing Plots: will replace the previous graph and plot a new graph on top.

- One Window - Waterfall: will plot all the generated focused workspace graphs in one window 
  which can be useful while comparing various graphs. 

- Multiple Windows - will plot graph in separate windows. 

However, user may also change the Plot Data representation drop-down box while a run is being carried out. This
will update the interface and plot workspace according to the new given input. 
For example, if a user has selected *One Window - Replacing Plots* and then decides to change it to *One Window -
Waterfall* during a run, the interface will carry on by plotting
Waterfall within the same window.

The user also has an option of saving GSS, XYE and OpenGenie formatted
files by clicking the Output Files checkbox. This will generate three
different files for each focused output workspace in Mantid. These
files can be found with appropriate name within:

`C:\\EnginX_Mantid\\<User>\\<RBNumber>\\Focus` on Windows or

`~/EnginX_Mantid/Foxus` on UNIX systems.

The files are also copied to the general (all) output directory which is

`C:\\EnginX_Mantid\\Focus` on Windows

`~/EnginX_Mantid/Focus` under on UNIX systems

`The Multiple Runs Focus Mode` combo-box enables two alternative
focus modes. `Focus Individual Run Files Separately` is the default
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

Focussing uses the algorithms :ref:`Rebin <algm-Rebin>` and :ref:`RebinByPulseTimes
<algm-RebinByPulseTimes>` to bin the data in different ways when
converting event data into histogram data.

Fitting
-------

.. warning:: This is a new capability that is currently in a very
             early stage of definition and implementation. Not all
             options may be supported and/or consistent at the moment.
			 
.. warning:: The input workspace must be converted into a focused file
			 first. The steps to complete this are found here: :ref:`focus-Engineering_Diffraction-ref`

The Fitting tab provides a graphical interface which fits an expected
diffraction pattern and visualises them. The pattern is specified by
providing a list of peak centre values where Bragg peaks are expected.
These values can have units of either TOF of dSpacing but **not** both.
The algorithm :ref:`EnggFitPeaks<algm-EnggFitPeaks>` is used to
background fit peaks in those areas using a peak fitting function.

To use the Fitting tab, user is required to follow these steps:

1. Load run(s) to perform fitting on by browsing for focused nexus
   files *User may click Load button to load the focused file to the
   canvas*
2. List of expected peaks which can be either by browsing a (*CSV*) file,
   manually selecting peaks from the canvas using peak picker tool and the "Add Peak to List" button after
   loading the focused file or by entering the peaks list within the text-field
3. Next click on the *Fit* button if you would like to fit single focused
   file or you can click *Fit All* button which will enable user to
   batch-process all the runs and banks when several files are loaded.
   *Fit All* process may also be used when a single run number is given
   or a file is browsed

.. _ExpectedPeaks-Engineering_Diffraction-ref:

Parameters
^^^^^^^^^^

These parameters are required to process Fitting successfully:

Focused Run files:
  .nxs files containing focused diffraction data. These should be the result
  of focusing data with the :ref:`focus-Engineering_Diffraction-ref` tab.

Peaks:
  A list of dSpacing values to be translated into TOF to find expected
  peaks. These peaks can be manually written or imported by selecting a
  (*CSV*) file.

Output
^^^^^^

Once the Fit button has been clicked Mantid will process the data. Please wait
until the Fitting process has completed. Upon completion you should be able to
view the Fitting tab which will contain:

- The focused workspace plotted in the background in gray crosses.
- The expected peaks plotted in various colours overlapping the
  focused workspace peaks.

Within the :ref:`Preview-Engineering_Diffraction-ref` section a user is
able to zoom-in or zoom-out as well as select, add and save peaks.

The interface will also generate workspaces that can be inspected in the
workspaces window:

1. The *engggui_fitting_fitpeaks_param* Table workspace
   with the parameters of the peaks found and fitted.
2. The *engggui_fitting_focused_ws* Focused workspace also loaded
   so the fitted data can be compared with focused data
3. The *engggui_fitting_single_peaks* workspace with each workspace
   index representing individual expected peak.

During the Fit process, :ref:`EnggSaveSinglePeakFitResultsToHDF5
<algm-EnggSaveSinglePeakFitResultsToHDF5>` algorithm will be utilised
to save *engggui_fitting_fitpeaks_param* TableWorkspace as a `hdf5`
file. There will one file per run, indexed by bank ID, and the file
will be found in the **Runs** directory of the user's output
directory. If **Fit All** was run on multiple runs, then an additional
file for all runs will be output, which is indexed first by run number
and then by bank ID.

In the plots, the x or abscissa axis is in d-spacing units, which are
more convenient for peak fitting than time-of-flight. However the run
files and the focus files are normally stored as time-of-flight
data. For this reason a conversion from the time-of-flight data to
d-spacing is required. The conversion is performed using the current
calibration of banks. The interface handles this internally and adds
special sample logs to the fitting workspaces
(*engggui_fitting_single_peaks* and *engggui_fitting_focused_ws*). By
inspecting the sample logs of these workspaces. The conversion is
performed using the `GSAS
<https://subversion.xray.aps.anl.gov/trac/pyGSAS>`__ equations, as
calculated by the algorithm :ref:`AlignDetectors
<algm-AlignDetectors>`

.. _Preview-Engineering_Diffraction-ref:

Preview
^^^^^^^
Once the fitting process has completed and you are able to view a
focused workspace with listed expected peaks on the data plot, the *Select
Peak* button should also be enabled. If the user choose to load the focus
workspace or if fitting fails with the given peaks then the focused
workspace will be plotted so that the user can select the peaks manually.

If you've run a fit but you can't see the reconstructed peaks, make sure
the checkbox **Plot fitted peaks** is checked - if the fit was successful,
then clicking this should show the results. Equally, if you want to hide
fitted peaks, just uncheck this box and they will disappear.

By clicking Select Peak button the peak picker tool can be activated.
To select a peak simply hold *Shift* key and left-click on the graph
near the peak's center.

To get help selecting the center of the peak, you may set the peak
width by left-click and drag horizontally, while holding *Ctrl* key
as well. Users may also zoom-in to the graph by holding left-click
and dragging a box on the plot, and zoom-out by left-clicking on
the plot.

When user is happy with the center position of the peak, you may
add the selected peak to :ref:`ExpectedPeaks-Engineering_Diffraction-ref`
list by clicking Add Peak button. User may rerun Fit process by
clearing peaks list using Clear button and manually selecting peaking
using Select Peak button or instead Save the peaks list in *CSV* file
by clicking Save button.

User may plot single peak fitting workspace in separate window by using
Plot To Separate Window button, if the *engggui_fitting_single_peaks*
is available.

.. _gsas-Engineering_Diffraction-ref:

GSAS Fitting
------------

.. warning:: This is a new capability that is currently in a very
             early stage of definition and implementation. Not all
	     options may be supported and/or consistent at the moment.

The GSAS tab provides a graphical interface to the Mantid algorithm
:ref:`GSASIIRefineFitPeaks <algm-GSASIIRefineFitPeaks>`. This allows
users to perform GSAS-style fitting on their data from Mantid.

The user must input the following files:

- **Focused run file(s)** - these must have been generated either by
  the **Fitting** tab or :ref:`EnggFocus <algm-EnggFocus>`.
- **Instrument Parameter File** - contains DIFA and DIFC GSAS
  constants, will probably be of ``.prm`` format
- **Phase file(s)** - contain crystallographic information about the
  sample in question. Currently only ``.cif`` files are supported

The following parameters are also required:

- **New GSAS-II Project** - GSASIIRefineFitPeaks creates a new
  ``.gpx`` project here, which can be opened and inspected from the
  GSAS-II GUI

  - Note, if running **Refine All** on more than one run, the run
    number and bank ID will be appended to the filename
- **GSAS-II Installation Directory**

  - This is the directory containing the GSAS-II executables and
    Python libraries. In particular, it must contain
    ``GSASIIscriptable.py``. This directory will normally be called
    `GSASII`, if GSAS-II was installed normally
  - You must have a version of GSAS-II from at least **February 2018**
    to use the GUI. See :ref:`Installing_GSASII` for how to install a
    compatible version
- **Refinement method** - can either be **Pawley** or
  **Rietveld**. Pawley refinement is currently under development, so
  Rietveld is recommended.

Optionally, you may also supply:

- **XMin** and **XMax** - the limits (in TOF) to perform fitting
  within
- **DMin** - the minimum dSpacing to use for refinement when
  performing Pawley refinement
- **Negative weight** - The weight for a penalty function applied
  during a Pawley refinement on resulting negative intensities.

To do a refinement, take the following steps:

1. Load a run by selecting the focused NeXuS file using the
   corresponding **Browse** button, then clicking **Load**. The run
   number and bank ID (for example ``123456_1``) should appear in the
   **Run Number** list in the **Preview** section. Click the label,
   and the run will be plotted
2. Select your input files, and input any additional parameters in the
   **GSASIIRefineFitPeaks Controls** section
3. Click **Run Refinement**. Once complete, fitted peaks for the run
   should be overplotted in the fitting area. In addition, Rwp
   (goodness of fit index), Sigma and Gamma (peak broadening
   coefficients) and lattice parameters should be displayed in the
   **Fit Results** section.

   - You can also click **Refine All** to run refinement on all runs
     loaded into GSAS tab

During the Fit process, :ref:`EnggSaveGSASIIFitResultsToHDF5
<algm-EnggSaveGSASIIFitResultsToHDF5>` algorithm will be utilised to
save the fit results, and also the parameters used, as a `hdf5`
file. There will be one file per run, indexed by bank ID, and the file
will be found in the **Runs** directory of the user's output
directory.

You can toggle the fitted peaks on and off with the **Plot Fitted
Peaks** checkbox, remove runs from the list with the **Remove Run**
button, and plot the run and fitted peaks to a larger, separate plot
using **Plot to separate window**.

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

Force recalculate all existing Vanadium files
  If this is enabled, Vanadium corrections will be recalculated even
  if previous correction results are available for the current Vanadium
  run number. This is not required unless a modification is done to the
  original Vanadium run file, or there is a change in the algorithms
  that calculate the corrections

Template .prm file
  By changing this option you can Use a different template file for
  the output GSAS IPAR/PAR/PRM that is generated in the Calibration
  tab.

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
and several other algorithms, explained in detail in the following Mantid
algorithms documentation pages.

.. categories:: Interfaces Diffraction

Indirect Data Reduction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

The Indirect Data Reduction interface provides the initial reduction that
is used to convert raw instrument data to S(Q, w) for analysis in the
Indirect Data Analysis and Indirect Bayes interfaces.

The tabs shown on this interface will vary depending on the current default
facility such that only tabs that will work with data from the facility are
shown, this page describes all the tabs which can possibly be shown.

.. interface:: Data Reduction
  :align: right
  :width: 350

Instrument Options
~~~~~~~~~~~~~~~~~~

Instrument
  Used to select the instrument on which the data being reduced was created on.

Analyser
  The analyser bank that was active when the experiment was run, or for which
  you are interested in seeing the results of.

Reflection
  The reflection number of the instrument setup.

.. tip:: If you need clarification as to the instrument setup you should use
  please speak to the instrument scientist who dealt with your experiment.

Action Buttons
~~~~~~~~~~~~~~

?
  Opens this help page.

Py
  Exports a Python script which will replicate the processing done by the current tab.

Run
  Runs the processing configured on the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

ISIS Energy Transfer
--------------------

.. interface:: Data Reduction
  :widget: tabISISEnergyTransfer

This tab provides you with the functionality to convert the raw data from the
experiment run into units of :math:`\Delta E`.

Options
~~~~~~~

Run Files
  Allows you to select the raw data files for an experiment. You can enter these
  either by clicking on the Browse button and selecting them, or entering the run
  numbers. Multiple files can be selected, multiple run numbers can be separated
  by a comma (,) or a sequence can be specified by using a dash (-).

Use Calib File & Calibration File
  Allows you to select a calibration file created using the Calibration tab.

Sum Files
  If selected the data from each raw file will be summed and from then on
  treated as a single run.

Load Log Files
  If selected the sample logs will be laoded from each of the run files.

Efixed
  This option allows you to override the default fixed final energy for the
  analyser and reflection number setting. This can be useful in correcting an
  offset peak caused by the sample being slightly out of centre.

Grouping
  Provides option of how data should be grouped.

Background Removal
  Allows removal of a background given a time-of-flight range.

Plot Time
  Creates a time of flight plot of the grouping of the spectra in the range
  defined in the Plot Time section, to include a single spectrum set the Spectra
  Min and Spectra Max selectors to the same value. Note that this first rebins
  the sample input to ensure that each detector spectrum has the same binning in
  order to be grouped into a single spectrum.

Detailed Balance
  Gives the option to perform an exponential correction on the data once it has
  been converted to Energy based on the temperature.

Scale
  Gives the option to scale the output by a given factor.

Spectra Min & Spectra Max
  Selecte the range of detectors you are interested in, default values are
  chosen based on the instrument and analyser bank selected.

Rebin Steps
  Select the type of rebinning you wish to perform.

Do Not Rebin
  If selected will disable the rebinning step.

Plot Output
  Allows the result to be plotted as either a spectrum plot or contour plot.

Fold Multiple Frames
  This option is only relevant for TOSCA. If checked, then multiple-framed data
  will be folded back into a single spectra, if unchecked the frames wil lbe
  left as is with the frame number given at the end of the workspace name.

Output in :math:`cm^{-1}`
  Converts the units of the energy axis from :math:`meV` to wave number
  (:math:`cm^{-1}`).

Select Save Formats
  Allows you to select multiple output save formats to save the reduced data as,
  in all cases the file will be saved in the defaut save directory.

Grouping
~~~~~~~~

The following options are available for grouping output data:

Custom
  Follows the same grouping patterns used in the :ref:`GroupDetectors <algm-GroupDetectors>` algorithm.
  An example of the syntax is 1,2+3,4-6,7-10

  This would produce spectra for: spectra 1, the sum of spectra 2 and 3, the sum of spectra 4-6 (4+5+6)
  and individual spectra from 7 to 10 (7,8,9,10)

Individual
  All detectors will remain on individual spectra.

Groups
  The detectors will automatically be divided into a given number of equal size groups. Any
  left over will be added as an additional group.

All
  All detectors will be grouped into a single spectra.

File
  Gives the option of supplying a grouping file to be used with the
  :ref:`GroupDetectors <algm-GroupDetectors>` algorithm.

Rebinning
~~~~~~~~~

Rebinning can be done using either a single step or multiple steps as described
in the sections below.

Single
######

.. interface:: Data Reduction
  :widget: pgSingleRebin

In this mode only a single binning range is defined as a range and width.

Multiple
########

.. interface:: Data Reduction
  :widget: pgMultipleRebin

In this mode multiple binning ranges can be defined using the rebin string syntax
used by the :ref:`Rebin <algm-Rebin>` algorithm.

ILL Energy Transfer
-------------------

.. interface:: Data Reduction
  :widget: tabILLEnergyTransfer

This tab handles the reduction of data from the IN16B instrument at the ILL.

Reduction Type
~~~~~~~~~~~~~~

There are two reduction types of IN16B data: Quasi-Elastic Neutron Scattering (QENS) or Fixed Window Scans (FWS),
and the latter can be either Elastic (EFWS) or Inelastic (IFWS).
If one or another reduction type is checked, the corresponding algorithm will be invoked
(see :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` and :ref:`IndirectILLReductionFWS <algm-IndirectILLReductionFWS>`).
There are several properties in common between the two, and several others that are specific to one or the other.
The specific ones will show up or disappear corresponding to the choice of the reduction type.

Common Options
~~~~~~~~~~~~~~

Input File
  Used to select the raw data in ``.nxs`` format. Note that multiple files can be specified following :py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>` instructions.

Detector Grouping
  Used to switch between grouping as per the IDF (*Default*) or grouping using a
  mapping file (*Map File*). This defines e.g. the summing of the vertical pixels per PSDs.

Background Subtraction
  Used to specify the background (i.e. empty can) runs to subtract. A scale factor can be applied to background subtraction.

Detector Calibration
  Used to specify the calibration (i.e. vanadium) runs to divide by.

Background Subtraction for Calibration
  Used to specify the background (i.e. empty can) runs to subtract from the vanadium calibration runs.

Output Name
  This will be the name of the resulting reduced workspace group.

Spectrum Axis
  This lets to convert the spectrum axis to elastic momentum transfer or scattering angle if desired.

Plot
  If enabled, will plot the result (of the first run) as a contour plot.

Save
  If enabled the reduced workspace group will be saved as a ``.nxs`` file in the default save
  directory.

QENS-only Options
~~~~~~~~~~~~~~~~~

Sum All Runs
  If checked, all the input runs will be summed while loading.

Crop Dead Monitor Channels
  If checked, the few channels in the beginning and at the end of the spectra, that contain zero monitor counts will be cropped out.
  As a result, the doppler maximum energy will be mapped to the first and last non-zero monitor channels, resulting in narrower peaks.
  Care must be taken with this option; since this alters the total number of bins,
  problems might occur while subtracting the background or performing unmirroring, if the number of dead monitor channels are different.

Calibration Peak Range
  This defines the integration range over the peak in calibration run in ``mev``.

Unmirror Options
  This is used to choose the option of summing of the left and right wings of the data, when recorded in mirror sense.
  See :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` for full details.
  Unmirror option 5 and 7 require vanadium alignment run.


FWS-only Options
~~~~~~~~~~~~~~~~

Observable
  This is the scanning ovservable, that will become the x-axis of the final result.
  It can be any numeric sample parameter defined in Sample Logs (e.g. sample.*) or a time-stamp string (e.g. start_time).
  It can also be the run number. It can not be an instrument parameter.

Sort X Axis
  If checked, the x-axis of the final results will be sorted.

Sum/Interpolate
  Both background and calibration have options to use the summed (averaged) or interpolated values over different observable points.
  Default behaviour is Sum. Interpolation is done using cubic (or linear for 2 measured values only) splines.
  If interpolation is requested, x-axis will be sorted automatically.

ISIS Calibration & Resolution
-----------------------------

.. interface:: Data Reduction
  :widget: tabISISCalibration

This tab gives you the ability to create Calibration and Resolution files.

The calibrtion file is normalised to an average of 1.

Options
~~~~~~~

Run No
  allows you to select a run for the function to use, either by selecting the
  *.raw* file with the Browse button or through entering the number in the box.

Plot Raw
  Updates the preview plots.

Intensity Scale Factor
  Optionally applies a scale by a given factor to the raw input data.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  If enabled will plot the result as a spectra plot.

Save
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Calibration
###########

Peak Min & Peak Max
  Selects the time-of-flight range corresponding to the peak. A default starting
  value is generally provided from the instrument's parameter file.

Back Min & Back Max
  Selects the time-of-flight range corresponding to the background. A default
  starting value is generally provided from the instrument's parameter file.

Resolution
##########

Create RES File
  If selected will create a resolution file when the tab is run.

Smooth RES
  If selected the :ref:`WienerSmooth <algm-WienerSmooth>` algorithm will be
  applied to the output of the resolution algorithm.

Scale RES
  Optionally apply a scale by a given factor to the resolution output.

Spectra Min & Spectra Max
  Allows restriction of the range of spectra used when creating the resolution
  curve.

Background Start & Background End
  Defines the time-of-flight range used to calculate the background noise.

Low, Width & High
  Binning parameters used to rebin the resolution curve.

Options
~~~~~~~

Input Files
  File for the calibration (e.g. vanadium) run. If multiple specified, they will be automatically summed.

Grouping
  Used to switch between grouping as per the IDF (*Default*) or grouping using a
  mapping file (*Map File*).

Peak Range
  Sets the integration range over the peak in :math:`meV`

Scale Factor
  Factor to scale the intensities with

ISIS Diagnostics
----------------

.. interface:: Data Reduction
  :widget: tabISISDiagnostics

This tab allows you to perform an integration on a raw file over a specified
time of flight range, and is equivalent to the Slice functionality found in
MODES.

Options
~~~~~~~

Input
  allows you to select a run for the function to use, either by selecting the
  *.raw* file with the Browse button or through entering the number in the box.
  Multiple files can be selected, in the same manner as described for the Energy
  Transfer tab.

Use Calibration
  Allows you to select either a calibrtion file or workspace to apply to the raw
  files.

Preview Spectrum
  Allows selection of the spectrum to be shown in the preview plot to the right
  of the Time Slice section.

Spectra Min & Spectra Max
  Allows selection of the range of detectors you are interested in, this is
  automatically set based on the instrument and analyser bank that are currently
  selected.

Peak
  The time-of-flight range that will be integrated over to give the result (the
  blue range in the plot window). A default starting value is generally provided
  from the instrument's parameter file.

Use Two Ranges
  If selected, enables subtraction of the background range.

Background
  An optional range denoting background noice that is to be removed from the raw
  data before the integration is performed. A default starting value is generally
  provided from the instrument's parameter file.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  If enabled will plot the result as a spectra plot.

Save
  If enabled the result will be saved as a NeXus file in the default save.

Transmission
------------

.. interface:: Data Reduction
  :widget: tabTransmission

Calculates the sample transmission using the raw data files of the sample and
its background or container. The incident and transmission monitors are
converted to wavelength and the transmission monitor is normalised to the
incident monitor over the common wavelength range. The sample is then divided by
the background/container to give the sample transmission as a function of
wavelength.

Options
~~~~~~~

Sample
  Allows selection of a raw file or workspace to be used as the sample.

Background
  Allows selection of a raw file or workspace to be used as the background.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  If enabled will plot the result as a spectra plot.

Save
  If enabled the result will be saved as a NeXus file in the default save.

Symmetrise
----------

.. interface:: Data Reduction
  :widget: tabSymmetrise

This tab allows you to take an asymmetric reduced file and symmetrise it about
the Y axis.

The curve is symmetrised such that the range of positive values between :math:`EMin`
and :math:`EMax` are reflected about the Y axis and repalce the negative values
in the range :math:`-EMax` to :math:`-EMin`, the curve between :math:`-EMin` and
:math:`EMin` is not modified.

Options
~~~~~~~

Input
  Allows you to select a reduced NeXus file (*_red.nxs*) or workspace (*_red*) as the
  input to the algorithm.

EMin & EMax
  Sets the energy range that is to be reflected about :math:`y=0`.

Spectrum No
  Changes the spectra shown in the preview plots.

XRange
  Changes the range of the preview plot, this can be useful for inspecting the
  curve before running the algorithm.

Preview
  This button will update the preview plot and parameters under the Preview
  section.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  If enabled will plot the result as a spectra plot.

Save
  If enabled the result will be saved as a NeXus file in the default save.

Preview
~~~~~~~

The preview section shows what a given spectra in the input will look like after
it has been symmetrised and gives an idea of how well the value of EMin fits the
curve on both sides of the peak.

Negative Y
  The value of :math:`y` at :math:`x=-EMin`.

Positive Y
  The value of :math:`y` at :math:`x=EMin`.

Delta Y
  The difference between Negative and Positive Y, typically this should be as
  close to zero as possible.

S(Q, w)
-------

.. interface:: Data Reduction
  :widget: tabSQw

Provides an interface for running the SofQW algorithms.

Options
~~~~~~~

Input
  Allows you to select a reduced NeXus file (*_red.nxs*) or workspace (*_red*) as the
  input to the algorithm.

Method
  Selects the :ref:`SofQW <algm-SofQW>` method that will be used.

Q Low, Q Width & Q High
  Q binning parameters that are passed to the SofQW algorithm.

Rebin in Energy
  If enabled the data will first be rebinned in energy before being passed to
  the SofQW algorithm.

E Low, E Width & E High
  Energy rebinning parameters.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  Allows the result to be plotted as either a spectrum plot or contour plot.

Save
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Moments
-------

.. interface:: Data Reduction
  :widget: tabMoments

This interface uses the :ref:`SofQWMoments <algm-SofQWMoments>` algorithm to
calculate the :math:`n^{th}` moment of an :math:`S(Q, \omega)` workspace created
by the SofQW tab.

Options
~~~~~~~

Input
  Allows you to select an :math:`S(Q, \omega)` file (*_sqw.nxs*) or workspace
  (*_sqw*) as the input to the algorithm.

Scale By
  Used to set an optional scale factor by which to scale the output of the
  algorithm.

EMin & EMax
  Used to set the energy range of the sample that the algorithm will use for
  processing.

Verbose
  Enables outputting additional information to the Results Log.

Plot
  If enabled will plot the result as a spectra plot.

Save
  If enabled the result will be saved as a NeXus file in the default save
  directory.

.. categories:: Interfaces Indirect

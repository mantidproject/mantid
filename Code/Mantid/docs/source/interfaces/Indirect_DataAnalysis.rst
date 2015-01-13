Indirect Data Analysis
======================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Data Analysis
  :align: right
  :width: 350

The Indirect Data Analysis interface is a collection of tools within MantidPlot
for analysing reduced data from indirect geometry spectrometers, such as IRIS and
OSIRIS.

The majority of the functions used within this interface can be used with both
reduced files (*_red.nxs*) and workspaces (*_red*) created using the Indirect Data
Reduction interface or using :math:`S(Q, \omega)` files (*_sqw.nxs*) and
workspaces (*_sqw*) created using either the Indirect Data Reduction interface or
taken from a bespoke algorithm or auto reduction.

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

Elwin
-----

.. interface:: Data Analysis
  :widget: tabElwin

Provides an interface for the :ref:`ElasticWindow <algm-ElasticWindow>`
algorithm, with the option of selecting the range to integrate over as well as
the background range. An on-screen plot is also provided.

For workspaces that have a sample log or have a sample log file available in the
Mantid data search paths that contains the sample environment information the
ELF workspace can also be normalised to the lowest temperature run in the range
of input files.

Options
~~~~~~~

Input File
  Specify a range of input files that are either reduced (*_red.nxs*) or
  :math:`S(Q, \omega)`.

Range One
  The energy range over which to integrate the values.

Use Two Ranges
  If checked a background will be calculated and subtracted from the raw data.

Range Two
  The energy range over which a background is calculated which is subtracted from
  the raw data.

Normalise to Lowest Temp
  If checked the raw files will be normalised to the run with the lowest
  temperature, to do this there must be a valid sample environment entry in the
  sample logs for each of the input files.

SE log name
  The name of the sample environment log entry in the input files sample logs
  (defaults to sample).

Plot Result
  If enabled will plot the result as a spectra plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

MSD Fit
-------

.. interface:: Data Analysis
  :widget: tabMSD

Given either a saved NeXus file or workspace generated using the ElWin tab, this
tab fits :math:`log(intensity)` vs. :math:`Q2` with a straight line for each run
specified to give the Mean Square Displacement (MSD). It then plots the MSD as
function of run number.

MSDFit searches for the log files named <runnumber>_sample.txt in your chosen
raw file directory (the name ‘sample’ is for OSIRIS). If they exist the
temperature is read and the MSD is plotted versus temperature; if they do not
exist the MSD is plotted versus run number (last 3 digits).

The fitted parameters for all runs are in _msd_Table and the <u2> in _msd. To
run the Sequential fit a workspace named <inst><first-run>_to_<last-run>_lnI is
created of :math:`ln(I)` v. :math:`Q2` for all runs. A contour or 3D plot of
this may be of interest.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be done using the Fit Single Spectrum button underneath the
preview plot.

Options
~~~~~~~

Input File
  A file that has been created using the Elwin tab with an :math:`x` axis of
  :math:`Q^2`.

StartX & EndX
  The :math:`x` range to perform fitting over.

Plot Spectrum
  The spectrum shown in the preview plot and will be fitted by running Fit
  Single Spectrum.

Spectra Range
  The spectra range over which to perform sequential fitting.

Verbose
  Enables outputting additional information to the Results Log.

Plot Result
  If enabled will plot the result as a spectra plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Fury
----

.. interface:: Data Analysis
  :widget: tabFury

Given sample and resolution inputs, carries out a fit as per the theory detailed
below.

Options
~~~~~~~

Sample
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Resolution
  Either a resolution file (_res.nxs) or workspace (_res) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

ELow, EHigh
  The rebiinning range.

SampleBinning
  The ratio at which to decrease the number of bins by through merging of
  intensities from neighbouring bins.

Verbose
  Enables outputting additional information to the Results Log.

Plot Result
  If enabled will plot the result as a spectra plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Binning
~~~~~~~

As a bin width that is a factor of the binning range is required for this
analysis the bin width is calculated automatically based on the binning range
and the number of desired bins in the output which is in turn calculated by
reducing the number of sample bins by a given factor.

The calculated binning parameters are displayed alongside the binning options:

EWidth
  The calculated bin width.

SampleBins
  Number of bins in the sample after rebinning.

ResolutionBins
  Number of bins in the resolution after rebinning, typically this should be at
  least 5 and a warning will be shown if it is less.

Theory
~~~~~~

TODO

Fury Fit
--------

.. interface:: Data Analysis
  :widget: tabFuryFit

FuryFit provides a simplified interface for controlling various fitting
functions (see the :ref:`Fit <algm-Fit>` algorithm for more info). The functions
are also available via the fit wizard.

Additionally, in the bottom-right of the interface there are options for doing a
sequential fit. This is where the program loops through each spectrum in the
input workspace, using the fitted values from the previous spectrum as input
values for fitting the next. This is done by means of the
:ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>` algorithm.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be done using the Fit Single Spectrum button underneath the
preview plot.

Options
~~~~~~~

Input
  Either a file (*_iqt.nxs*) or workspace (*_iqt*) that has been created using
  the Fury tab.

Fit Type
  The type of fitting to perform.

Constrain Intensities
  Check to ensure that the sum of the background and intensities is always equal
  to 1.

Constrain Beta over all Q
  Check to use a multi-domain fitting function with the value of beta
  constrained.

Plot Guess
  When checked a curve will be created on the plot window based on the bitting
  parameters.

StartX & EndX
  The range of :math:`x` over which the fitting will be applied (blue lines on
  preview plot).

Linear Background A0
  The constant amplitude of the background (horizontal green line on the preview
  plot).

Fitting Parameters
  Depending on the Fit Type the parameters shown for each of the fit functions
  will differ, for more information refer to the documentation pages for the fit
  function in question.

Plot Spectrum
  The spectrum shown in the preview plot and will be fitted by running Fit
  Single Spectrum.

Spectra Range
  The spectra range over which to perform sequential fitting.

Verbose
  Enables outputting additional information to the Results Log.

Plot Output
  Allows plotting spectra plots of fitting parameters, the options available
  will depend on the type of fit chosen.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Conv Fit
--------

.. interface:: Data Analysis
  :widget: tabConFit

Similarly to FuryFit, ConvFit provides a simplified interface for controlling
various fitting functions (see the :ref:`Fit <algm-Fit>` algorithm for more
info). The functions are also available via the fit wizard.

Additionally, in the bottom-right of the interface there are options for doing a
sequential fit. This is where the program loops through each spectrum in the
input workspace, using the fitted values from the previous spectrum as input
values for fitting the next. This is done by means of the
:ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>` algorithm.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be done using the Fit Single Spectrum button underneath the
preview plot.

Options
~~~~~~~

Sample
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Resolution
  Either a resolution file (_res.nxs) or workspace (_res) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Fit Type
  The type of fitting to perform.

Background
  Select the background type, see options below.

Plot Guess
  When checked a curve will be created on the plot window based on the bitting
  parameters.

StartX & EndX
  The range of :math:`x` over which the fitting will be applied (blue lines on
  preview plot).

A0 & A1 (background)
  The A0 and A1 parameters as they appear in the LinearBackground fir function,
  depending on the Fit Type selected A1 may not be shown.

Delta Function
  Enables use of a delta function.

Fitting Parameters
  Depending on the Fit Type the parameters shown for each of the fit functions
  will differ, for more information refer to the documentation pages for the fit
  function in question.

Plot Spectrum
  The spectrum shown in the preview plot and will be fitted by running Fit
  Single Spectrum.

Spectra Range
  The spectra range over which to perform sequential fitting.

Verbose
  Enables outputting additional information to the Results Log.

Plot Output
  Allows plotting spectra plots of fitting parameters, the options available
  will depend on the type of fit chosen.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Background Options
~~~~~~~~~~~~~~~~~~

Fixed Flat
  The A0 parameter is applied to all points in the data.

Fit Flat
  Similar to Fixed Flat, however the A0 parameter is treated as an initial guess
  and will be included as a parameter to the LinearBackground fit function with
  the coefficient of the linear term fixed to 0.

Fit Linear
  The A0 and A1 parameters are used as parameters to the LinearBackground fit
  function and the best possible fit will be used as the background.

Theory
~~~~~~

TODO

Calculate Corrections
---------------------

.. warning:: This interface is only available on Windows

.. interface:: Data Analysis
  :widget: tabAbsF2P

Calculates absorption corrections that could be applied to the data when given
information about the sample (and optionally can) geometry.

Options
~~~~~~~

Input
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Use Can
  If checked allows you to select a workspace for the container in the format of
  either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Sample Shape
  Sets the shape of the sample, this affects the options for the sample details,
  see below.

Beam Width
  Width of the incident beam.

Verbose
  Enables outputting additional information to the Results Log.

Plot Result
  Plots the :math:`A_{s,s}`, :math:`A_{s,sc}`, :math:`A_{c,sc}` and
  :math:`A_{c,c}` workspaces as spectra plots.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Sample Details
~~~~~~~~~~~~~~

Depending on the shape of the sample different parameters for the sample
dimension are required and are detailed below.

Flat
####

.. interface:: Data Analysis
  :widget: absp_pageFlat

Thickness
  TODO

Can Front Thickness
  TODO

Can Back Thickness
  TODO

Sample Angle
  TODO

Cylinder
########

.. interface:: Data Analysis
  :widget: absp_pageCylinder

Radius 1
  TODO

Radius 2
  TODO

Can Radius
  TODO

Step Size
  TODO

Theory
~~~~~~

TODO

Apply Corrections
-----------------

.. interface:: Data Analysis
  :widget: tabApplyAbsorptionCorrections

The Apply Corrections tab applies the corrections calculated in the Calculate
Corrections tab of the Indirect Data Analysis interface.

This tab will expect to find the ass file generated in the previous tab. If Use
Can is selected, it will also expect the assc, acsc and acc files. It will take
the run number from the sample file, and geometry from the option you select.

Once run the corrected output and can correction is shown in the preview plot,
the Spectrum spin box can be used to scroll through each spectrum.

Options
~~~~~~~

Input
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Geometry
  Sets the sample geometry (this must match the sample shape used when running
  Calculate Corrections).

Use Can
  If checked allows you to select a workspace for the container in the format of
  either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Corrections File
  The output file (_Abs.nxs) or workspace group (_Abs) generated by Calculate
  Corrections.

Verbose
  Enables outputting additional information to the Results Log.

Plot Output
  Gives the option to create either a spectra or contour plot (or both) of the
  corrected workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

.. categories:: Interfaces Indirect

Indirect Data Analysis
======================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Data Analysis
  :align: right
  :width: 350

TODO

Elwin
-----

.. interface:: Data Analysis
  :widget: tabElwin

Provides an interface for the ElasticWindow algorithm, with the option of
selecting the range to integrate over as well as the background range. An
on-screen plot is also provided.

For workspaces that have a sample log or have a sample log file available in the
Mantid data search paths that contains the sample environment information the
ELF workspace can also be normalised to the lowest temperature run in the range
of input files.

Options
~~~~~~~

Input File

Range One

Use Two Ranges

Range Two

Normalise to Lowest Temp

SE log name

Plot Result

Save Result

MSD Fit
-------

.. interface:: Data Analysis
  :widget: tabMSD

Given either a saved NeXus file or workspace generated using the ElWin tab, this
tab fits :math:`log(intensity)` vs. :math:`Q2` with a straight line for each run
specified to give the Mean Square Displacement (MSD). It then plots the MSD as
function of run.

MSDFit searches for the log files named <runnumber>_sample.txt in your chosen
raw file directory (the name ‘sample’ is for OSIRIS). If they exist the
temperature is read and the MSD is plotted versus temperature; if they do not
exist the MSD is plotted versus run number (last 3 digits).

The fitted parameters for all runs are in _msd_Table and the <u2> in _msd. To
run the Sequential fit a workspace named <inst><first-run>_to_<last-run>_lnI is
created of :math:`ln(I)` v. :math:`Q2` for all runs. A contour or 3D plot of
this may be of interest.

Options
~~~~~~~

Input File

StartX & EndX

Verbose

Plot Result

Save Result

Fury
----

.. interface:: Data Analysis
  :widget: tabFury

Given sample and resolution inputs, carries out a fit as per the theory detailed
below.

Options
~~~~~~~

Sample

Resolution

ELow, EHigh

EWidth

SampleBinning

SampleBins

ResolutionBins

Verbose

Plot Result

Save Result

Theory
~~~~~~

TODO

Fury Fit
--------

.. interface:: Data Analysis
  :widget: tabFuryFit

FuryFit provides a simplified interface for controlling various fitting
functions (see the Fit algorithm for more info). The functions are also
available via the fit wizard.

Additionally, in the bottom-right of the interface there are options for doing a
sequential fit. This is where the program loops through each spectrum in the
input workspace, using the fitted values from the previous spectrum as input
values for fitting the next. This is done by means of the PlotPeakByLogValue
algorithm.

Options
~~~~~~~

Input

Spectrum

Fit Type

Constrain Intensities

Constrain Beta over all Q

Plot Guess

StartX & EndX

Linear Background A0

Fitting Parameters

Verbose

Plot Output

Save Result

Conv Fit
--------

.. interface:: Data Analysis
  :widget: tabConFit

Similarly to FuryFit, ConvFit provides a simplified interface for controlling
various fitting functions (see the Fit algorithm for more info). The functions
are also available via the fit wizard.

Additionally, in the bottom-right of the interface there are options for doing a
sequential fit. This is where the program loops through each spectrum in the
input workspace, using the fitted values from the previous spectrum as input
values for fitting the next. This is done by means of the PlotPeakByLogValue
algorithm.

Options
~~~~~~~

Input

Resolution

Fit Type

Background

Plot Guess

StartX & EndX

A0 & A1 (background)

Delta Function

Spectra Range

Verbose

Plot Output

Save Result

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

Use Can

Sample Shape

Beam Width

Sample Details

Verbose

Plot Result

Save Result

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

Once run the correccted output and can correction is shown in the preview plot,
the Spectrum spin box can be used to scroll through each spectrum.

Options
~~~~~~~

Input

Can File

Geometry

Corrections File

Verbose

Plot Output

Save Result

.. categories:: Interfaces Indirect

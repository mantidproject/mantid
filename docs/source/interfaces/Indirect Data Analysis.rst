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

These interfaces do not support GroupWorkspace as input.

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

Bayesian
--------

There is the option to perform Bayesian data analysis on the I(Q, t) Fit ConvFit
tabs on this interface by using the :ref:`FABADA` fitting minimizer, however in
order to to use this you will need to use better starting parameters than the
defaults provided by the interface.

You may also experience issues where the starting parameters may give a reliable
fit on one spectra but not others, in this case the best option is to reduce
the number of spectra that are fitted in one operation.

In both I(Q, t) Fit and ConvFit the following options are available when fitting
using FABADA:

Output Chain
  Select to enable output of the FABADA chain when using FABADA as the fitting
  minimizer.

Chain Length
  Number of further steps carried out by fitting algorithm once parameters have
  converged (see *ChainLength* is :ref:`FABADA` documentation)

Convergence Criteria
  The minimum variation in the cost function before the parameters are
  considered to have converged (see *ConvergenceCriteria* in :ref:`FABADA`
  documentation)

Acceptance Rate
  The desired percentage acceptance of new parameters (see *JumpAcceptanceRate*
  in :ref:`FABADA` documentation)

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

Integration Range
  The energy range over which to integrate the values.

Background Subtraction
  If checked a background will be calculated and subtracted from the raw data.

Background Range
  The energy range over which a background is calculated which is subtracted from
  the raw data.

Normalise to Lowest Temp
  If checked the raw files will be normalised to the run with the lowest
  temperature, to do this there must be a valid sample environment entry in the
  sample logs for each of the input files.

SE log name
  The name of the sample environment log entry in the input files sample logs
  (defaults to sample).

SE log value
  The value to be taken from the "SE log name" data series (defaults to the
  specified value in the intrument parameters file, and in the absence of such
  specification, defaults to "last value")

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
tab fits :math:`log(intensity)` vs. :math:`Q^{2}` with a straight line for each
run specified to give the Mean Square Displacement (MSD). It then plots the MSD
as function of run number.

MSDFit searches for the log files named <runnumber>_sample.txt in your chosen
raw file directory (the name ‘sample’ is for OSIRIS). If they exist the
temperature is read and the MSD is plotted versus temperature; if they do not
exist the MSD is plotted versus run number (last 3 digits).

The fitted parameters for all runs are in _msd_Table and the <u2> in _msd. To
run the Sequential fit a workspace named <inst><first-run>_to_<last-run>_lnI is
created of :math:`ln(I)` v. :math:`Q^{2}` for all runs. A contour or 3D plot of
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

Plot Result
  If enabled will plot the result as a spectra plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

I(Q, t)
-------

.. interface:: Data Analysis
  :widget: tabIqt

Given sample and resolution inputs, carries out a fit as per the theory detailed
in the :ref:`TransformToIqt <algm-TransformToIqt>` algorithm.

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

I(Q, t) Fit
-----------

.. interface:: Data Analysis
  :widget: tabIqtFit

I(Q, t) Fit provides a simplified interface for controlling various fitting
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

Max Iterations
  The maximum number of iterations that can be carried out by the fitting
  algorithm (automatically increased when FABADA is enabled).

StartX & EndX
  The range of :math:`x` over which the fitting will be applied (blue lines on
  preview plot).

Use FABADA
  Select to enable use of the :ref:`FABADA` minimizer when performing the fit.

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

Plot Output
  Allows plotting spectra plots of fitting parameters, the options available
  will depend on the type of fit chosen.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Conv Fit
--------

.. interface:: Data Analysis
  :widget: tabConvFit

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

Fitting Model
~~~~~~~~~~~~~

The model used to perform fitting is described in the following tree, note that
everything under the Model section is optional and determined by the *Fit Type*
and *Use Delta Function* options in the interface.

- :ref:`CompositeFunction <func-CompositeFunction>`

  - :ref:`LinearBackground <func-LinearBackground>`

  - :ref:`Convolution <func-Convolution>`

    - Resolution

    - Model (:ref:`CompositeFunction <func-CompositeFunction>`)

      - DeltaFunction

      - :ref:`ProductFunction <func-ProductFunction>` (One Lorentzian)

        - :ref:`Lorentzian <func-Lorentzian>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (Two Lorentzians)

        - :ref:`Lorentzian <func-Lorentzian>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (InelasticDiffSphere)

        - :ref:`Inelastic Diff Sphere <func-DiffSphere>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (InelasticDiffRotDiscreteCircle)

        - :ref:`Inelastic Diff Rot Discrete Circle <func-DiffRotDiscreteCircle>` 

        - Temperature Correction
		
      - :ref:`ProductFunction <func-ProductFunction>` (ElasticDiffSphere)

        - :ref:`Elastic Diff Sphere <func-DiffSphere>`

        - Temperature Correction
		
      - :ref:`ProductFunction <func-ProductFunction>` (ElasticDiffRotDiscreteCircle)

        - :ref:`Elastic Diff Rot Discrete Circle <func-DiffRotDiscreteCircle>`

        - Temperature Correction
		
      - :ref:`ProductFunction <func-ProductFunction>` (StretchedExpFT)

        - :ref:`StretchedExpFT <func-StretchedExpFT>`

        - Temperature Correction

The Temperature Correction is a :ref:`UserFunction <func-UserFunction>` with the
formula :math:`((x * 11.606) / T) / (1 - exp(-((x * 11.606) / T)))` where
:math:`T` is the temperature in Kelvin.

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

Max Iterations
  The maximum number of iterations that can be carried out by the fitting
  algorithm (automatically increased when FABADA is enabled).

StartX & EndX
  The range of :math:`x` over which the fitting will be applied (blue lines on
  preview plot).

Use FABADA
  Select to enable use of the :ref:`FABADA` minimizer when performing the fit.

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

The measured data :math:`I(Q, \omega)` is proportional to the convolution of the
scattering law :math:`S(Q, \omega)` with the resolution function :math:`R(Q,
\omega)` of the spectrometer via :math:`I(Q, \omega) = S(Q, \omega) ⊗  R(Q,
\omega)`. The traditional method of analysis has been to fit the measured
:math:`I(Q, \omega)` with an appropriate set of functions related to the form of
:math:`S(Q, \omega)` predicted by theory.

* In quasielastic scattering the simplest form is when both the :math:`S(Q,
  \omega)` and the :math:`R(Q, \omega)` have the form of a Lorentzian - a
  situation which is almost correct for reactor based backscattering
  spectrometers such as IN10 & IN16 at ILL. The convolution of two Lorentzians
  is itself a Lorentzian so that the spectrum of the measured and resolution
  data can both just be fitted with Lorentzians. The broadening of the sample
  spectrum is then just the  difference of the two widths.
* The next easiest case is when both :math:`S(Q, \omega)` and :math:`R(Q,
  \omega)` have a simple functional form and the convolution is also a function
  containing the parameters of the :math:`S(Q, \omega)` and R(Q,  \omega) functions.
  The convoluted function may then be fitted to the data to provide the
  parameters. An example would be the case where the :math:`S(Q, \omega)` is a
  Lorentzian and the :math:`R(Q, \omega)` is a Gaussian.
* For diffraction, the shape of the peak in time is a convolution of a Gaussian
  with a decaying exponential and this function can be used to fit the Bragg
  peaks.
* The final case is where :math:`R(Q, \omega)` does not have a simple function
  form so that the measured data has to be convoluted numerically with the
  :math:`S(Q, \omega)` function to provide an estimate of the sample scattering.
  The result is least-squares fitted to the measured data to provide values for
  the parameters in the :math:`S(Q, \omega)` function.

This latter form of peak fitting is provided by SWIFT. It employs a
least-squares algorithm which requires the derivatives of the fitting function
with respect to its parameters in order to be faster and more efficient than
those algorithms which calculate the derivatives numerically. To do this the
assumption is made that the derivative of a convolution is equal to the
convolution of the derivative-as the derivative and the convolution are
performed over different variables (function parameters and energy transfer
respectively) this should be correct. A flat background is subtracted from the
resolution data before the convolution is performed.

Four types of sample function are available for :math:`S(Q, \omega)`:

Quasielastic
  This is the most common case and applies to both translational (diffusion) and
  rotational modes, both of which have the form of a Lorentzian. The fitted
  function is a set of Lorentzians centred at the origin in energy transfer.

Elastic
  Comprising a central elastic peak together with a set of quasi-elastic
  Lorentzians also centred at the origin. The elastic peak is taken to be the
  un-broadened resolution function.

Shift
  A central Lorentzian with pairs of energy shifted Lorentzians. This was
  originally used for crystal field splitting data but more recently has been
  applied to quantum tunnelling peaks. The fitting function assumes that the
  peaks are symmetric about the origin in energy transfer both in position and
  width. The widths of the central and side peaks may be different.

Polymer
  A single quasi-elastic peak with 3 different forms of shape. The theory behind
  this is described elsewhere [1,2]. Briefly, polymer theory predicts 3 forms
  of the :math:`I(Q,t)` in the form of :math:`exp(-at2/b)` where :math:`b` can
  be 2, 3 or 4. The Full Width Half-Maximum (FWHM) then has a Q-dependence
  (power law) of the form :math:`Qb`. The :math:`I(Q,t)` has been numerically
  Fourier transformed into :math:`I(Q, \omega)` and the :math:`I(Q, \omega)`
  have been fitted with functions of the form of a modified Lorentzian. These
  latter functions are used in the energy fitting procedures.

References:

1. J S Higgins, R E Ghosh, W S Howells & G Allen, `JCS Faraday II 73 40 (1977) <http://dx.doi.org/10.1039/F29777300040>`_
2. J S Higgins, G Allen, R E Ghosh, W S Howells & B Farnoux, `Chem Phys Lett 49 197 (1977) <http://dx.doi.org/10.1016/0009-2614(77)80569-1>`_

JumpFit
-------

.. interface:: Data Analysis
  :widget: tabJumpFit

One of the models used to interpret diffusion is that of jump diffusion in which
it is assumed that an atom remains at a given site for a time :math:`\tau`; and
then moves rapidly, that is, in a time negligible compared to :math:`\tau`;
hence ‘jump’.

Options
~~~~~~~

Sample
  A sample workspace created with either ConvFit or Quasi.

Fit Funcion
  Selects the model to be used for fitting.

Width
  Spectrum in the sample workspace to fit.

QMin & QMax
  The Q range to perform fitting within.

Fitting Parameters
  Provides the option to change the defautl fitting parameters passed to the
  chosen function.

Plot Result
  Plots the result workspaces.

Save Result
  Saves the result in the default save directory.

.. categories:: Interfaces Indirect

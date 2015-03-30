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

The measured spectrum :math:`I(Q, \omega)` is proportional to the four
dimensional convolution of the scattering law :math:`S(Q, \omega)` with the
resolution function :math:`R(Q, \omega)` of the spectrometer via :math:`I(Q,
\omega) = S(Q, \omega) ⊗ R(Q, \omega)`, so :math:`S(Q, \omega)` can be obtained,
in principle, by a deconvolution in :math:`Q` and :math:`\omega`. The method
employed here is based on the Fourier Transform (FT) technique [6,7]. On Fourier
transforming the equation becomes :math:`I(Q, t) = S(Q, t) x R(Q, t)` where the
convolution in :math:`\omega`-space is replaced by a simple multiplication in
:math:`t`-space. The intermediate scattering law :math:`I(Q, t)` is then
obtained by simple division and the scattering law :math:`S(Q, \omega)` itself
can be obtained by back transformation. The latter however is full of pitfalls
for the unwary. The advantage of this technique over that of a fitting procedure
such as SWIFT is that a functional form for :math:`I(Q, t)` does not have to be
assumed. On IRIS the resolution function is close to a Lorentzian and the
scattering law is often in the form of one or more Lorentzians. The FT of a
Lorentzian is a decaying exponential, :math:`exp(-\alpha t)` , so that plots of
:math:`ln(I(Q, t))` against t would be straight lines thus making interpretation
easier.

In general, the origin in energy for the sample run and the resolution run need
not necessarily be the same or indeed be exactly zero in the conversion of the
RAW data from time-of-flight to energy transfer. This will depend, for example,
on the sample and vanadium shapes and positions and whether the analyser
temperature has changed between the runs. The procedure takes this into account
automatically, without using an arbitrary fitting procedure, in the following
way. From the general properties of the FT, the transform of an offset
Lorentzian has the form :math:`(cos(\omega_{0}t) + isin(\omega_{0}t))exp(-\Gamma
t)` , thus taking the modulus produces the exponential :math:`exp(-\Gamma t)`
which is the required function. If this is carried out for both sample and
resolution, the difference in the energy origin is automatically removed. The
results of this procedure should however be treated with some caution when
applied to more complicated spectra in which it is possible for :math:`I(Q, t)`
to become negative, for example, when inelastic side peaks are comparable in
height to the elastic peak.

The interpretation of the data must also take into account the propagation of
statistical errors (counting statistics) in the measured data as discussed by
Wild et al [1]. If the count in channel :math:`k` is :math:`X_{k}` , then
:math:`X_{k}=<X_{k}>+\Delta X_{k}` where :math:`<X_{k}>` is the mean value and
:math:`\Delta X_{k}` the error. The standard deviation for channel :math:`k` is
:math:`\sigma k` :math:`2=<\Delta X_{k}>2` which is assumed to be given by
:math:`\sigma k=<X_{k}>`. The FT of :math:`X_{k}` is defined by
:math:`X_{j}=<X_{j}>+\Delta X_{j}` and the real and imaginary parts denoted by
:math:`X_{j} I` and :math:`X_{j} I` respectively. The standard deviations on
:math:`X_{j}` are then given by :math:`\sigma 2(X_{j} R)=1/2 X0 R + 1/2 X2j R`
and :math:`\sigma 2(X_{j} I)=1/2 X0 I - 1/2 X2j I`.

Note that :math:`\sigma 2(X_{0} R) = X_{0} R` and from the properties of FT
:math:`X_{0} R = X_{k}`.  Thus the standard deviation of the first coefficient
of the FT is the square root of the integrated intensity of the spectrum. In
practice, apart from the first few coefficients, the error is nearly constant
and close to :math:`X_{0} R`.  A further point to note is that the errors make
the imaginary part of :math:`I(Q, t)` non-zero and that, although these will be
distributed about zero, on taking the modulus of :math:`I(Q, t)`, they become
positive at all times and are distributed about a non-zero positive value. When
:math:`I(Q, t)` is plotted on a log-scale the size of the error bars increases
with time (coefficient) and for the resolution will reach a point where the
error on a coefficient is comparable to its value. This region must therefore be
treated with caution. For a true deconvolution by back transforming, the data
would be truncated to remove this poor region before back transforming. If the
truncation is severe the back transform may contain added ripples, so an
automatic back transform is not provided.

References:

1. U P Wild, R Holzwarth & H P Good, Rev Sci Instr 48 1621 (1977)

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

1. J S Higgins, R E Ghosh, W S Howells & G Allen, JCS Faraday II 73 40 (1977)
2. J S Higgins, G Allen, R E Ghosh, W S Howells & B Farnoux, Chem Phys Lett 49 197 (1977)

Calculate Corrections
---------------------

.. interface:: Data Analysis
  :widget: tabCalcCorr

Calculates absorption corrections in the Paalman & Pings absorption factors that
could be applied to the data when given information about the sample (and
optionally can) geometry.

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
  Sets the shape of the sample, this affects the options for the shape details
  (see below).

Sample/Can Number Density
  Density of the sample or container.

Sample/Can Chemical Formula
  Chemical formula of the sample or can material. This must be provided in the
  format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm.

Plot Output
  Plots the :math:`A_{s,s}`, :math:`A_{s,sc}`, :math:`A_{c,sc}` and
  :math:`A_{c,c}` workspaces as spectra plots.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Shape Details
~~~~~~~~~~~~~

Depending on the shape of the sample different parameters for the sample
dimension are required and are detailed below.

Flat Plate
##########

.. interface:: Data Analysis
  :widget: pgFlatPlate

The calculation for a flat plate geometry is performed by the
:ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>`
algorithm.

Sample Thickness
  Thickness of sample (cm).

Sample Angle
  Sample angle (degrees).

Can Front Thickness
  Thickness of front container (cm).

Can Back Thickness
  Thickness of back container (cm).

Cylinder
########

.. warning:: This mode is only available on Windows

.. interface:: Data Analysis
  :widget: pgCylinder

The calculation for a cylindrical geometry is performed by the
:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
algorithm, this algorithm is currently only available on Windows as it uses
FORTRAN code dependant of F2Py.

Sample Inner Radius
  Radius of the inner wall of the sample (cm).

Sample Outer Radius
  Radius of the outer wall of the sample (cm).

Container Outer Radius
  Radius of outer wall of the container (cm).

Beam Height
  Height of incident beam (cm).

Beam Width
  Width of incident beam (cm).

Step Size
  Step size used in calculation.

Background
~~~~~~~~~~

The main correction to be applied to neutron scattering data is that for
absorption both in the sample and its container, when present. For flat plate
geometry, the corrections can be analytical and have been discussed for example
by Carlile [1]. The situation for cylindrical geometry is more complex and
requires numerical integration. These techniques are well known and used in
liquid and amorphous diffraction, and are described in the ATLAS manual [2].

The absorption corrections use the formulism of Paalman and Pings [3] and
involve the attenuation factors :math:`A_{i,j}` where :math:`i` refers to
scattering and :math:`j` attenuation. For example, :math:`A_{s,sc}` is the
attenuation factor for scattering in the sample and attenuation in the sample
plus container. If the scattering cross sections for sample and container are
:math:`\Sigma_{s}` and :math:`\Sigma_{c}` respectively, then the measured
scattering from the empty container is :math:`I_{c} = \Sigma_{c}A_{c,c}` and
that from the sample plus container is :math:`I_{sc} = \Sigma_{s}A_{s,sc} +
\Sigma_{c}A_{c,sc}`, thus :math:`\Sigma_{s} = (I_{sc} - I_{c}A_{c,sc}/A_{c,c}) /
A_{s,sc}`.

References:

1. C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)
2. A K Soper, W S Howells & A C Hannon, RAL Report RAL-89-046 (1989)
3. H H Paalman & C J Pings, J Appl Phys 33 2635 (1962)


Apply Corrections
-----------------

.. interface:: Data Analysis
  :widget: tabApplyCorr

The Apply Corrections tab applies the corrections calculated in the Calculate
Corrections tab of the Indirect Data Analysis interface.

This uses the :ref:`ApplyPaalmanPingsCorrection
<algm-ApplyPaalmanPingsCorrection>` algorithm to apply absorption corrections in
the form of the Paalman & Pings correction factors. When *Use Can* is disabled
only the :math:`A_{s,s}` factor must be provided, when using a container the
additional factors must be provided: :math:`A_{c,sc}`, :math:`A_{s,sc}` and
:math:`A_{c,c}`.

Once run the corrected output and can correction is shown in the preview plot,
the Spectrum spin box can be used to scroll through each spectrum. Note that
when this plot shows the result of a calculation the X axis is always in
wavelength, however when data is initially selected the X axis unit matches that
of the sample workspace.

The input and container workspaces will be converted to wavelength (using
:ref:`ConvertUnits <algm-ConvertUnits>`) if they do not already have wavelength
as their X unit.

The binning of the sample, container and corrections factor workspace must all
match, if the sample and container do not match you will be given the option to
rebin (using :ref:`RebinToWorkspace <algm-RebinToWorkspace>`) the sample to
match the container, if the correction factors do not match you will be given
the option to interpolate (:ref:`SplineInterpolation
<algm-SplineInterpolation>`) the correction factor to match the sample.

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

Scale Can by factor
  Allows the container intensity to be scaled by a given scale factor before
  being used in the corrections calculation.

Use Corrections
  The Paalman & Pings correction factors to use in the calculation, note that
  the file or workspace name must end in either *_flt_abs* or *_cyl_abs* for the
  flat plate and cylinder geometries respectively.

Plot Output
  Gives the option to create either a spectra or contour plot (or both) of the
  corrected workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

.. categories:: Interfaces Indirect

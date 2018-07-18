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

Four of the available tabs are QENS fitting interfaces and share common features and layout. These common factors are documented in the :ref:`qens-fitting-features` section of this document.

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

Plot Current Preview
  Takes the data currently plotted in the preview plot and puts it in a separate external plot
  

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

Given either a saved NeXus file or workspace generated using the Elwin tab, this
tab fits :math:`log(intensity)` vs. :math:`Q` with a straight line for each
run specified to give the Mean Square Displacement (MSD). It then plots the MSD
as function of run number. This is done by means of the
:ref:`QENSFitSequential <algm-QENSFitSequential>` algorithm.

MSDFit searches for the log files named <runnumber>_sample.txt in your chosen
raw file directory (the name ‘sample’ is for OSIRIS). If they exist the
temperature is read and the MSD is plotted versus temperature; if they do not
exist the MSD is plotted versus run number (last 3 digits).

The fitted parameters for all runs are in _msd_Table and the <u2> in _msd. To
run the Sequential fit a workspace named <inst><first-run>_to_<last-run>_lnI is
created of :math:`ln(I)` v. :math:`Q` for all runs. A contour or 3D plot of
this may be of interest.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be done using the Fit Single Spectrum button underneath the
preview plot.

Options
~~~~~~~

Note: common options are detailled in the :ref:`qens-fitting-features` section.

Sample
  A file that has been created using the Elwin tab with an :math:`x` axis of
  :math:`Q`. Alternatively, a workspace may be provided.




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
  The ratio at which to decrease the number of bins by merging of
  intensities from neighbouring bins.

Plot Result
  If enabled will plot the result as a spectra plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Monte Carlo Error Calculation - Number Of Iterations
  The number of iterations to perform in the Monte Carlo routine for error
  calculation in I(Q,t)

A note on Binning
~~~~~~~~~~~~~~~~~
  
The bin width is determined by the binning range and the sample binning factor. The number of bins is automatically calculated based on the **SampleBinning** specified. The width is the determined by the width of the range divided by the number of bins.

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
:ref:`IqtFitSequential <algm-IqtFitSequential>` algorithm.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be done using the Fit Single Spectrum button underneath the
preview plot.

Options
~~~~~~~

Note: common options are detailled in the :ref:`qens-fitting-features` section.

Input
  Either a file (*_iqt.nxs*) or workspace (*_iqt*) that has been created using
  the Iqt tab.

Constrain Intensities
  Check to ensure that the sum of the background and intensities is always equal
  to 1.

Make Beta Global
  Check to use a multi-domain fitting function with the value of beta
  constrained - the :ref:`IqtFitSimultaneous <algm-IqtFitSimultaneous>` will be
  used to perform this fit.

Extract Members
  If checked, each individual member of the fit (e.g. exponential functions), will
  be extracted.

Background Options
~~~~~~~~~~~~~~~~~~

Linear Background
  Adds a linear background to the composite fit function.

Conv Fit
--------

.. interface:: Data Analysis
  :widget: tabConvFit

ConvFit provides a simplified interface for controlling
various fitting functions (see the :ref:`Fit <algm-Fit>` algorithm for more
info). The functions are also available via the fit wizard.

Additionally, in the bottom-right of the interface there are options for doing a
sequential fit. This is where the program loops through each spectrum in the
input workspace, using the fitted values from the previous spectrum as input
values for fitting the next. This is done by means of the
:ref:`ConvolutionFitSequential <algm-ConvolutionFitSequential>` algorithm.

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

Note: common options are detailled in the :ref:`qens-fitting-features` section.

Sample
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Resolution
  Either a resolution file (_res.nxs) or workspace (_res) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Use Delta Function
  Found under 'Custom Function Groups'. Enables use of a delta function.

Extract Members
  If checked, each individual member of the fit (e.g. exponential functions), will
  be extracted into a <result_name>_Members group workspace.

Use Temperature Correction
  Adds the custom user function for temperature correction to the fit function.

Background Options
~~~~~~~~~~~~~~~~~~

Flat Background
  Adds a flat background to the composite fit function.

Linear Background
  Adds a linear background to the composite fit function.

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

F(Q) Fit
--------

.. interface:: Data Analysis
  :widget: tabJumpFit

One of the models used to interpret diffusion is that of jump diffusion in which
it is assumed that an atom remains at a given site for a time :math:`\tau`; and
then moves rapidly, that is, in a time negligible compared to :math:`\tau`.

This interface can be used for a jump diffusion fit as well as fitting across
EISF. This is done by means of the
:ref:`QENSFitSequential <algm-QENSFitSequential>` algorithm.


Options
~~~~~~~

Note: common options are detailled in the :ref:`qens-fitting-features` section.

Sample
  A sample workspace created with either ConvFit or Quasi.

Fit Parameter
  Either 'Width' or 'EISF' can be selected here, determining whether a width or
  EISF parameter will be fit across.

Width/EISF
  Next to the 'Fit Parameter' menu, will be either a 'Width' or 'EISF' menu, depending on
  which was selected. This menu can be used to select the specific width/EISF parameter to be fit.
  
 .. _qens-fitting-features:
  
QENS Fitting Interfaces Features
--------------------------------

There are four QENS fitting interfaces:  

* MSD Fit
* I(Q,t) Fit, 
* Conv Fit 
* F(Q)

These fitting interfaces share common features, with a few unique options in each.

Single & Multiple Input
~~~~~~~~~~~~~~~~~~~~~~~

Each interface provides the option to choose between selecting one or multiple data files to be fit.
The selected mode can be changed by clicking either the 'Single Input' tab or 'Multiple Input' tab at the the top
of the interface to switch between selecting one or multiple data files respectively.
Data may either be provided as a file, or selected from workspaces which have already been loaded.

When selecting 'Multiple Input', a table along with two buttons 'Add Workspace' and 'Remove' will be displayed.
Clicking 'Add Workspace' will allow you to add a new data-set to be fit (this will bring up a menu allowing you
to select a file/workspace and the spectra to load). Once data has been loaded, it will be displayed in the table.
Highlighting data in the table and selecting 'Remove' will allow you to remove data from the fit. Above the preview
plots will be a drop-down menu with which you can select the active data-set, which will be shown in the plots.

Custom Function Groups
~~~~~~~~~~~~~~~~~~~~~~

Under 'Custom Function Groups', you will find utility options for quick selection of common fit functions, specific
to each fitting interface.

The 'Fit Type' drop-down menu will be available here in each of the QENS fitting interfaces -- which is useful for
selecting common fit functions but not mandatory.

Fitting Range
~~~~~~~~~~~~~

Under 'Fitting Range', you may select the start and end :math:`x`-values ('StartX' and 'EndX') to be used in the fit.

Functions
~~~~~~~~~

Under 'Functions', you can view the selected model and associated parameters as well as make modifications.
Right-clicking on 'Functions' and selecting 'Add Function' will allow you to add any function from Mantid's library
of fitting functions. It is also possible to right-click on a composite function and select 'Add Function' to add a
function to the composite.

Parameters may be tied by right-clicking on a parameter and selecting either 'Tie > To Function' when creating a tie
to a parameter of the same name in a different function or by selecting 'Tie > Custom Tie' to tie to parameters of
different names and for providing mathematical expressions. Parameters can be constrained by right-clicking and
using the available options under 'Constrain'.

Upon performing a fit, the parameter values will be updated here to display the result of the fit for the selected
spectrum.

Settings
~~~~~~~~

Minimizer
  The minimizer which will be used in the fit (defaults to Levenberg-Marquadt).

Ignore invalid data
  Whether to ignore invalid (infinity/NaN) values when performing the fit.

Cost function
  The cost function to be used in the fit (defaults to Least Squares).

Max Iterations
  The maximum number of iterations used to perform the fit of each spectrum.

Preview Plots
~~~~~~~~~~~~~

Two preview plots are included in each of the fitting interfaces. The top preview plot displays the sample, guess
and fit curves. The bottom preview plot displays the difference curve.

The preview plots will display the curves for the selected spectrum ('Plot Spectrum') of the selected data-set
(when in multiple input mode, a drop-down menu will be available above the plots to select the active data-set).

The 'Plot Spectrum' option can be used to select the active/displayed spectrum.

A button labelled 'Fit Single Spectrum' is found under the preview plots and can be used to perform a fit of the
selected specturm.

'Plot Current Preview' can be used to plot the sample, fit and difference curves of the selected spectrum in
a separate plotting window.

The 'Plot Guess' check-box can be used to enable/disable the guess curve in the top preview plot.


Spectrum Selection
~~~~~~~~~~~~~~~~~~

Below the preview plots, the spectra to be fit can be selected. The 'Fit Spectra' drop-down menu allows for
selecting either 'Range' or 'String'. If 'Range' is selected, you are able to select a range of spectra to fit by
providing the upper and lower bounds. If 'String' is selected you can provide the spectra to fit in a text form.
When selecting spectra using text, you can use '-' to identify a range and ',' to separate each spectrum/range.

:math:`X`-Ranges may be excluded from the fit by selecting a spectrum next to the 'Mask Bins of Spectrum' label and
then providing a comma-separated list of pairs, where each pair designates a range to exclude from the fit.

Output
~~~~~~

The results of the fit may be plot and saved under the 'Output' section of the fitting interfaces.

Next to the 'Plot Output' label, you can select a parameter to plot and then click 'Plot' to plot it across the
fit spectra (if multiple data-sets have been used, a separate plot will be produced for each data-set).

Clicking the 'Save Result' button will save the result of the fit to your default save location.

  
Bayesian (FABADA minimizer)
---------------------------

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


.. categories:: Interfaces Indirect

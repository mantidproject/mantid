.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm fits a set of specified peaks in a set of specified spectra in a MatrixWorkspace,
returnig a list of the successfully fitted peaks along with
optional output for fitting information.

The list of candidate peaks found is passed to a fitting routine and
those that are successfully fitted are kept and returned in the output
workspace (and logged at information level). The output
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ contains the following columns,
which reflect the fact that the peak has been fitted to a Gaussian atop
a linear background: spectrum, centre, width, height,
backgroundintercept & backgroundslope.

Assumption
##########

It is assumed that the values of peaks' profile parameters, such as peak width, 
are somehow correlated within a single spectrum.
Therefore, the fitted profile parameters values of a peak should be good starting values
to fit the peaks adjacent to this peak.

Required pre-knowledge
######################

The following information are required.

* Assumed position of each peak;
* Peak profile;
* Background type (flat or linear);
* Starting values of peak parameters;

For better results
==================

* Peak fit window: this is an option

  a. specified by user
  b. figured out by FindPeakBackground, first moments and second moments

* Peak position tolerance: there could be three cases for how the peak position tolerance is specified.

  a. specified by user
  b. defined by peak windows
  c. half distance to the neighboring peak (if not (a) and not (b))
  d. whole range of X-axis (if there is one and only one peak in a spectrum)



Descrption of algorithm
#######################

For each spectrum, it is assumed that there are N peaks to fit.
The fitting starts from either the peak at the smallest X value or the largest X values depending on the 
user's input.
The first peak will be fit with the starting value provided by the user.
except background and peak center, which will be determiend by *observation*.

Background
##########

Linear background is used for fitting peaks.  The background parameters
will be estimated via *observation*.

Estimation of values of peak profiles
#####################################

Peak intensity and peak center are estimated by observing the maximum value within peak fit window.

Subalgorithms used
##################

FitPeaks uses the :ref:`algm-Fit` algorithm to fit each single peak.
FitPeaks uses the :ref:`algm-FindPeakBackground` algorithm to estimate the background of each peak.


Inputs
======

The inputs tends to be general enough for various use cases.


Limitations
###########

 * For partial spectra peak fitting, the spectra must be consecutive.


Peak positions
##############

One of only one of the following will be taken.

 * A MatrixWorkspace
   * Number of spectra shall be the same as the number of spectra of the workspace containing peaks to fit for.  Or the number of spectra is the same as the number of spectra of the input workspace.
   * X value is the index of the peak.
   * Y value is the position of the peaks to fit.
   * Peak centers are stored in **m_peakCenterWorkspace**.
   - Spectra can be a subset of all spectra because FitPeaks can work on partial spectra.

 * An array of double as the positions of the peaks to fit.

   * Peak centers are stored in **m_peakCenters**


 **Peaks' positions must be given in ascending order**

 Parameter **FitFromRight** is used as the flag to fit from right most peak or left most peak.



Fit Window
##########

There are two input parameters that are associated with fitting window.

 * FitWindowBoundaryList
 * FitPeakWindowWorkspace


If FitWindows is defined, then a peak's range to fit (i.e., x-min and
x-max) is confined by this window.

If FitWindows is defined, starting peak centres are NOT user's input,
but found by highest value within peak window. (Is this correct???)


Further down the road, here are the fitting setup that can be affected.

  * Peak positions are uniform among all spectra;

    - Peak window information will be retrieved from **m_peakWindowVector**

  * Peak positions are different among spectra.

    - Peak windown information will be retrieved from **m_peakWindowWorkspace**


Tolerance on Fitting Peaks Positions
####################################

Tolerance will be always checked!

 * Uniform tolerance
 
 * Non-uniform tolerance

 * Case 2, 3 and 4



Algorithm Configurations
########################

 * Peak profile starting value will be given as 

    - an array **PeakParameterValues** such that the starting values are uniform among all spectra.
    - a table (workspace) **PeakParameterValueTable** such that the starting values are not necessary same among all spectra.


Calculation of starting value of peak profile and background parameters
-----------------------------------------------------------------------

FitPeaks supports estimating peak parameter names as starting values.


Workflow
########

1. Call `algm-FindPeakBackground` to estimate the background of peak with a numerical approach.

2. If `algm-FindPeakBackground` fails, *estimate-peak-background* will be used for simple approximation.

3. Estimate the peak parameter, *estimate-peak-parameter*, by using the estimated peak background obtained in either step 1 or step 2.

4. Estimate the peak range, which is used to constrain the peak position in fitting, by using the left *FWHM* and right *FWHM* from step 3.

Estimate background
###################

*Estimate-peak-background* takes *peak fit window* for pre-knowledge, and calculate *a* and *b* in the linear background function.

The algorithm is
1. Find the left and right *N* points respectively, average both *x* and *y* value
2. Use :math:`(\bar{x}_1, \bar{y}_1)` and :math:`(\bar{x}_2, \bar{y}_2)` to calculate *a* and *b*
   in :math:`y = a\cdot x + b`

Estimate peak parameters
########################

*Estimate-peak-parameters* requires background parameters being estimated.

Here is the approach to estimate the peak parameters

1. Remove background;

2. Find maximum Y value as the *observed* peak center and peak height :math:`H_{obs}`;

3. Check peak height with user-specified minimum height and peak center that must be at least more than 3 data points away from the boundary of fit window.

4. Find the left and right *FWHM* by searching :math:`x_i` and :math:`x_{i+1}` such that :math:`H_{obs}` is between :math:`y_i` and :math:`y_{i+1}`.


Estimate peak range
===================

*Estimate-peak-range* requires inputs including expected peak center, fit window and estimated right and left FWHM.
It will output the left and right boundary of the peak such that the background can be fit by excluding the peak.

1. Peak range is defined as :math:`x_0 \pm 6 \cdot w`, where *w* is half of FWHM for either left or right half of peak.

2. Check the number of background points out of peak range at the left and right side respectively.
   It is required to have at least 3 background points at either side, i.e., :math:`min(3, \frac{i_{x0} - i_{min}}{6})` for left side.



Fit peak with high background
#############################

Step 1
======

Reduce the background by finding a linear function :math:`B_i = a\cdot x_i + b`,
such that :math:`\sum_i (Y_i - B_i)` is minimum while any :math:`Y_i - B_i` is non-negative.

This approach is good for any background close to linear within the fit window.

Step 2
======

With the background reduced in step 1, it will be more reliable to estimate the peak's FWHM via *observation*.

Step 3
======

Get the peak range (by *estimate-peak-range*) and fit the background with *FitMultiDomain* to fit background.

Step 4
======

Remove the background and fit peak!


Outputs
-------

Algorithm *FitPeaks* is designed for various purposes including but not limited to vanadium peak striping and fitting diamond peaks to calibrate detectors' positions.
On the other hand, due to the complexity in peak fitting, users prefer to check the fitting results. 
Therefore, *FitPeaks* supports various fexible and informative outputs.

OutputWorkspace
###############

It is a MatrixWorkspace containing the peak positions expected and fitted.

- The output workspace has *N* spectra corresponding to the spectra that are specified by user via **MinimumWorkspaceIndex** and **MaximumWorkspaceIndex**.
- If there are *m* peaks that are required to fit for, then each spectrum in the output workspace has *m* data points.
- In each spectrum, *x(i)* is the expected position of *i-th* peak; *y(i)* is the fitted position of *i-th* peak; and *e(i)* is the cost from fitting.
- There are several cases that the fitting could fail.  A negative peak position *y(i)* combined with *e(i)* equal to *DBL_MAX* denote such failure.
- Cause of fitting failure is denoted by different negative value of *y(i)*
  * -1: empty spectrum
  * -2: spectrum with too few counts
  * -3: peak is low
  * -4: blabla



It is a TableWorkspace containing peak parameters.
According to user's specication, it will contain one parameter, i.e., peak position, or all parameters.

The order of the peaks will be exactly the sequence of peaks as the order of the given positions of peaks.


FittingCostWorkspace
####################

It is a MatrixWorkspace recording the cost of each peak that is fitted.
It is in the exactly same order as the given positions of peaks to fit.
Its X values store the fitted peak positions and Y values are for :math:`\chi^2`.

If a peak's fitting is bad, then the peak position will be its proposed peak position,
while its :math:`\chi^2` shall be some special value.


FittedPeaksWorkspace
####################

It is an optional output MatrixWorkspace.

For each spectrum, in each fit window, the Y values will be replaced by the calcualted peak and background value.
If fitting is bad, then only background is calculated.



Usage
-----

**Example - Find a single peak:**

.. testcode:: ExFindPeakSingle

   ws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
      A0=0.3;name=Gaussian, PeakCentre=5, Height=10, Sigma=0.7", NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

   table = FindPeaks(InputWorkspace='ws', FWHM='20')

   row = table.row(0)

   #print row
   print "Peak 1 {Centre: %.3f, width: %.3f, height: %.3f }" % ( row["centre"],  row["width"], row["height"])


Output:

.. testoutput:: ExFindPeakSingle

   Peak 1 {Centre: 5.050, width: 1.648, height: 10.000 }


**Example - Find multiple peaks with uniform positions among all spectra:**


**Example - Find multiple peaks with various positions among all sepctra:**


**Example - Find multiple peaks in partial spectra with various positions among those spectra:**


.. categories::

.. sourcelink::

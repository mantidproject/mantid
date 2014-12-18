.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm searches the specified spectra in a workspace for peaks,
returning a list of the found and successfully fitted peaks. The search
algorithm is described in full in reference [1]. In summary: the second
difference of each spectrum is computed and smoothed. This smoothed data
is then searched for patterns consistent with the presence of a peak.
The list of candidate peaks found is passed to a fitting routine and
those that are successfully fitted are kept and returned in the output
workspace (and logged at information level). The output
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ contains the following columns,
which reflect the fact that the peak has been fitted to a Gaussian atop
a linear background: spectrum, centre, width, height,
backgroundintercept & backgroundslope.

Subalgorithms used
##################

FindPeaks uses the :ref:`algm-SmoothData` algorithm to, well,
smooth the data - a necessary step to identify peaks in statistically
fluctuating data. The :ref:`algm-Fit` algorithm is used to fit candidate
peaks.

Treating weak peaks vs. high background
#######################################

FindPeaks uses a more complicated approach to fit peaks if
**HighBackground** is flagged. In this case, FindPeak will fit the
background first, and then do a Gaussian fit the peak with the fitted
background removed. This procedure will be repeated for a couple of
times with different guessed peak widths. And the parameters of the best
result is selected. The last step is to fit the peak with a combo
function including background and Gaussian by using the previously
recorded best background and peak parameters as the starting values.

Criteria To Validate Peaks Found
################################

FindPeaks finds peaks by fitting a Guassian with background to a certain
range in the input histogram. :ref:`algm-Fit` may not give a correct
result even if chi^2 is used as criteria alone. Thus some other criteria
are provided as options to validate the result

#. Peak position. If peak positions are given, and trustful, then the
   fitted peak position must be within a short distance to the give one.
#. Peak height. In the certain number of trial, peak height can be used
   to select the best fit among various starting sigma values.

Fit Window
##########

If FitWindows is defined, then a peak's range to fit (i.e., x-min and
x-max) is confined by this window.

If FitWindows is defined, starting peak centres are NOT user's input,
but found by highest value within peak window. (Is this correct???)

Estimation of peak's background and range
#########################################

If FindPeaksBackground fails, then it is necessary to estimate a rough peak range and background according to
observed data.

#. Assume the local background (within the given fitting window) is close to linear;
#. Take the first 3 and last 3 data points to calcualte the linear background;
#. Remove background (rougly) and calcualte peak's height, width, and centre;
#. If the peak centre (starting value) uses observed value, then set peakcentre to that value.  Otherwise, set it to given value;
#. Get the bin indexes of xmin, xmax and peakcentre;
#. Calcualte peak range, i.e., left and right boundary;
#. If any peak boundary exceeds or too close to the boundary, there will be 2 methods to solve this issue;

   #. If peak centre is restricted to given value, then the peak range will be from 1/6 to 5/6 of the given data points;
   #. If peak centre is set to observed value, then the 3 leftmost data points will be used for background.

References
----------

#. M.A.Mariscotti, *A method for automatic identification of peaks in the presence of background and its application to spectrum analysis , NIM 50 (1967) 309.*

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


.. categories::

.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits a single peak with a Gaussian function.

It assumes that the peak centre is at or near the point with the highest value
and the peak is at least three samples wide above the half maximum. It estimates
the peak height, sigma, and range; then calls the :ref:`algm-Fit` algorithm to
fit the peak curve.

Input and Output
################

The input parameters are a histogram workspace and the index of the histogram.
The ouput parameters are the fitted peak centre and sigma. If the data could not
be fitted, 0.0 is returned as both the peak centre and sigma values and a warning
message is logged. Errors in parameters raise RuntimeError.

ChildAlgorithms used
####################

Uses the :ref:`algm-Fit` algorithm to fit the peak curve with a Gaussian function.

Usage
-----

**Example: fit a peak**

.. testcode:: ExFitPeak

   # create a workspace with a single peak
   wspace = CreateSampleWorkspace(
      Function="User Defined",
      UserDefinedFunction="name=Gaussian,PeakCentre=2,Height=10,Sigma=.7",
      NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

   # attempt the fit
   fitResult = FitGaussian(wspace,0)
   if (0.0,0.0) == fitResult:
      print("the fit was not successful")
   else:
      print("the fitted peak: centre={:.2f}, sigma={:.2f}".format(fitResult[0], fitResult[1]))

.. testcleanup:: ExFitPeak

    DeleteWorkspace('wspace')

Output:

.. testoutput:: ExFitPeak

   the fitted peak: centre=2.05, sigma=0.70

.. categories::

.. sourcelink::

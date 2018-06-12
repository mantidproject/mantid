.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to fit a single peak with some checking mechanism
to ensure its fitting result is physical.

The output `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ contains the following
columns...

Peak profile
############

All peak profiles supported by Mantid are supported by FitPeak. 

Starting parameter values specified by user should be close to the real values,
unless the automatic starting values determination algorithm works well with that 
profile type.  

But only [[Gaussian]] has been well tested with this functionalities. 

Background 
##########

There are only three types of backgrounds that are supported, including
FlatBackground, LinearBackground and Quadratic. 

Input and Output
################

FitPeak is designed to fit the parameters of a peak function with background function in a given region.
In order to provide convenient UI to both user interactively and algorithms calling this algorithm automatically,
a various formats for the input of the starting values of functions and output of the fitted values of the functions are
implemented.

Default function parameters' names
==================================

Input function parameters' names should be exactly the same as those defined in Mantid.  
This brings some inconvenience to users through MantidPlot GUI. 
Thus user can input function parameters values in the default order,
which is instructed in the list of functions.  
For example, one background function type is defined as 'Linear (A0, A1)'.
As user chooses this function, he does not need to input background function parameter names, 
but writes values of A0 and A1, respectively.  

Output of function parameters
=============================

The output [[TableWorkspace]] contains the following columns...
* parameter name
* fitted parameter value
* error


Fitting algorithm
#################

FindPeaks uses a more complicated approach to fit peaks if '''HighBackground''' is flagged. In this case, FindPeak will fit the background first, and then do a Gaussian fit the peak with the fitted background removed.  This procedure will be repeated for a couple of times with different guessed peak widths.  And the parameters of the best result is selected.  The last step is to fit the peak with a combo function including background and Gaussian by using the previously recorded best background and peak parameters as the starting values.

Simple fit
==========
In the 'simple fit' mode, the algorithm will make a composite function including
peak and background function and fit it agains the observed data. 

It works well with good starting values of the peak and background function,
especially when the peak is significant with low background. 


High background fit
===================

In the 'high background fit' mode, the background will be removed first;
then the fitting is focussed on the 'pure' peak function;
and a composite function is fit agains the original data as the last step. 

This approach is developed due to the failure of 'simple fit' mode on the cases
that background level is much higher than the peak height.  
Without the background being removed, the optimizer intends to favor the background
rather than the peak function. 


Starting values of the peak function
====================================

* Peak height is estimated by the maximum value, with background removed, inside the peak range;
* Peak position can be set up either to the X value of the maximum Y value in the peak range, or to the vlaue specified by user accordin to user's selection.  For example, in the case of calibrating the offsets of detectors of powder diffractometers, the peak positons are unknown.  Then it is better to use the X value with the maximum Y value as the starting peak centre.  While in the case of striping vanadium peaks, all peaks' centres should be exactly same as the theortical values.  


Criteria To Validate Peaks Found
================================

FindPeaks finds peaks by fitting a Guassian with background to a certain
range in the input histogram. :ref:`algm-Fit` may not give a correct
result even if chi^2 is used as criteria alone. Thus some other criteria
are provided as options to validate the result

1. Peak position. If peak positions are given, and trustful, then the
fitted peak position must be within a short distance to the give one.

2. Peak height. In the certain number of trial, peak height can be used
to select the best fit among various starting sigma values.

3. Peak width: Peak width cannot be equal or wider than the given fit window. 


Fit Window and Peak Range
=========================

If FitWindows is defined, then a peak's range to fit (i.e., x-min and
x-max) is confined by this window.

If PeakRange is defined and starting peak centre given by user is not
within this range, then the situation is considered illegal. In future,
FitPeak might be able to estimate the peak centre in this situation by
locating the X-value whose corresponding Y-value is largest within
user-defined peak range.

Subalgorithms used
##################

-  Fit

Usage
-----

**Example - Fit a single peak with rough estimation on staring parameter values:**

.. testcode:: ExFitPeak

  Load(Filename=r'focussed.nxs', OutputWorkspace='focussed')
  FitPeak(InputWorkspace='focussed', OutputWorkspace='peak4', ParameterTableWorkspace='peak4result',
          WorkspaceIndex='3',PeakFunctionType='Gaussian (Height, PeakCentre, Sigma)',
          PeakParameterValues='2000,2.14,0.01',BackgroundType='Linear (A0, A1)',
          BackgroundParameterValues='2500,2000',FitWindow='2,2.3',PeakRange='2.1,2.25')


  tbws = mtd["peak4result"]
  chi2 = tbws.cell(0, 1)
  peakheight = tbws.cell(2, 1)
  peakcentre = tbws.cell(3, 1)
  sigma = tbws.cell(4, 1)
  print("Chi-square = {:.5f}: Peak centre = {:.5f}, Height = {:.2f}, Sigma = {:.5f}".format(chi2, peakcentre, peakheight, sigma))


.. testcleanup:: ExFitPeak

  DeleteWorkspace(Workspace='focussed')
  DeleteWorkspace(Workspace=tbws)

Output:

.. testoutput:: ExFitPeak

  Chi-square = 1.74892: Peak centre = 2.14201, Height = 7490.67, Sigma = 0.00776

.. categories::

.. sourcelink::

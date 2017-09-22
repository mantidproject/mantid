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

Background
##########

Linear background is used for fitting peaks.  The background parameters
will be estimated via *observation*.

Estimation of values of peak profiles
#####################################

Peak intensity and peak center are estimated by observing the maximum value within peak fit window.

Subalgorithms used
##################

FitPeaks uses the :ref:`algm-FitPeak` algorithm to fit each single peak.



Fit Window
##########

If FitWindows is defined, then a peak's range to fit (i.e., x-min and
x-max) is confined by this window.

If FitWindows is defined, starting peak centres are NOT user's input,
but found by highest value within peak window. (Is this correct???)


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

.. sourcelink::

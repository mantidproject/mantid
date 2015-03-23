.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm requires a workspace that is both in d-spacing, but has
also been preprocessed by the :ref:`algm-CrossCorrelate`
algorithm. In this first step you select one spectrum to be the
reference spectrum and all of the other spectrum are cross correlated
against it. Each output spectrum then contains a peak whose location
defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a
function (default is a :ref:`Gaussian <func-Gaussian>`) to the reference peak. The fit is used
to calculate the centre of the fitted peak, and the offset is then
calculated as:

This is then written into a `.cal file <http://www.mantidproject.org/CalFile>`__ for every detector
that contributes to that spectrum. All of the entries in the cal file
are initially set to both be included, but also to all group into a
single group on :ref:`algm-DiffractionFocussing`. The
:ref:`algm-CreateCalFileByNames` algorithm can be used to
alter the grouping in the cal file.

Fit for peak offset
###################

The algorithm to calculate offset of peaks' positions is to minimize a
cost function as

.. math:: \sum_{p} |X_{0, p} - (1+offset)\cdot X_{0, p}|/\chi^2_{p}

, which p is the index of a peak whose position is within MinD and MaxD.


Criteria on peaks
#################

The (fitted) peak must meet a series of criteria to be used to fit
spectrum's offset.

A peak will not be used if

-  its centre is out of pre-defined d-range, i.e., MinD and MaxD;
-  its centre is out of fitting window if it is defined;
-  its :math:`\chi^2` of peak fitting is larger than pre-defined maximum
   value;
-  its height is lower than pre-defined lowest peak height;
-  its observed maximum value corrected by background is smaller than user specified value; 
-  its signal/noise ratio is less than 5
   :math:`H\cdot FWHM\_To\_SIGMA/width < 5`;
-  its height is not outside of error bars of background
   :math:`H < \sqrt{H + B}/2`;
-  its z-value on :math:`\frac{\delta d}{d}` is larger than 2.0;
-  its offset from theoretical position exceeds the limitation specified by user; 
-  its resolution (:math:`\Delta(d)/d`) is out of user-specified range. 

Generate fit window
###################

There are two approach to generate fit window.  One is via property 'FitWindowMaxWidth';
and the other is 'FitwindowTableWorkspace'.

If neither of these 2 properties are correctly specified, then then there won't be any window defined.

Uniform fit window
==================

By specifying a postive float, maxWidth, for 'FitWindowMaxWidth',
it is the definition of fit window for peaks indexed from 0 to N-1:

   -  Peak 0: window = :math:`\min((X0_0-dmin), maxWidth)`, :math:`\min((X0_1-X0_0)/2,maxWidth)`
   -  Peak :math:`i (0 < i < N-1)`: window = :math:`\min((X0_i-X0_{i-1})/2, maxWidth)`, :math:`\min((X0_1-X0_0)/2, maxWidth)`
   -  Peak :math:`N-1`: window = :math:`\min((X0_i-X0_{i-1})/2, maxWidth)`, :math:`\min((dmax-X0_i), maxWidth)`

where :math:`X0_i` is the centre of i-th peak.

Fit window for individual peak
==============================

FitwindowTableWorkspace contains the fit window for each individual peak in the workspace
to find.
It contains :math:`1+2\times N` columns, where N is the number of peaks positions specified in 'DReference'.

- Column 0: spectrum number (workspace index) :math:`iws`.  If :math:`iws < 0`, then it is a 'universal' spectrum;
- Column :math:`2i+1`: left boundary of peak :math:`i` defined in 'DReference' of spectrum :math:`iws`;
- Column :math:`2i+2`: right boundary of peak :math:`i` defined in 'DReference' of spectrum :math:`iws`;

Default fit windows
+++++++++++++++++++

In the fit window table workspace, if there is a row, whose 'spectrum number' is a negative number,
then the fit windows defined in this row is treated as the default fit windows.
It means that for any spectrum that has no fit windows defined in the tableworkspace,
the default fit windows will be applied to it.


Quality of Fitting
------------------

GetDetOffsetsMultiPeaks have 2 levels of fitting. First it will call
FindPeaks to fit Bragg peaks within d-range. Then it will fit offsets
from the peak positions obtained in the previous step. Therefore, the
performance of FindPeaks is critical to this algorithm. It is necessary
to output values reflecting the goodness of fitting of this algorithm to
users.

Number of spectra that are NOT masked
#####################################

A spectrum will be masked if it is a dead pixel, has an empty detector
or has no peak that can be fit with given peak positions. The
performance of *FindPeaks* affects the third criteria. A better
algorithm to find and fit peaks may save some spectrum with relatively
much fewer events received, i.e., poorer signal.

:math:`\chi^2` of the offset fitting function
#############################################

The goodness of fit, :math:`\chi^2_{iws}`, of the offset fitting
function

.. math:: \sum_{p} |X_{0, p} - (1+offset)X_{0, p}|\cdot H^2_{p}

is an important measure of fitting quality on each spectrum (indexed as
iws).

Deviation of highest peaks
##########################

We observed that in some situation, the calibrated peaks' positions of
some spectra are far off to the targeted peak positions, while goodness
of fit such as :math:`\chi^2` are still good. It is usally caused by the
bad fit of one or two peaks in that spectrum, which feeds some erroreous
peak positions to peak offset fitting function.

This type of bad fitting is very easily identified by visualization,
because the shift of peaks from the correct positions is significant in
fill plot.

Therefore, deviation of highest peak if spectrum i, :math:`D_{i}` is
defined as:

.. math:: D_{i} = |X^{(o)}\cdots(1+offset) - X^{(c)}|

where :math:`X^{(o)}` is the fitted centre of the highest peak of
spectrum i, and :math:`X^{(c)}` is the theoretical centre of this peak.

Collective quantities to illustrate goodness of fitting (still in developement)
###############################################################################

Be noticed that the idea of this section is still under development and
has not been implemented yet.

On the other hand, since GetDetOffsetsMultiPeaks always operates on an
EventWorkspace with thousands or several ten thousands of spectra, it is
very hard to tell the quality of fitting by looking at
:math:`\chi^2_{iws}` of all spectra. Hence, Here are two other
parameters are defined for comparison of results.

    :math:`g_1 = \frac{\sum_{s}D_{s}^2}{N_{nm}}`

, where s is the index of any unmasked spectrum and :math:`N_{mn}` is
the number of unmasked spectra;

    :math:`g_2 = \frac{\sum_{s}D_{s}^2\cdot H_{s}^2}{N_{nm}}`,

where :math:`H_{s}` is the height of highest peak of spectrum s.

Standard error on offset
########################

The offset in unit of d-spacing differs is proportional to peak's
position by definition:

.. math:: X_0^{(f)} = X_0^{(o)} * (1+offset)

where :math:`X_0^{(f)}` is the focussed peak position, and
:math:`X_0^{(o)}` is the observed peak position by fitting.

As different spectrum covers different d-space range, the highest peak
differs. Therefore, the error of offset should be normalized by the
peak's position.

.. math:: E = (X_0^{(f)} - X_0^{(o)}*(1+offset))/X_0^{(f)} = 1 - \frac{X_0^{(o)}}{X_0^{(f)}}\cdot(1+offset)

And it is unitless.

By this mean, the error of all peaks should be close if they are fitted
correctly.


Spectra to be masked
--------------------

A MaskWorskpace is output from the algorithm.  Along with it, a TableWorkspace is output
to describe the status of offset calculation. 

Here are the cases that a spectra (i.e., a detector) will be masked in the output MaskWorkspace. 

-  An empty spectrum (i.e., the corresponding EventList is empty).  It is noted as "empty det";

-  A dead detector, i.e., the corresponding spectrum has counts less than :math:`10^{-3}` in defined d-range.  It isnoted as "dead det";

-  A spectrum that does not have peak within specified d-range.  It is noted as "no peaks". Here is the criteria for this case.

 - Algorithm FindPeaks fails to find any peak;
 - No peak found has height larger than specified 'MinimumPeakHeight';
 - No peak found has observed height larger than specified 'MinimumPeakHeightObs';
 - No peak found has resolution within specified range;
 - No peak found whose calculated offset is smaller than the user-defined maximum offset.

Usage
-----

.. testcode::

  import os

  # Create a workspace with two Gaussian peaks in each spectrum
  function_str = 'name=Gaussian,Height=3,PeakCentre=5,Sigma=0.3;name=Gaussian,Height=2.1,PeakCentre=15,Sigma=0.3'
  ws = CreateSampleWorkspace(Function='User Defined',UserDefinedFunction=function_str,XMin=0,XMax=20,BinWidth=0.1)
  # Make sure the X axis is in d-spacing.
  ws.getAxis(0).setUnit( 'dSpacing' )

  # Generate a file path to save the .cal file at.
  calFilePath = os.path.expanduser( '~/MantidUsageExample_CalFile.cal' )

  # Run the algorithm
  msk = GetDetOffsetsMultiPeaks(ws,DReference=[5,15], GroupingFileName=calFilePath)

  # Read the saved .cal file back in
  f = open( calFilePath, 'r' )
  file = f.read().split('\n')
  f.close()

  # Print out first 10 lines of the file
  print file[0][:55],'...'
  for line in file[1:10]:
    print line

Output
######

.. testoutput::

  # Calibration file for instrument basic_rect written on ...
  # Format: number    UDET         offset    select    group
          0            100     -0.0033750       1       1
          1            101     -0.0033750       1       1
          2            102     -0.0033750       1       1
          3            103     -0.0033750       1       1
          4            104     -0.0033750       1       1
          5            105     -0.0033750       1       1
          6            106     -0.0033750       1       1
          7            107     -0.0033750       1       1

.. testcleanup::

  os.remove( calFilePath )

.. seealso :: Algorithm :ref:`algm-EstimateResolutionDiffraction`

.. categories::

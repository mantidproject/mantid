.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Description
-----------

This algorithm requires a workspace that is both in d-spacing, but has
also been preprocessed by the :ref:`algm-CrossCorrelate`
algorithm. In this first step you select one spectrum to be the
reference spectrum and all of the other spectrum are cross correlated
against it. Each output spectrum then contains a peak whose location
defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a
`Gaussian <Gaussian>`__ function to the reference peak. The fit is used
to calculate the centre of the fitted peak, and the offset is then
calculated as:

This is then written into a `.cal file <CalFile>`__ for every detector
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

Spectra to mask
###############

-  Empty spectrum marked as "empty det"

-  Spectrum with counts less than 1.0E^-3 in defined d-range as "dead
   det"

-  Calculated offset exceeds the user-defined maximum offset.

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
-  its signal/noise ratio is less than 5
   :math:`H\cdot FWHM\_To\_SIGMA/width < 5`;
-  its height is not outside of error bars of background
   :math:`H < \sqrt{H + B}/2`;
-  its z-value on :math:`\frac{\delta d}{d}` is larger than 2.0.

Generate fit window
###################

-  Required parameter: maxWidth. If it is not given, i.e., less or equal
   to zero, then there won't be any window defined;
-  Definition of fit window for peaks indexed from 0 to N-1

   -  Peak 0: window = Min((X0\_0-dmin), maxWidth), Min((X0\_1-X0\_0)/2,
      maxWidth)
   -  Peak i (0 < i < N-1): window = Min((X0\_i-X0\_{i-1})/2, maxWidth),
      Min((X0\_1-X0\_0)/2, maxWidth)
   -  Peak N-1: window = Min((X0\_i-X0\_{i-1})/2, maxWidth),
      Min((dmax-X0\_i), maxWidth)

where X0\_i is the centre of i-th peak.

Fitting Quality
---------------

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

Usage
-----

**Python**

OutputW,NumberPeaksFitted,Mask =
GetDetOffsetsMultiPeaks("InputW",0.01,2.0,1.8,2.2,"output.cal")

.. categories::

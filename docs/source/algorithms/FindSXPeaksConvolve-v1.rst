.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is an algorithm to find single-crystal Bragg peaks in a :ref:`MatrixWorkspace <MatrixWorkspace>` with detector
banks of type :ref:`RectangularDetector <RectangularDetector>` (e.g. SXD, TOPAZ).

There are two peak finding strategies set by ``PeakFindingStrategy``:

a. ``PeakFindingStrategy="IOverSigma"`` - by integrating the data by convolution with a shoebox kernel and looking for
   regions with statistically significant I/sigma (larger than ``ThresholdIoverSigma``). Note a valid peak would be
   expected to have intensity/sigma > 3 and stronger peaks will have a larger intensity/sigma.

b. ``PeakFindingStrategy="VarianceOverMean"`` - by looking for regions with ratio of variance/mean larger than
   ``ThresholdVarianceOverMean`` - note for a poisson distributed counts with a constant count-rate the ratio is
   expected to be 1. This peak finding criterion taken from DIALS [1]_.


The size of the kernel is defined in the input to the algorithm and should match the approximate extent of a typical peak.
The size on the detector is governed by ``NRows`` and ``NCols`` which are in units of pixels.
The size of the kernel along the TOF dimension can be specified in one of two ways:

1. Provide ``NBins`` - number of TOF bins in the kernel

2. Setting ``GetNBinsFromBackToBackParams=True`` and providing ``NFWHM`` - in which case ``NBins``  will be NFWHM x FWHM
   of a :ref:`BackToBackExponential <func-BackToBackExponential>` peak at the center of each detector panel/bank at the
   middle of the spectrum.

Note to use method 2, back-to-back exponential coefficients must be defined in the Parameters.xml file for the
instrument.

The shoebox integration (for ``PeakFindingStrategy="IOverSigma"``) requires a background shell with negative weights,
such that the total kernel size is increased by a factor 1.25 along each dimension (such that there are approximately
the same number of elements in the kernel and the background shell). The integral of the kernel and background shell
together is zero.

The integrated intensity is evaluated by convolution of the signal array with the kernel and the square of the error on
the integrated intensity is determined by convolution of the squared error array with the squared value of the kernel.


Usage
-----

**Example - FindSXPeaksConvolve**

.. testcode:: exampleFindSXPeaksConvolve

    from mantid.simpleapi import *

    ws = Load(Filename="SXD23767.raw", OutputWorkspace="SXD23767")
    peaks = FindSXPeaksConvolve(InputWorkspace=ws, PeaksWorkspace='peaks_out', GetNBinsFromBackToBackParams=True, ThresholdIoverSigma=5.0)

    print(f"Found {peaks.getNumberPeaks()} peaks")

**Output:**

.. testoutput:: exampleFindSXPeaksConvolve

    Found 261 peaks

References
----------

.. [1]  Winter, G., et al.  Acta Crystallographica Section D: Structural Biology 74.2 (2018): 85-97.

.. categories::

.. sourcelink::

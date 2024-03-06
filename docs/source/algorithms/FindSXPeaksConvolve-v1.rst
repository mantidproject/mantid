.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is an algorithm to find single-crystal Bragg peaks in a :ref:`MatrixWorkspace <MatrixWorkspace>` with detector
banks of type :ref:`RectangularDetector <RectangularDetector>` (e.g. SXD, TOPAZ) by integrating the data using a
convolution with a shoebox kernel.

The size of the kernel is defined in the input to the algorithm and should match the approximate extent of a typical peak.
The size on the detector is governed by ``NRows`` and ``NCols`` which are in units of pixels.
The size of the kernel along the TOF dimension can be specified in one of two ways:

1. Provide ``NBins`` - number of TOF bins in the kernel

2. Setting ``GetNBinsFromBackToBackParams=True`` and providing ``NFWHM`` - in which case ``NBins``  will be NFWHM x FWHM
   of a :ref:`BackToBackExponential <func-BackToBackExponential>` peak at the center of each detector panel/bank at the
   middle of the spectrum.

Note to use method 2, back-to-back exponential coefficients must be defined in the Parameters.xml file for the
instrument.

The integration requires a background shell with negative weights, such that the total kernel size is increased by a
factor 1.25 along each dimension (such that there are approximately the same number of elements in the kernel and the
background shell). The integral of the kernel and background shell together is zero.

The integrated intensity is evaluated by convolution of the signal array with the kernel and the square of the error on
the integrated intensity is determined by convolution of the squared error array with the squared value of the kernel.

The threshold for peak detection is given by ``ThresholdIoverSigma`` which is the cutoff ratio of intensity/sigma (i.e.
a valid peak would be expected to have intensity/sigma > 3). Stronger peaks will have a larger intensity/sigma.

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


.. categories::

.. sourcelink::

.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is an algorithm to integrate single-crystal Bragg peaks in a :ref:`MatrixWorkspace <MatrixWorkspace>` with x-units
of Time-of-Flight (TOF). It is adapted and extended from an algorithm in SXD2001 [1]_.

The algorithm proceeds by fitting a ``PeakFunction`` and ``BackgroundFunction`` to the TOF spectrum of individual
pixels. If the fit is successful (fit converged and the ratio of intensity/sigma > ``IOverSigmaThreshold``), then the
fit is attempted on a pixel that is adjacent to a previously successfully fit pixel. The algorithm stops
when no adjacent pixels can be successfully fitted.

The algorithm requires the user to define the number of pixels in the vicinity of a peak using the input ``NRows``
and ``NCols`` and the TOF extent of the data to fit, which can be specified in one of two ways:

1. Provide ``NBins`` directly - number of TOF bins in the kernel

2. Setting ``GetNBinsFromBackToBackParams=True`` and providing ``NFWHM`` - in which case ``NBins`` will be NFWHM x FWHM
   of a :ref:`BackToBackExponential <func-BackToBackExponential>` peak at the center of each detector panel/bank at the
   middle of the spectrum.

Note to use method 2, back-to-back exponential coefficients must be defined in the Parameters.xml file for the
instrument.

Optionally if ``OutputFile`` is provided a pdf can be output that shows the shoebox kernel and the data integrated along
each dimension like so

.. figure:: ../images/IntegratePeaks1DProfile_OutputFile.png
    :align: center
    :width: 50%
    :alt: Output for a peak from SXD NaCl data showing the TOF integrated intensity on the detector and the pixels
          fitted (white circles) along with the peak position (red +) and the TOF focussed spectrum in the fitted pixels
          along with the fitted intensity (red curve).


Usage
-----

**Example - IntegratePeaks1DProfile**

.. testcode:: exampleIntegratePeaks1DProfile

    from mantid.simpleapi import *

    Load(Filename="SXD23767.raw", OutputWorkspace="SXD23767")
    CreatePeaksWorkspace(InstrumentWorkspace="SXD23767", NumberOfPeaks=0, OutputWorkspace="peaks")
    AddPeak(PeaksWorkspace="peaks", RunWorkspace="SXD23767", TOF=8303.3735339704781, DetectorID=7646)

    peaks_out = IntegratePeaks1DProfile(InputWorkspace="SXD23767", PeaksWorkspace="peaks", OutputWorkspace="peaks_int",
                                        GetNBinsFromBackToBackParams=True, NFWHM=6, CostFunction="Poisson",
                                        PeakFunction="BackToBackExponential", FixPeakParameters='A',
                                        FractionalChangeDSpacing=0.01, IntegrateIfOnEdge=True)

    print(f"I/sigma = {peaks_out.getPeak(0).getIntensityOverSigma():.2f}")

**Output:**

.. testoutput:: exampleIntegratePeaks1DProfile

    I/sigma = 92.97

References
----------

.. [1] Gutmann, M. J. (2005). SXD2001. ISIS Facility, Rutherford Appleton Laboratory, Oxfordshire, England.

.. categories::

.. sourcelink::

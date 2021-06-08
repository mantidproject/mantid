.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Estimates the background for a spectra by employing an iterative smoothing procedure adapted from an algorithm published in [1].
In each iteration a smoothing window is convolved with the data to produce a new spectra which is compared point by point with spectra from the previous iteration, with the lowest of the two intensities retained. In this way counting statistics are taken into conisderation to some extent, but if the data are noisy then this can lead to the background being underestimated (by the approxiumate amplitude of the noise). This can be avoided by rebinning/rebunching data, however this is not always desirable. Therefore there is an option to apply a Savitzky–Golay filter with a linear polynomial over a window the same length as the smoothing window before the iterative smoothing.


Useage
-----------

**Example:**

.. code-block:: python

	from mantid.simpleapi import *

	CreateSampleWorkspace(OutputWorkspace='ws', Function='Multiple Peaks', NumBanks=1, BankPixelWidth=1, XMax=20, BinWidth=0.2)
	EnggEstimateFocussedBackground(InputWorkspace='ws', OutputWorkspace='ws_bg', NIterations='200', XWindow=2.5, ApplyFilterSG=False)


References
----------

The source for how this calculation is done is

#. Brückner, S. (2000). Estimation of the background in powder diffraction patterns through a robust smoothing procedure. *Journal of Applied Crystallography*, 33(3), 977-979._


.. categories::

.. sourcelink::

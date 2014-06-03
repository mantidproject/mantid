.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Takes a histogram in a 2D workspace and fit it to a Lorentzian function,
i.e. to the function:

.. math:: \mbox{BG0}+\mbox{BG1}*x+\mbox{Height}* \left( \frac{\mbox{HWHM}^2}{(x-\mbox{PeakCentre})^2+\mbox{HWHM}^2} \right)

where

-  BG0 - constant background value
-  BG1 - constant background value
-  Height - height of peak (at maximum)
-  PeakCentre - centre of peak
-  HWHM - half-width at half-maximum

Note that the FWHM (Full Width Half Maximum) equals two times HWHM, and
the integral over the Lorentzian equals
:math:`\mbox{Height} * \pi * \mbox{HWHM}` (ignoring the linear
background). In the literature you may also often see the notation
:math:`\gamma` = HWHM.

The figure below illustrate this symmetric peakshape function fitted to
a TOF peak:

.. figure:: /images/LorentzianWithConstBackground.png
   :alt: LorentzianWithConstBackground.png

   LorentzianWithConstBackground.png

.. categories::

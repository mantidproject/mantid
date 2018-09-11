.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm extends the 
:ref:`TOFSANSResolution <algm-TOFSANSResolution>`
to implement the experimentally determined TOF resolution for EQSANS.

The Q resolution for a TOF SANS has two components: a geometrical contribution and a 
contributions from the resolution in TOF.

The TOF resolution for EQSANS was measured to be:

:math:`\Delta T = y_0 + A ( f/(1+\exp((\lambda-x^0_1)/k_1)) + (1-f)/(1+\exp((\lambda-x^0_2)/k_2)) )`

where :math:`\lambda` is the wavelength and 

:math:`y_0 = -388`

:math:`A = 3838`

:math:`f = 0.04398`

:math:`x^0_1 = 3.392`

:math:`x^0_2 = 134.3`

:math:`k_1 = -0.5587`

:math:`k_2 = -65.46`

This algorithm is generally not called directly. It's called by 
:ref:`EQSANSAzimuthalAverage1D <algm-EQSANSAzimuthalAverage1D>`
after the calculation of I(Q). It can only be applied to an I(Q) workspace.

.. categories::

.. sourcelink::

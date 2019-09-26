.. _func-TriangleOsc:

===========
TriangleOsc
===========

.. index:: TriangleOsc

Description
-----------

A trigular waveform oscillation.

.. math:: x(t)=4f \left(t-\frac{1}{2f} \left\lfloor 2ft+\frac{1}{2} \right\rfloor \right)(-1)^{\left\lfloor 2ft+\frac{1}{2}\right\rfloor}

where 

:math:`\lfloor n \rfloor` is the Floor Function of ''n'',

and :math:`f` is the frequency (MHz) of the oscillation.

.. figure:: /images/TriangleOsc.png

.. attributes::

.. properties::

References
----------

[1]  `Fourier Series -- Triangular Wave, WolframMathWorld <http://mathworld.wolfram.com/FourierSeriesTriangleWave.html>`_.

.. categories::

.. sourcelink::

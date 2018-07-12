.. _func-Convolution:

===========
Convolution
===========

.. index:: Convolution

Description
-----------

Convolution is an extension of :ref:`func-CompositeFunction`
which performs convolution of its members using either the
Fast Fourier Transform (symmetric domain) or the direct
formula (asymmetric domain).

.. math:: f(x)=\int\limits_{A}^{B}R(x-\xi)F(\xi)\mbox{d}\xi

Here :math:`R` is the first member function and :math:`F` is the second
member. A Convolution must have exactly two member functions. The
members can be composite if necessary. Interval :math:`[A,B]` is the
fitting interval.

FFT mode
========

if :math:`|A|` similar to :math:`|B|`, the function is evaluated
by first transforming :math:`R` and :math:`F` to the Fourier domain,
multiplying the transforms, then transforming back to the original domain.
The GSL FFT routines are used to do the actual transformations.

It should be noted that the two functions (:math:`R` and :math:`F`) are
evaluated on different intervals. :math:`F` is computed on :math:`[A,B]`
while :math:`R` is computed on :math:`[-\Delta/2, \Delta/2]`, where
:math:`\Delta=B-A`.

In the following example a :ref:`func-Convolution` is convolved with a
box function:

.. figure:: /images/Convolution.png
   :alt: Convolution.png

Note that the box function is defined on interval [-5, 5]:

.. figure:: /images/Box.png
   :alt: Box.png

Direct mode
===========

If :math:`|A|` and :math:`|B|` differ, the convolution is performed
with the direct formula. :math:`F` is computed on :math:`[A-B,B-A]`
and :math:`R` is computed on :math:`[A,B]`. This setting guarantees
that :math:`F` overlaps completely :math:`R` in the domain :math:`[A,B]`
when performing the convolution.

In the following example a QENS signal is fitted to a two-Lorentzian
model, convolved with the experimental resolution, in the
asymmetric energy range :math:`[A,B]=[-0.12, 0.52]`.

.. figure:: /images/ConvolutionAsymmetric.png
   :alt: ConvolutionAsymmetric.png

.. attributes::

.. properties::

.. categories::

.. sourcelink::
   :h: Framework/CurveFitting/inc/MantidCurveFitting/Functions/Convolution.h
   :cpp: Framework/CurveFitting/src/Functions/Convolution.cpp

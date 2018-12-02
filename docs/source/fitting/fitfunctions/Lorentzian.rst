.. _func-Lorentzian:

==========
Lorentzian
==========

.. index:: Lorentzian

Description
-----------

A Lorentzian function is defined as:

.. math::

   L(x) = \frac{A}{\pi} \left( \frac{\frac{\Gamma}{2}}{(x-x_0)^2 + (\frac{\Gamma}{2})^2}\right)

where:

-  A (Amplitude or :math:`I`) - Intensity scaling (intensity)
-  :math:`x_0` (PeakCentre) - centre of peak
-  FWHM (:math:`\Gamma` or :math:`H` in other functions' definition)

Note that the FWHM (Full Width Half Maximum) equals two times HWHM, and
the integral over the Lorentzian equals the intensity scaling A.

.. math::
   \int_{-\infty}^{\infty}L(x) = A \int_{-\infty}^{\infty}L'(x)  = A

Effective peak parameters
+++++++++++++++++++++++++

Lorentzian function has the following effective parameters

- Height (:math:`h`):  :math:`h = L(x_0) = \frac{2A}{\pi\Gamma}`


The figure below illustrate this symmetric peakshape function fitted to
a TOF peak:

.. figure:: /images/LorentzianWithConstBackground.png
   :alt: LorentzianWithConstBackground.png

.. attributes::

.. properties::

.. categories::

.. sourcelink::

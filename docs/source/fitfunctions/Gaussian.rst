.. _func-Gaussian:

========
Gaussian
========

.. index:: Gaussian

Description
-----------

A Gaussian function (also referred to as a normal distribution) is
defined as:

.. math:: \mbox{Height}*\exp \left( -0.5*\frac{(x-\mbox{PeakCentre})^2}{\mbox{Sigma}^2} \right)

where

-  Height - height of peak
-  PeakCentre - centre of peak
-  Sigma - Gaussian width parameter

Note that the FWHM (Full Width Half Maximum) of a Gaussian equals
:math:`2\sqrt{2\ln 2}*\mbox{Sigma}`.

The figure below illustrate this symmetric peakshape function fitted to
a TOF peak:

.. figure:: /images/GaussianWithConstBackground.png
   :alt: GaussianWithConstBackground.png

.. attributes::

.. properties::

.. categories::

.. sourcelink::

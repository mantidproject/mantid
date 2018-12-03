.. _func-PseudoVoigt:

===========
PseudoVoigt
===========

.. index:: PseudoVoigt

Description
-----------

The Pseudo-Voigt function is an approximation for the Voigt function, which is a convolution of Gaussian and Lorentzian function. It is often used as a peak profile in powder diffraction for cases where neither a pure Gaussian or Lorentzian function appropriately describe a peak.

Instead of convoluting those two functions, the Pseudo-Voigt function is defined as the sum of a Gaussian peak :math:`G(x)` and a Lorentzian peak :math:`L(x)`, weighted by a fourth parameter :math:`\eta` (values between 0 and 1) which shifts the profile more towards pure Gaussian or pure Lorentzian when approaching 1 or 0 respectively:

.. math:: pV(x) = \eta G(x) + (1 - \eta)L(x)

Both functions share three parameters: Height (height of the peak at the maximum), PeakCentre (position of the maximum) and FWHM (full width at half maximum of the peak).

Thus pseudo-voigt can be expressed in such way

.. math:: pV(x) = I \cdot (\eta \cdot g(x, \Gamma) + (1 - \eta) \cdot l(x, \Gamma))

where :math:`g(x, \Gamma)` and `l(x, \Gamma)` are normalized Gaussian and Lorentzian.



Native peak parameters
++++++++++++++++++++++

Pseudo-voigt function in Mantid has the following native parameters

- Peak intensity :math:`I`: shared peak height between Gaussian and Lorentzian.  
- Peak width FWHM :math:`\Gamma` or :math:`H`: shared 
- Peak position :math:`x_0`
- Gaussian ratio :math:`\eta`

From given FWHM

For Gaussian part:

.. math:: \sigma = \frac{1}{2\sqrt{2\ln(2)}} * \Gamma

.. math:: g(x, \Gamma) = A_G \cdot exp(-\frac{(x-x_0)^2}{2\sigma})

where

.. math:: A_G = \frac{2}{H}\sqrt{\frac{\ln{2}}{\pi}} = \frac{1}{\sigma\sqrt{2\pi}}

For Lorentzian part:

.. math:: l(x) = \frac{1}{\pi} \cdot \frac{\Gamma/2}{(x-x_0)^2 + (\Gamma/2)^2}

Thus both :math:`g(x)` and :math:`l(x)` are normalized.


Effective peak parameter
++++++++++++++++++++++++

- Peak height :math:`h`: 

.. math:: h = I \cdot (\eta \cdot A_G + (1 - \eta) \cdot \frac{2}{\pi\cdot\Gamma}) = (1 + (\sqrt{\ln{2}\pi}-1)\eta) \frac{2\cdot I}{\pi\cdot H}


About previous implementation
+++++++++++++++++++++++++++++

Before Mantid release v3.14, the equation of Pseudo-Voigt is defined as

.. math:: pV(x) = h \cdot (\eta \cdot \exp(-\frac{(x-x_0)^2}{-2\sigma^2}) + \frac{(\Gamma/2)^2}{(x-x_0)^2 + (\Gamma/2)^2})

This equation has several issues:

1. It does not have normalized Gaussian and Lorentzian. 
2. At :math:`x = x_0`, :math:`pV = h`.  By this definition, the mixing ratio factor :math:`\eta` between Gaussian and Lorentzian is the the intensity ratio at :math:`x = x_0`.  But it does not make sense at any other :math:`x` value. According to the literature or manual (Fullprof and GSAS), :math:`\eta` shall be the ratio of the intensities between Gaussian and Lorentzian.


The figure below shows data together with a fitted Pseudo-Voigt function, as well as Gaussian and Lorentzian with equal parameters. The mixing parameter for that example is 0.7, which means that the function is behaving more like a Gaussian.

.. figure:: /images/PseudoVoigt.png
   :alt: Comparison of Pseudo-Voigt function with Gaussian and Lorentzian profiles.

.. attributes::

.. properties::

.. categories::

.. sourcelink::

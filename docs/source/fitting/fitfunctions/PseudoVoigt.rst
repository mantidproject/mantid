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

.. math:: pV(x) = I \cdot (\eta \cdot G'(x, H) + (1 - \eta) \cdot (x, H))

where :math:`G'(x, H)` and `L'(x, H)` are normalized Gaussian and Lorentzian.

To be noticed that the Fullprof manual's way to define parameter names is following.
In the code, in order to avoid confusion between :math:`H` and peak height :math:`h`, 
*gamma* will be used instead of :math:`H`.



Native peak parameters
++++++++++++++++++++++

Pseudo-voigt function in Mantid has the following native parameters

- Peak intensity :math:`I`: shared peak height between Gaussian and Lorentzian.  
- Peak width FWHM :math:`\Gamma` or :math:`H`: shared FWHM be between Gaussian and Lorentzian
- Peak position :math:`x_0`
- Gaussian ratio :math:`\eta`: ratio of intensity of Gaussian.

From given FWHM

**Gaussian part** :math:`G'(x, H)`

.. math:: G'(x, H) = a_G \cdot exp(-b_G (x - x_0)^2) = \frac{1}{\sigma\sqrt{2\pi}} \exp{-\frac{(x-x_0)^2}{2\sigma^2}}


where

.. math:: a_G = \frac{2}{H}\sqrt{\frac{\ln{2}}{\pi}} = \frac{1}{\sigma\sqrt{2\pi}}

.. math:: b_G = \frac{4\ln{2}}{H^2}

.. math:: \sigma = \frac{H}{2\sqrt{2\ln(2)}}

**Lorentzian part** :math:`L'(x, H)`

.. math:: L'(x) = \frac{1}{\pi} \cdot \frac{H/2}{(x-x_0)^2 + (H/2)^2}

Thus both :math:`G'(x)` and :math:`L'(x)` are normalized.


Effective peak parameters
+++++++++++++++++++++++++

- Peak height :math:`h`: 
.. math:: h = I \cdot (\eta \cdot a_G + (1 - \eta) \cdot \frac{2}{\pi\cdot H}) = (1 + (\sqrt{\ln{2}\pi}-1)\eta) \frac{2\cdot I}{\pi\cdot H}

- :math:`\sigma`:
.. math:: \sigma = \frac{H}{2\sqrt{2\ln(2)}}


Derivative
++++++++++

- To mixing paramter :math:`\eta`
.. math:: \frac{\partial pV(x)}{\partial \eta} = I \cdot [G'(x, H) - L'(x, H)]


- To intensity :math:`I`
.. math:: \frac{\partial pV(x)}{\partial x} = \eta G'(x, H) + (1-\eta) L'(x, H)

- To peak centre :math:`x_0`
.. math:: \frac{\partial pV(x)}{\partial x_0} = I \cdot [\eta \frac{\partial G'(x, H)}{\partial x_0} + (1 - \eta) \frac{\partial L'(x, H)}{\partial x_0}]

.. math:: \frac{\partial L'(x, H)}{\partial x_0} = a_G \exp{(-b_G(x-x_0)^2)} (-b_G) (-2) (x - x_0) = 2 b_G (x - x_0) G'(x, H)

.. math:: \frac{\partial L'(x, H)}{\partial x_0} = \frac{H}{2} (-1) (-2) (x - x_0) \frac{1}{[(x - x_0)^2 + \frac{H^2}{4}]^2} = \frac{-(x-x_0)H}{[(x - x_0)^2 + \frac{H^2}{4}]^2}

- To peak width :math:`H`
.. math:: \frac{\partial pV(x)}{\partial H} = I \cdot [\eta \frac{\partial G'(x, H)}{\partial H} + (1 - \eta) \frac{\partial L'(x, H)}{\partial H}]

For Gaussian part:
.. math:: \frac{\partial G'(x, H)}{\partial H} = \frac{\partial a_G}{\patial H} e^{-b_G(x-x_0)^2} + a_G \frac{\partial\exp{(e^{-b_G(x-x_0)^2)}}}{\partial H} = t_1 + t_2

.. math:: t_1 = \frac{-1}{H} a_G e^{-b_G(x-x_0)^2} = \frac{-1}{H} G'(x, H)

.. math:: t_2 = a_G \exp{(e^{-b_G(x-x_0)^2)})} (-1) (x-x_0)^2 \frac{\partial b_G}{\partial H} = G'(x, H) (-1) (x-x_0)^2 \frac{-2}{H} b_G = 2 b_G (x-x_0)^2 G'(x, H)

For Lorentzian part:
.. math:: \frac{\partial L'(x, H)}{\partial H} = \frac{1}{\pi} (\frac{\partial H/2}{\partial H}\frac{1}{(x-x_0)^2} + (H/2)^2} + \frac{H}{2}\frac{\partial \frac{1}{(x-x_0)^2 + (H/2)^2}}{\partial H} = t_3 + t_4

.. math:: t_3 = {1}{2\pi} \frac{1}{(x-x_0)^2 + (H/2)^2} = \frac{L'(x, H)}{H}

.. math:: t_4 = \frac{H}{2\pi}\frac{-1}{[(x-x_0)^2} + (H/2)^2]^2} \frac{H}{2} = -\pi[L'(x, H)]^2


Estimation of peak parameters
+++++++++++++++++++++++++++++

- Peak width (FWHMW :math:`H`)

- Peak intensity

- Peak height

- Peak centre

- Mixing parameter :math:`\eta` then can be estimated by peak width, intensity and height at estimated peak centre. 



About previous implementation
+++++++++++++++++++++++++++++

Before Mantid release v3.14, the equation of Pseudo-Voigt is defined as

.. math:: pV(x) = h \cdot [\eta \cdot \exp(-\frac{(x-x_0)^2}{-2\sigma^2}) + (1-\eta)\frac{(\Gamma/2)^2}{(x-x_0)^2 + (\Gamma/2)^2}]

This equation has several issues:

1. It does not have normalized Gaussian and Lorentzian. 
2. At :math:`x = x_0`, :math:`pV(x_0) = h`.  By this definition, the mixing ratio factor :math:`\eta` between Gaussian and Lorentzian is the the intensity ratio at :math:`x = x_0`.  But it does not make sense with other :math:`x` value. According to the literature or manual (Fullprof and GSAS), :math:`\eta` shall be the ratio of the intensities between Gaussian and Lorentzian.


The figure below shows data together with a fitted Pseudo-Voigt function, as well as Gaussian and Lorentzian with equal parameters. The mixing parameter for that example is 0.7, which means that the function is behaving more like a Gaussian.

.. figure:: /images/PseudoVoigt.png
   :alt: Comparison of Pseudo-Voigt function with Gaussian and Lorentzian profiles.

.. attributes::

.. properties::

.. categories::

.. sourcelink::

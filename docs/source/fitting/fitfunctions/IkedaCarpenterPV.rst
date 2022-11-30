.. _func-IkedaCarpenterPV:

================
IkedaCarpenterPV
================

.. index:: IkedaCarpenterPV

Description
-----------

This peakshape function is designed to be used to fit time-of-flight
peaks. In particular this function is the convolution of the
Ikeda-Carpenter function (Ref [1]), which aims to model the neutron pulse shape
from a moderator, and a pseudo-Voigt that models any broadening to the peak
due to sample properties etc.

The convolution of the Ikeda-Carpenter function with psuedo-Voigt is (Ref [3])

.. math:: N \left[ (1-\eta)\Omega_G + \eta \Omega_L \right]

where :math:`\Omega_G` and :math:`\Omega_L` are the Gaussian and Lorentzian parts of the function, respectively:

.. math:: \Omega_G = N_u e^u erfc(y_u) + N_v e^v erfc(y_v) + N_s e^s erfc(y_s) + N_r e^r erfc(y_r)

.. math:: \Omega_L = -\frac{2}{\pi} \left[ N_u Im[e^{z_u}E_1(z_u)] + N_v Im[e^{z_v}E_1(z_v)] + N_s Im[e^{z_s}E_1(z_s)] + N_r Im[e^{z_r}E_1(z_r)]  \right]

:math:`erfc` is the complementary error function, :math:`E_1` is the complex exponential integral, and :math:`Im` is the imaginary component.

The parameters used above are defined as (from Panel 13 of Ref [3]):

+------------------------------------+------------------------------------------------------------------------+--------------------------------------------------------------+------------------------------------------------------+---------------------------------------------------------------------+
| .. math:: k = 0.05                 | .. math:: z_s = -\alpha dt + i\frac{1}{2} \alpha \gamma                | .. math:: u = \frac{1}{2} \alpha^- (\alpha^- \sigma^2 - 2dt) | .. math:: N = \frac{1}{4} \alpha \frac{(1-k^2)}{k^2} | .. math:: y_u = \frac{ (\alpha^- \sigma^2 - dt) }{\sqrt{2\sigma^2}} |
| .. math:: \alpha^- = \alpha(1 - k) | .. math:: z_u = -\alpha^- dt + i\frac{1}{2} \alpha^- \gamma = (1-k)z_s | .. math:: v = \frac{1}{2} \alpha^+ (\alpha^+ \sigma^2 - 2dt) | .. math:: N_u = 1 - R \frac{\alpha^-}{x}             | .. math:: y_v = \frac{ (\alpha^+ \sigma^2 - dt) }{\sqrt{2\sigma^2}} |
| .. math:: \alpha^+ = \alpha(1 + k) | .. math:: z_v = -\alpha^+ dt + i\frac{1}{2} \alpha^+ \gamma = (1+k)z_s | .. math:: s = \frac{1}{2} \alpha (\alpha \sigma^2 - 2dt)     | .. math:: N_v = 1 - R \frac{\alpha^+}{z}             | .. math:: y_s = \frac{ (\alpha \sigma^2 - dt) }{\sqrt{2\sigma^2}}   |
| .. math:: x = \alpha^- - \beta     | .. math:: z_r = -\beta dt + i\frac{1}{2} \beta \gamma                  | .. math:: r = \frac{1}{2} \beta (\beta \sigma^2 - 2dt)       | .. math:: N_s = -2(1 - R\frac{\alpha}{y})            | .. math:: y_r = \frac{ (\beta \sigma^2 - dt) }{\sqrt{2\sigma^2}}    |
| .. math:: y = \alpha - \beta       |                                                                        |                                                              | .. math:: N_r = 2R\alpha^2\beta \frac{k^2}{xyz}      |                                                                     |
| .. math:: z = \alpha^+ - \beta     |                                                                        |                                                              |                                                      |                                                                     |
+------------------------------------+------------------------------------------------------------------------+--------------------------------------------------------------+------------------------------------------------------+---------------------------------------------------------------------+

where :math:`dt = T_i - T_h` is the shift in microseconds with respect to the Bragg position,
:math:`\alpha` and :math:`\beta` are the fast and slow neutron
decay constants respectively, and :math:`R` is a maxing coefficient that
relates to the moderator temperature.
:math:`\alpha` and :math:`R` are further modelled to depend on
wavelength and using the notation in the Fullprof manual (Ref [2]). The
refineable Ikeda-Carpenter parameters are Alpha0, Alpha1, Beta0 and
Kappa and these are defined as

.. math:: \alpha=1/(\mbox{Alpha0}+\lambda*\mbox{Alpha1})

.. math:: \beta = 1/\mbox{Beta0}

.. math:: R = \exp (-81.799/(\mbox{Kappa}*\lambda^2))

, where :math:`\lambda` is the neutron wavelength. *In general when
fitting a single peak it is not recommended to refine both Alpha0 and
Alpha1 at the same time since these two parameters will effectively be
100% correlated because the wavelength over a single peak is likely
effectively constant*. All parameters are constrained to be non-negative.

The pseudo-Voigt function is defined as a linear combination of a
Lorentzian and Gaussian and is a computational efficient way of
calculation a Voigt function. The Voigt parameters are related to the
pseudo-Voigt parameters through a relation (see Fullprof manual eq.
(3.16) which in revision July2001 is missing a power 1/5). It is the two
Voigt parameters which you can refine with this peakshape function:
SigmaSquared (for the Gaussian part) and Gamma (for the Lorentzian
part). Notice the Voigt Gaussian FWHM=SigmaSquared\*8\*ln(2) and the
Voigt Lorentzian FWHM=Gamma.

For information about how to create instrument specific values for the
parameters of this fitting function see
:ref:`CreateIkedaCarpenterParameters <CreateIkedaCarpenterParameters>`.

The implementation of the IkedaCarpenterPV peakshape function here
follows the analytical expression for this function as presented in Panels 13-17
of Ref[3].

References:

#. S. Ikeda and J. M. Carpenter, `Nuclear Inst. and Meth. in Phys. Res.
   A239, 536 (1985) <http://dx.doi.org/10.1016/0168-9002(85)90033-6>`_
#. Fullprof manual, see http://www.ill.eu/sites/fullprof/
#. J. Rodriguez-Carvajal, `Using FullProf to analyze Time of Flight
   Neutron Powder Diffraction data <http://mill2.chem.ucl.ac.uk/ccp/web-mirrors/plotr/Tutorials&Documents/TOF_FullProf.pdf>`_

The figure below illustrate this peakshape function fitted to a TOF
peak:

.. figure:: /images/IkedaCarpenterPVwithBackground.png
   :alt: IkedaCarpenterPVwithBackground.png

.. attributes::

.. properties::

.. categories::

.. sourcelink::

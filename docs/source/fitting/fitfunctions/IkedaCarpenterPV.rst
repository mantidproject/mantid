.. _func-IkedaCarpenterPV:

================
IkedaCarpenterPV
================

.. index:: IkedaCarpenterPV

Description
-----------

This peakshape function is designed to be used to fit time-of-flight
peaks. In particular this function is the convolution of the
Ikeda-Carpender function, which aims to model the neutron pulse shape
from a moderator, and a pseudo-Voigt that model any broading to the peak
due to sample properties etc.

The Ikeda-Carpender function is (Ref [1])

.. math:: \frac{\alpha}{2} \left\{ (1-R)*(\alpha t)^2e^{-\alpha t} + 2R\frac{\alpha^2\beta}{(\alpha-\beta)^3} \right\}

where :math:`\alpha` and :math:`\beta` are the fast and slow neutron
decay constants respectively, :math:`R` a maxing coefficient that
relates to the moderator temperature and :math:`t` is time.
:math:`\alpha` and :math:`R` are further modelled to depend on
wavelength and using the notation in the Fullprof manual (Ref [2]) the
refineable Ikeda-Carpender parameters are Alpha0, Alpha1, Beta0 and
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
`CreateIkedaCarpenterParameters <http://www.mantidproject.org/CreateIkedaCarpenterParameters>`_.

The implementation of the IkedaCarpenterPV peakshape function here
follows the analytical expression for this function as presented in the
Fullprof manual, see Ref[2].

References:

#. S. Ikeda and J. M. Carpenter, `Nuclear Inst. and Meth. in Phys. Res.
   A239, 536 (1985) <http://dx.doi.org/10.1016/0168-9002(85)90033-6>`_
#. Fullprof manual, see http://www.ill.eu/sites/fullprof/

The figure below illustrate this peakshape function fitted to a TOF
peak:

.. figure:: /images/IkedaCarpenterPVwithBackground.png
   :alt: IkedaCarpenterPVwithBackground.png

.. attributes::

.. properties::

.. categories::

.. sourcelink::

.. _func-GramCharlierComptonProfile:

==========================
GramCharlierComptonProfile
==========================

.. index:: GramCharlierComptonProfile

Description
-----------

The GramCharlierComptonProfile function calculates the Compton profile of a nucleus using a
Gram-Charlier approximation convoluted with an instrument resolution function.
The Gram-Charlier expansion of the Neutron Compton profile, :math:`J(y)` is given by [1] as an
expansion of Hermite polynomials,

.. math::
    J(y) = \frac{e^{-y^2/2\sigma^2}}{\sqrt{2\pi}\sigma}\left[ 1+ \sum_{n=2}^{\infty}\frac{a_n}{2^{2n}n!}H_{2n}\left(\frac{y}{\sqrt{2}\sigma}\right)\right]\label{a}

where, :math:`\sigma` is the standard deviation (Gaussian width parameter), :math:`a_n` the hermite coefficients and :math:`H_n` the Hermite polynomial terms.
As well as the even polynomial terms, a third order factor is included of the form,

.. math::
    \frac{A}{\sqrt{2\pi} \sigma} \times FSE \times \exp(-z^2) \times H_3 (z) \label{b}

where :math:`z=y/\sqrt{2\pi\sigma^2}` and :math:`FSE` is an input ampltiude scaling parameter. The Hermite coefficients, :math:`a_n`,
are supplied to the function in the parameters :math:`C_0`, :math:`C_2` and :math:`C_4`. The attribute HermiteCoeffs may be used
to determine which polynomial terms are active, e.g "1 0 1" will cause :math:`C_0` and :math:`C_4` to be active.


The instrument resolution, :math:`R_M`, is approximated by a :ref:`Voigt <func-Voigt>` function.

.. attributes::

.. properties::

References
----------
[1] Pantalei C, Pietropaolo A, Senesi R, Imberti S, Andreani C, Mayers J, et al.
Proton Momentum Distribution of Liquid Water from Room Temperature to the Supercritical Phase.
Phys Rev Lett 2008;100. https://doi.org/10.1103/physrevlett.100.177801.

.. categories::

.. sourcelink::

.. _func-GaussianComptonProfile:

======================
GaussianComptonProfile
======================

.. index:: GaussianComptonProfile

Description
-----------

The GaussianComptonProfile function describes the Compton profile of a nucleus using a
Gaussian approximation convoluted with an instrument resolution function,
that is approximated by a Voigt function. The function approximates the Count rate, :math:`C(t)` as [1],

.. math::
    C(t) = \left[\frac{E_0I(E_0)}{q}\right](t) A_m J_M(y_M)\otimes R_M(t)  \label{a}

for the given mass, M, :math:`J_M` is approximated using a Gaussian and :math:`R_M` is the resolution function.
In Equation :math:`\ref{a}`, the term :math:`A_M` is proportional to the scattering intensity and :math:`\left[\frac{E_0I(E_0)}{q}\right](t)`
depends on the incident neutron spectrum, which are both obtained the input workspace to the fit.

The Gaussian approximation, :math:`J_M`, takes two input parameters,

-  Width: :math:`\sigma`
-  Intensity: :math:`I`

The instrument resolution, :math:`R_M`, is approximated by a Voigt function using the :ref:`VesuvioResolution <algm-VesuvioResolution>`
function.

.. attributes::

.. properties::

References
----------
[1] Mayers J, Abdul-Redah T. The measurement of anomalous neutron inelastic cross-sections at electronvolt energy transfers.
J Phys: Condens Matter 2004;16:4811–32. https://doi.org/10.1088/0953-8984/16/28/005

.. categories::

.. sourcelink::

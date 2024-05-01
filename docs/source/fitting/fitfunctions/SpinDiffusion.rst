.. _func-SpinDiffusion:

=============
SpinDiffusion
=============

.. index:: SpinDiffusion

Description
-----------

[Get description from Adam] [1]_. The data fitted using this fit function is assumed to be in units of Gauss.

.. math::
    \lambda(B) &= \frac{A^2}{4} J(\omega) \\
    J(\omega) &= 2 \int_{0}^{+\infty} S(t)\cos(\omega t) \\
    S(t) &= \prod_{i=1}^{3} \exp(-2 D_{i} t) I_{0}(2 D_{i} t) \\
    \omega &= 2 \pi f = \gamma_{\mu} B

where:

- :math:`I_{0}(x)` is the zeroth order modified Bessel function.
- :math:`\omega` is the angular momentum (:math:`MHz`).
- :math:`\gamma_{\mu}` is the Muon gyromagnetic ratio (:math:`2 \pi \times 0.001356 MHz/G`).
- :math:`S(t)` is the autocorrelation function, represented by an anisotropic random walk.
- :math:`J(\omega)` is the spectral density (:math:`MHz^{-1}`). It is the Fourier Transform of :math:`S(t)`.
- :math:`A` is a parameter to be fitted.
- :math:`D_{i}` are the fast and slow rate dipolar terms. These are also fitting parameters.

Systems of different dimensionality :math:`d` can simply be represented in terms of fast and slow rates :math:`D_{\parallel}` and :math:`D_{\perp}`:

.. math::
    D_{1} = D_{\parallel},                D_{2}, D_{3} = D_{\perp}   (d=1)
    D_{1}, D_{2} = D_{\parallel},         D_{3} = D_{\perp}          (d=2)
    D_{1}, D_{2}, D_{3} = D_{\parallel}                              (d=3)

.. attributes::

.. properties::

References
----------

.. [1] Blundell, Stephen J., and others (eds), Muon Spectroscopy: An Introduction (Oxford, 2021; online edn, Oxford Academic, 23 June 2022), https://doi.org/10.1093/oso/9780198858959.001.0001, accessed 17 Apr. 2024.

.. categories::

.. sourcelink::

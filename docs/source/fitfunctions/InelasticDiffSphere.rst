.. _func-InelasticDiffSphere:

===================
InelasticDiffSphere
===================

.. index:: InelasticDiffSphere

Description
-----------

This fitting function models the inelastic part of the dynamics structure factor
of a particle undergoing continuous diffusion but confined to a spherical volume,
:ref:`DiffSphere <func-DiffSphere>`.

.. math::

   S(Q,E\equiv \hbar \omega) = \frac{1}{\pi} \sum_{l=1}^{N-1} (2l+1) A_{n,l} (Q\cdot R) \frac{x_{n,l}^2 D/R^2}{[x_{n,l}^2 D/R^2]^21+\omega^2}

.. math::

   A_{n,l} = \frac{6x_{n,l}^2}{x_{n,l}^2-l(l+1)} [\frac{QRj_{l+1}(QR) - lj_l(QR)}{(QR)^2 - x_{n,l}^2}]^2

Because of the spherical symmetry of the problem, the structure factor
is expressed in terms of the :math:`j_l(z)`
`spherical Bessel functions <http://mathworld.wolfram.com/SphericalBesselFunctionoftheFirstKind.html>`__.

The value of the momentum transfer can be obtained either through
attribute :math:`Q`, or can be calculated from the input workspace
using attribute  :math:`WorkspaceIndex`. The value calculated
using the workspace is used whenever attibute :math:`Q` is set empty.

.. attributes::

:math:`Q` (double, default=1.0) Momentum transfer -
:math:`WorkspaceIndex` (integer, default=0)

.. properties::

.. categories::

.. sourcelink::

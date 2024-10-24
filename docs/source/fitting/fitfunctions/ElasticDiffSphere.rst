.. _func-ElasticDiffSphere:

=================
ElasticDiffSphere
=================

.. index:: ElasticDiffSphere

Description
-----------

This fitting function models the elastic part of the dynamics structure factor
of a particle undergoing continuous diffusion but confined to a spherical volume,
:ref:`ElasticDiffSphere <func-ElasticDiffSphere>`.

.. math::

   S(Q,E\equiv \hbar \omega) = (3 \frac{j_1(QR)}{QR})^2(Q\cdot R) \delta (\omega)

Because of the spherical symmetry of the problem, the structure factor
is expressed in terms of the :math:`j_l(z)`
`spherical Bessel functions <http://mathworld.wolfram.com/SphericalBesselFunctionoftheFirstKind.html>`__.

.. attributes::

:math:`Q` (double, default=1.0) Momentum transfer

.. properties::

.. categories::

.. sourcelink::

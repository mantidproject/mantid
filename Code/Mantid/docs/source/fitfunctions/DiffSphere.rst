.. _func-DiffSphere:

==========
DiffSphere
==========

.. index:: DiffSphere

Description
-----------

Summary
-------

This fitting function models the dynamics structure factor of a particle
undergoing continuous diffusion but confined to a spherical volume.
According to Volino and Dianoux
`1 <http://apps.webofknowledge.com/InboundService.do?SID=4Bayo9ujffV3CUc9Qx8&product=WOS&UT=A1980KQ74800002&SrcApp=EndNote&DestFail=http%3A%2F%2Fwww.webofknowledge.com&Init=Yes&action=retrieve&Func=Frame&customersID=ResearchSoft&SrcAuth=ResearchSoft&IsProductCode=Yes&mode=FullRecord>`__,

.. math::

   S(Q,E\equiv \hbar \omega) = A_{0,0}(Q\cdot R) \delta (\omega) + \frac{1}{\pi} \sum_{l=1}^{N-1} (2l+1) A_{n,l} (Q\cdot R) \frac{x_{n,l}^2 D/R^2}{[x_{n,l}^2 D/R^2]^21+\omega^2}

.. math::

   A_{n,l} = \frac{6x_{n,l}^2}{x_{n,l}^2-l(l+1)} [\frac{QRj_{l+1}(QR) - lj_l(QR)}{(QR)^2 - x_{n,l}^2}]^2

Because of the spherical symmetry of the problem, the structure factor
is expressed in terms of the :math:`j_l(z)` spherical Bessel functions.
Furthermore, the requirement that no particle flux can escape the sphere
leads to the following boundary
condition\ `2 <http://apps.webofknowledge.com/InboundService.do?SID=4Bayo9ujffV3CUc9Qx8&product=WOS&UT=A1980KQ74800002&SrcApp=EndNote&DestFail=http%3A%2F%2Fwww.webofknowledge.com&Init=Yes&action=retrieve&Func=Frame&customersID=ResearchSoft&SrcAuth=ResearchSoft&IsProductCode=Yes&mode=FullRecord>`__:

.. math::

   \frac{d}{dr}j_l(rx_{n,l}/R)|_{r=R}=0 \,\,\,\, \forall l

The roots of this set of equations are the numerical coefficients
:math:`x_{n,l}`.

The fit function DiffSphere has an elastic part, modelled by fitting
function ElasticDiffSphere and an inelastic part, modelled by
InelasticDiffSphere.

When using InelasticDiffSphere, he value of Q can be obained either though the Q
attribute or can be calucated from the input workspace using the WorkspaceIndex
property. The value calculated using the workspace is used whenever the Q
attibute is empty.

.. attributes::

.. properties::

.. categories::

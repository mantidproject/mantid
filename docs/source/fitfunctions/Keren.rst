.. _func-Keren:

============
Keren
============

.. index:: Keren

Description
-----------

Keren's generalization of the Abragam relaxation function to a longitudinal field,
for fitting the time-dependent muon polarization.

The function is derived in `*Phys Rev B, vol. 50, 14, 10039-42 (1994)* <http://dx.doi.org/10.1103/PhysRevB.50.10039>`_ and is given by

.. math:: P_z(t) = A\exp\left[-\Gamma(t)t\right]

where the relaxation rate :math:`\Gamma(t)` is

.. math:: \Gamma(t)t = 2\Delta^2 \frac{\left\{\left(\omega_L^2 + \nu^2\right)\nu t + \left(\omega_L^2-\nu^2\right)\left(1-e^{-\nu t}\cos(\omega_L t)\right) - 2\nu\omega_L e^{-\nu t}\sin(\omega_L t)\right\}}{\left(\omega_L^2 + \nu^2\right)^2}

:math:`A = P_z(0)` is the polarization at time zero, :math:`\nu` is the fluctuation rate
(inverse correlation time), :math:`\Delta` is the distribution width of the local fields 
and :math:`\omega_L` is the Larmor frequency (longitudinal field times muon gyromagnetic ratio).

.. attributes::

.. properties::

.. categories::

.. sourcelink::

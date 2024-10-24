.. _func-MuoniumDecouplingCurve:

==============================
Muonium-style Decoupling Curve
==============================

.. index:: MuoniumDecouplingCurve

Description
-----------

A Muonium-style decoupling curve is defined as:

.. math:: y = A_R\frac{0.5+(\frac{x}{B_0})^2}{1+(\frac{x}{B_0})^2}+A_\mathrm{BG}

where

-  :math:`A_R` - the repolarising asymmetry
-  :math:`B_0` - the decoupling field
-  :math:`A_\mathrm{BG}` - the background asymmetry

The function models the repolarisation of isotropic muonium. The x axis should be the magnetic field, typically scanned from 0 to a few kGauss. The y axis is the asymmetry.

Examples
--------

This has been used in examining the influence of strong magnetic field on the depolarizations of Muons[1] and in investigating the Paschen-Back Effect as a means of detecting Muonium[2].


.. attributes::

.. properties::

References
----------
[1] Orearm J. and Harris, G. and Bierman, E. (1957) Influence of Strong Magnetic Field on Depolarization of Muons, Phys. Rev., Vol 107 Issue 1, 322-323 `doi: 10.1103/PhysRev.107.322 <https://doi.org/10.1103/PhysRev.107.322>`
[2] Ferrell, R. A. and Chaos, F. (1957) Paschen-Back Effect as a Means of Detecting Muonium, Phys. Rev., Vol 107 Issue 5, 1322-1323 `doi: 10.1103/PhysRev.107.1322 <https://doi.org/10.1103/PhysRev.107.1322>`

.. categories::

.. sourcelink::

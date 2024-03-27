.. _func-TeixeiraWaterIqt:

================
TeixeiraWaterIqt
================

.. index:: TeixeiraWaterIqt

Description
-----------

It models the intermediate scattering function for the inelastic spectra of water molecules, based on Teixeira's model [1]_.

.. math::
    F_s(Q,t) &= A  R(Q,t)  T(Q,t) \\
    T(Q,t) &= e^{- \Gamma t} \\
    R(Q,t) &= j_{0}^{2}(Qa)+3j_{1}^{2}(Qa) e^{-t/{3\tau_1}}+5j_{2}^{2}(Qa) e^{-t/\tau_1}

where:

- :math:`j_{n}(x)` are the spherical bessel functions of order *n*.
- :math:`A` is the amplitude.
- :math:`\Gamma` is the spectrum linewidth.
- :math:`\tau_{1}` is the relaxation time associated with rotation diffusion.
- :math:`Q` is the elastic momentum transfer, :math:`0.4\AA^{-1}` by default, but should be set to a correct value prior to fitting.
- :math:`a` is the molecular radius of rotation, set by default to :math:`0.98\AA`.

Units of :math:`a` are inverse units of :math:`Q`.

Units of :math:`\Gamma` inverse units of :math:`t`. After fitting, if scaled by correct :math:`\hbar`, units are :math:`meV` if units of :math:`\tau_1` are *ps*.
Alternatively, units of :math:`\Gamma` are :math:`\mu eV` if units of
:math:`\tau_{1}` are *ns*.

.. attributes::

.. properties::

References
----------

.. [1] J. Teixeira, M.-C. Bellissent-Funel, S. H. Chen, and A. J. Dianoux. `Phys. Rev. A, 31:1913–1917 <http://dx.doi.org/10.1103/PhysRevA.31.1913>`__

.. categories::

.. sourcelink::

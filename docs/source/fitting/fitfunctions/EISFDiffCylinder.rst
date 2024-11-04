.. _func-EISFDiffCylinder:

================
EISFDiffCylinder
================

.. index:: EISFDiffCylinder

Description
-----------

This fitting function models the diffusion of a particle confined in a cylinder of radius :math:`R` and
length :math:`L` [1]_. :math:`A_0(Q_z)` implements diffusion along the cylinder axis.

.. math::
    A_0(Q_z) = (j_0(Q L \cos(\theta)))^2 = (\frac{\sin(Q L \cos(\theta))}{Q L \cos(\theta)})^2

:math:`B_0^0(Q_{\perp})` implements diffusion perpendicular to the cylinder axis.

.. math::
    B_0^0(Q_{\perp}) = (2 \frac{J_1(Q R \sin(\theta))}{Q R \sin(\theta)} )^2

Both diffusions are assumed to be decoupled. Finally, the integration
in :math:`\theta` implements a powder average
(`spherical Bessel functions <http://mathworld.wolfram.com/SphericalBesselFunctionoftheFirstKind.html>`__).

:math:`R` and :math:`L` units are inverse of :math:`Q` units.


References
----------

.. [1] A. J. Dianoux et al. `Mol. Phys. 46:1129-37, 1982 <https://doi.org/10.1080/00268978200101121>`__.

Usage
-----

**Example - fit of Q-dependence:**

.. testcode:: QdependenceFit

    q =  [0.3, 0.5, 0.7, 0.9, 1.1, 1.3, 1.5, 1.7, 1.9]
    # A=1.0, R=3.5, L=1.7
    eisf = [0.8327688, 0.60447105, 0.36837178, 0.18538092, 0.07615478,
            0.02660468, 0.00973061, 0.00461192, 0.00222067]
    w = CreateWorkspace(q, eisf, NSpec=1)
    results = Fit('name=EISFDiffCylinder,A=1,R=2.0,L=1,constraints=(0.01<R,0.01<L),ties=(A=1)', w, WorkspaceIndex=0)
    print(results.Function)

Output:

.. testoutput:: QdependenceFit

    name=EISFDiffCylinder,A=1,R=3.11497,L=1.68724,constraints=(0.01<R,0.01<L),ties=(A=1)

.. properties::

.. categories::

.. sourcelink::

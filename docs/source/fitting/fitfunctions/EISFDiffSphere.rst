.. _func-EISFDiffSphere:

==============
EISFDiffSphere
==============

.. index:: EISFDiffSphere

Description
-----------

This fitting function models the elastic incoherent intensity of a particle
undergoing continuous diffusion but confined to a spherical volume [1]_,
:ref:`EISFDiffSphere <func-EISFDiffSphere>`.

.. math::

   EISF(Q) = (3 \frac{j_1(QR)}{QR})^2(Q\cdot R)

:math:`R` units are inverse of :math:`Q` units. Because of the spherical
symmetry of the problem, the structure factor is expressed in terms of the
:math:`j_l(z)` `spherical Bessel functions <http://mathworld.wolfram.com/SphericalBesselFunctionoftheFirstKind.html>`__.

Related functions:
- :ref:`ElasticDiffSphere <func-ElasticDiffSphere>`
- :ref:`InelasticDiffSphere <func-InelasticDiffSphere>`
- :ref:`DiffSphere <func-DiffSphere>`

References
----------

.. [1] F. Volino and A. J. Dianoux, `Molecular Physics 41(2):271-279 <https://doi.org/10.1080/00268978000102761>`__.

Usage
-----

**Example - fit of Q-dependence:**

.. testcode:: QdependenceFit

    q =  [0.3, 0.5, 0.7, 0.9, 1.1, 1.3, 1.5, 1.7, 1.9]
    # A=2.0 and R=0.5
    eisf = [1.9910173378712506, 1.9751335160492454,
            1.9515113999365767, 1.9203919612905054,
            1.8820909475511156, 1.8369942556092735,
            1.7855523076069868, 1.7282735296640419,
            1.6657170499144847]
    w = CreateWorkspace(q, eisf, NSpec=1)
    results = Fit('name=EISFDiffSphere', w, WorkspaceIndex=0)
    print(results.Function)

Output:

.. testoutput:: QdependenceFit

    name=EISFDiffSphere,A=2,R=0.5

.. properties::

.. categories::

.. sourcelink::

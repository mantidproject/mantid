.. _func-ElasticDiffRotDiscreteCircle:

============================
ElasticDiffRotDiscreteCircle
============================

.. index:: ElasticDiffRotDiscreteCircle

Description
-----------

Summary
-------

This fitting function models the *elastic part* of the dynamics structure factor
of a particle undergoing discrete jumps on N-sites evenly distributed in a circle.
The particle can only jump to neighboring sites. This is the most common
type of discrete rotational diffusion in a circle.

.. math::

   S(Q,E) \equiv = \int e^{-iEt/\hbar} I(Q,t) dt = A_0(Q,r) \delta (E)

.. math::

   A_0(Q,r) = \frac{1}{N} \sum_{k=1}^{N} j_0( 2 Q r sin(\frac{\pi k}{N}) )

This function makes up the elastic part of :ref:`DiffRotDiscreteCircle <func-DiffRotDiscreteCircle>`.

.. attributes::

:math:`N` (integer, default=3) number of sites -
:math:`Q` (double, default=0.5) Momentum transfer

.. properties::

.. categories::

.. sourcelink::

.. _func-InelasticDiffRotDiscreteCircle:

==============================
InelasticDiffRotDiscreteCircle
==============================

.. index:: InelasticDiffRotDiscreteCircle

Description
-----------

Summary
-------

This fitting function models the *inelastic part* of the dynamics structure factor
of a particle undergoing discrete jumps on N-sites evenly distributed in a circle.
The particle can only jump to neighboring sites. This is the most common
type of discrete rotational diffusion in a circle.

.. math::

   S(Q,E) \equiv = \int e^{-iEt/\hbar} I(Q,t) dt = \frac{1}{\pi} \sum_{l=1}^{N-1} A_l (Q,r) \frac{\hbar \tau_l^{-1}}{(\hbar \tau_l^{-1})^2+E^2}

.. math::

   A_l(Q,r) = \frac{1}{N} \sum_{k=1}^{N} j_0( 2 Q r sin(\frac{\pi k}{N}) ) cos(\frac{2\pi lk}{N})

.. math::

   \tau_l^{-1} = 4 \tau^{-1} sin^2(\frac{\pi l}{N})

with h = 4.135665616 meV ps.

This function makes up the inelastic part of :ref:`DiffRotDiscreteCircle <func-DiffRotDiscreteCircle>`.

.. attributes::

:math:`N` (integer, default=3) number of sites -
:math:`Q` (double, default=0.5) Momentum transfer -
:math:`WorkspaceIndex` (integer, default=0)

.. properties::

.. categories::

.. sourcelink::

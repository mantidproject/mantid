.. _func-DiffRotDiscreteCircle:

=====================
DiffRotDiscreteCircle
=====================

.. index:: DiffRotDiscreteCircle

Description
-----------

Summary
-------

This fitting function models the dynamics structure factor of a particle
undergoing discrete jumps on N-sites evenly distributed in a circle. The
particle can only jump to neighboring sites. This is the most common
type of discrete rotational diffusion in a circle.

Markov model for jumps between neighboring sites:

.. math::

   \frac{d}{dt} p_j(t) = \frac{1}{\tau} [p_{j-1}(t) -2 p_j(t) + p_{j+1}(t)]

The Decay fitting parameter :math:`\tau` is the inverse of the
transition rate. This, along with the circle radius :math:`r`, conform
the two fundamental fitting parameters of the structure factor
:math:`S(Q,E)`:

.. math:: 

   S(Q,E) \equiv = \int e^{-iEt/\hbar} I(Q,t) dt = A_0(Q,r) \delta (E) + \frac{1}{\pi} \sum_{l=1}^{N-1} A_l (Q,r) \frac{\hbar \tau_l^{-1}}{(\hbar \tau_l^{-1})^2+E^2}

.. math::

   A_l(Q,r) = \frac{1}{N} \sum_{k=1}^{N} j_0( 2 Q r sin(\frac{\pi k}{N}) ) cos(\frac{2\pi lk}{N})

.. math::

   \tau_l^{-1} = 4 \tau^{-1} sin^2(\frac{\pi l}{N})

The transition rate, expressed in units of energy is :math:`h\tau^{-1}`,
with h = 4.135665616 meV THz.

When using InelasticDiffRotDiscreteCircle, he value of Q can be obained either
though the Q attribute or can be calucated from the input workspace using the
WorkspaceIndex property. The value calculated using the workspace is used
whenever the Q attibute is empty.

Example: Methyl Rotations
-------------------------

Methyl Rotations can be modelled setting N=3. In this case, the
inelastic part reduces to a single Lorentzian:

.. math::

   S(Q,E) = A_0(Q,r) \delta (E) + \frac{2}{\pi} A_1 (Q,r) \frac{3 \hbar \tau^{-1}}{(3 \hbar \tau^{-1})^2+E^2}

If, alternatively, one models these dynamics using the
`Lorentzian <Lorentzian>`__ function provided in Mantid:

.. math::

  S(Q,E) = A \delta (\omega) + \frac{B}{\pi} \left( \frac{\frac{\Gamma}{2}}{(\frac{\Gamma}{2})^2 + (\hbar\omega)^2}\right)

Then:

.. math::

   B = \frac{1}{\pi}h A_1

.. math::

   \Gamma = \frac{3}{\pi} h\tau^{-1} = 3.949269754 meV\cdot THz\cdot \tau^{-1}

.. attributes::

.. properties::

.. categories::

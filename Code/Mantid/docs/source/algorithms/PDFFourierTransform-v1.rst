.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm PDFFourierTransform transforms :math:`S(Q)`,
:math:`S(Q)-1`, or :math:`Q[S(Q)-1]` (as a fuction of MomentumTransfer
or dSpacing) to a PDF (pair distribution function) as described below.

The input Workspace can have the unit in d-space of Q-space. The
algorithm itself is able to identify the unit. The allowed unit are
MomentumTransfer and d-spacing.

**Note:** All other forms are calculated by transforming :math:`G(r)`.

Output Options
##############

G(r)
''''

:math:`G(r) = 4\pi r[\rho(r)-\rho_0] = \frac{2}{\pi} \int_{0}^{\infty} Q[S(Q)-1]\sin(Qr)dQ`

and in this algorithm, it is implemented as

:math:`G(r) =  \frac{2}{\pi} \sum_{Q_{min}}^{Q_{max}} Q[S(Q)-1]\sin(Qr) M(Q,Q_{max}) \Delta Q`

where :math:`M(Q,Q_{max})` is an optional filter function. If Filter
property is set (true) then

:math:`M(Q,Q_{max}) = \frac{\sin(\pi Q/Q_{max})}{\pi Q/Q_{max}}`

otherwise

:math:`M(Q,Q_{max}) = 1\,`

g(r)
''''

:math:`G(r) = 4 \pi \rho_0 r [g(r)-1]`

transforms to

:math:`g(r) = \frac{G(r)}{4 \pi \rho_0 r} + 1`

RDF(r)
''''''

:math:`RDF(r) = 4 \pi \rho_0 r^2 g(r)`

transforms to

:math:`RDF(r) = r G(r) + 4 \pi \rho_0 r^2`

.. categories::

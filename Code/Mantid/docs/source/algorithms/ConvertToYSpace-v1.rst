.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The final unit of the x-axis is changed to momentum (Y) space as defined
by

.. raw:: html

   <center>

:math:`Y = 0.2393\frac{M}{\epsilon_i^{0.1}}(\omega - \frac{q^2}{2M})`

.. raw:: html

   </center>

where :math:`M` is the mass in atomic mass units,
:math:`\displaystyle\epsilon` is the incident energy,
:math:`\displaystyle\omega` is the energy change and :math:`q` is
defined as :math:`\sqrt{(k_0^2 + k_1^2 - 2k_0k_1\cos(\theta))}`.

The TOF is used to calculate :math:`\displaystyle\epsilon_i` and the
:math:`\displaystyle1/\epsilon` dependence causes an increasing set of
TOF values to be mapped to a decreasing set of :math:`\displaystyle Y`
values. As a result the final :math:`Y`-space values are reversed to
give a workspace with monotonically increasing :math:`Y` values.

.. categories::

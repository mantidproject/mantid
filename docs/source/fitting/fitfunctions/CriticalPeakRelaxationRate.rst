.. _func-CriticalPeakRelaxationRate:

==========================
CriticalPeakRelaxationRate
==========================

.. index:: CriticalPeakRelaxationRate

Description
-----------

The Critical Peak Relexation Rate is defined as:

.. math:: y = \frac{B_1}{(|x - T_c|)^a} + B_1\Theta(x < T_c) + B_2\Theta(x >= T_c)

where:
- :math:`S_c` - Scaling
- :math:`T_c` - Critical temperature
- :math:`a` - Critical exponent
- :math:`B_1` - is a non-critical background when :math:`x < T_c`
- :math:`B_2` - is a non-critical background when :math:`x >= T_c`

When fitting users should set :math:`T_c` as the temperature at which the peak occurs. Users are also asked to supply two values for :math:`B_g`. The first should be the value of y when x is at it's minimum. The second should be the value of y when x is at its maximum, minus the first background value.


Examples
--------

An example of when this might be used is for examining the Chiral-like critical behaviour in antiferromagnet Cobalt Glycerolate[1] or in muon spin relaxation studies of critical fluctuations and diffusive spin dynamics in molecular magnets[2].


.. attributes::

.. properties::

References
----------
[1] Pratt, F.L, Baker, P.J., Blundell, S.J., Lancaster, T., Green, M.A., and Kurmoo, M. (2007). Chiral-Like Critical Behaviour in the Antiferromagnet Cobalt Glycerolate. Phys. Rev. Lett., Vol 99 Issue 1, 017202 `doi: 10.1103/PhysRevLett.99.017202 <https://doi.org/10.1103/PhysRevLett.99.017202>`_.
[2] Pratt, F. et al (2009) Muon spin relaxation studies of critical fluctuations and diffusive spin dynamics in molecular magnets. Physica B: Condensed Matter, Volume 404 Issues 5–7, pp585-589 `doi: 10.1016/j.physb.2008.11.123 <https://doi.org/10.1016/j.physb.2008.11.123>`_.

.. categories::

.. sourcelink::

.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The probability of neutron detection by each detector in the
`workspace <workspace>`__ is calculated from the neutrons' kinetic
energy, angle between their path and the detector axis, detector gas
pressure, radius and wall thickness. The detectors must be cylindrical
and their :sup:`3`\ He partial pressure, wall thickness and radius
stored in the input workspace, the first in atmospheres and the last two
in metres. That workspace then needs to be converted so that its
X-values are in `units <Unit_Factory>`__ of energy transfer, e.g. using
the :ref:`algm-ConvertUnits` algorithm.

To estimate the true number of neutrons that entered the detector the
counts in each bin are divided by the detector efficiency of that
detector at that energy.

The numbers of counts are then multiplied by the value of
:math:`k_i/k_f` for each bin. In that formula :math:`k_i` is the
wavenumber a neutron leaving the source (the same for all neutrons) and
:math:`k_f` is the wavenumber on hitting the detector (dependent on the
detector and energy bin). They're calculated, in angstrom\ :sup:`-1`, as

| :math:`k_i = \sqrt{\frac{E_i}{2.07212466}}`
| :math:`k_f = \sqrt{\frac{E_i - \Delta E}{2.07212466}}`

where :math:`E_i` and :math:`\Delta E` are energies in meV, the inital
neutron kinetic energy and the energy lost to the sample respectively.

Note: it is not possible to use this `algorithm <algorithm>`__ to
correct for the detector efficiency alone. One solution to this is to
divide the output algorithm by :math:`k_i/k_f` calculated as above.

.. categories::

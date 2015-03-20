.. _func-PawleyFunction:

===========
PawleyFunction
===========

.. index:: PawleyFunction

Description
-----------

The basic principle of fitting unit cell parameters to a complete powder diffraction pattern has been described by Pawley. Instead allowing each peak to have a freely selectable position, these positions are calculated as a result from the unit cell parameters. All other parameters of the peaks are refined independently. The implementation of the function differs from the method described in the paper in some points, for example the number of parameters can not change during the refinement (no reflections will be added or removed).

Since the function requires special setup (assignment of HKLs, selection of crystal system and profile function), there is an algorithm that can perform a Pawley-type fit with different input data. Please see the documentation of :ref:`algm-PawleyFit` for details on how to use it.

.. attributes::

.. properties::

.. categories::

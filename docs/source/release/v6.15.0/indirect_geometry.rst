=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------


Bugfixes
--------
- (`#40243 <https://github.com/mantidproject/mantid/pull/40243>`_) Fixed bug in `FilterEvents <algm-FilterEvents>` for ``CorrectionToSample="Indirect"``. The code now matches the equations in the documentation.


Algorithms
----------

New features
############
- (`#40399 <https://github.com/mantidproject/mantid/pull/40399>`_) Abins2D algorithm will now validate combined chopper settings
  (Chopper, IncidentEnergy, ChopperFrequency) for pychop-based
  instruments (MAPS, MARI, MERLIN) and prevent algorithm running if
  there would be no neutron transmission.

Bugfixes
############
- (`#40229 <https://github.com/mantidproject/mantid/pull/40229>`_) Improved Abins/Abins2D resolution function for ILL-LAGRANGE
  instrument. A minimum resolution of 0.4 meV is applied at low energy
  transfer for the Cu(220) monochromator: the switch between this
  constant resolution and the energy-dependent resolution was
  previously implemented too low in energy transfer, giving
  unreasonably sharp peaks below 30meV.
- (`#40240 <https://github.com/mantidproject/mantid/pull/40240>`_) Fixed incident energy limits of Abins2D instruments: MARI 1000 meV,
  PANTHER 181 meV, Ideal2D infinite

:ref:`Release 6.15.0 <v6.15.0>`

=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

Bugfixes
--------
- (`#40243 <https://github.com/mantidproject/mantid/pull/40243>`_) :ref:`FilterEvents <algm-FilterEvents>` had a bug when ``CorrectionToSample="Indirect"``. The code now matches the equations in the documentation.

Algorithms
----------

New features
############
- (`#40399 <https://github.com/mantidproject/mantid/pull/40399>`_) The :ref:`Abins2D <algm-Abins2D>` algorithm will now validate combined chopper settings (``Chopper``, ``IncidentEnergy``, ``ChopperFrequency``) for pychop-based instruments (MAPS, MARI, MERLIN) and will not run if there would be no neutron transmission.

Bugfixes
############
- (`#40229 <https://github.com/mantidproject/mantid/pull/40229>`_) Improved :ref:`Abins <algm-Abins>`/:ref:`Abins2D <algm-Abins2D>` resolution function for the ILL-LAGRANGE instrument. A minimum resolution of 0.4 meV is applied at low energy transfer for the Cu(220) monochromator: the switch between this constant resolution and the energy-dependent resolution was previously implemented too low in energy transfer, giving unreasonably sharp peaks below 30 meV.
- (`#40240 <https://github.com/mantidproject/mantid/pull/40240>`_) Updated energy limits of :ref:`Abins2D <algm-Abins2D>` instruments: MARI to 1000 meV and PANTHER to 181 meV. Ideal2D has been set to infinite (any choice would be arbitrary as this instrument is for "Ideal simulations").

:ref:`Release 6.15.0 <v6.15.0>`

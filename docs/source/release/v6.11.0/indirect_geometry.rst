=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------



Bugfixes
--------
- Fixed a bug on the Energy Transfer tab of the :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` interface where the first raw file would be loaded into the ADS but never deleted.
- Fixed a bug where the spectra numbering for reduced files from the :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` interface was not consistent for a summed file reduction compared to individual file reduction.
- Fixed a crash on the :ref:`interface-indirect-simulation` interface after clicking "Run" without loading data.


Algorithms
----------

New features
############


Bugfixes
############
- fix :ref:`CompareWorkspaces <algm-CompareWorkspaces-v1>` for relative differences of small values.

:ref:`Release 6.11.0 <v6.11.0>`
============
Muon Changes
============

.. contents:: Table of Contents
   :local:

Muon Analysis
-------------

Bugfixes
############
- Fixed a bug where the parameters for a UserFunction after a simultaneous fit were not being updated.

Algorithms
----------

Bugfixes
############
- Fixed an index error when loading data with three periods but only two histograms with the :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>` algorithm.
- Fixed a bug in the :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>` algorithm when loading a Nexus file with no grouping info. It now loads the grouping from the IDF.

:ref:`Release 6.9.0 <v6.9.0>`
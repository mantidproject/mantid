============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Frequency Domain Analysis
-------------------------

Bugfixes
############



Muon Analysis
-------------

Bugfixes
############
- Fixed a bug where the parameters for a UserFunction after a simultaneous fit were not being updated.


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

Bugfixes
############



ALC
---

Bugfixes
############



Elemental Analysis
------------------

Bugfixes
############



Algorithms
----------

Bugfixes
############
- Fixed an index error when loading data with three periods but only two histograms with the :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>` algorithm.
- Fixed the :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>` algorithm when loading a Nexus file with no grouping info so that it loads the grouping from the IDF.

:ref:`Release 6.9.0 <v6.9.0>`
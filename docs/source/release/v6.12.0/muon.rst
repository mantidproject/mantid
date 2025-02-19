============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Frequency Domain Analysis
-------------------------

Bugfixes
############
.. amalgamate:: Muon/FDA/Bugfixes


Muon Analysis
-------------

Bugfixes
############
.. amalgamate:: Muon/Muon_Analysis/Bugfixes


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

Bugfixes
############
.. amalgamate:: Muon/MA_FDA/Bugfixes


ALC
---

Bugfixes
############
.. amalgamate:: Muon/ALC/Bugfixes


Elemental Analysis
------------------

Bugfixes
############
.. amalgamate:: Muon/Elemental_Analysis/Bugfixes


Algorithms
----------

New features
############
- The :ref:`LoadMuonNexus v2<algm-LoadMuonNexus-v2>` algorithm no longer chooses a different Muon loader if appropriate. This functionality has been moved to a new algorithm version, :ref:`LoadMuonNexus v3<algm-LoadMuonNexus-v3>`.
- Users can also call the :ref:`Load <algm-Load>` algorithm directly which will also select the appropriate loader for them.

:ref:`Release 6.12.0 <v6.12.0>`

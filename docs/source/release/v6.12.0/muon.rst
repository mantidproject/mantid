============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Frequency Domain Analysis
-------------------------

Bugfixes
############
- The :ref:`Muon FDA interface <Frequency_Domain_Analysis-ref>` will no longer display an unnecessary error prompt after cancelling a ``MaxEnt`` calculation.


Muon Analysis
-------------

Bugfixes
############
- External plots will no longer be squashed by plot legends.


ALC
---

New features
############
- The :ref:`Muon ALC interface<MuonALC-ref>` can now import data from an externally generated matrix workspace (i.e. not previously exported from the Muon ALC interface).


Algorithms
----------

Bugfixes
############
- The :ref:`LoadMuonNexus v2<algm-LoadMuonNexus-v2>` algorithm no longer chooses a different Muon loader if appropriate. Users should call the :ref:`Load <algm-Load>` algorithm directly if they want the appropriate loader to be chosen for them.

:ref:`Release 6.12.0 <v6.12.0>`

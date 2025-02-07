============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Frequency Domain Analysis
-------------------------

Bugfixes
############
- Remove off-putting error prompt that appears when canceling a MaxEnt calculation in the :ref:`Muon FDA interface <Frequency_Domain_Analysis-ref>`


Muon Analysis
-------------

Bugfixes
############
- On Muon Interfaces, a legend containing a large string of text does not shrink the plot axes for tight layouts.


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

Bugfixes
############



ALC
---

New features
############
+- The :ref:`Muon ALC <MuonALC-ref>` interface can now import data from a matrix workspace which was externally generated (i.e. not previously exported from the Muon ALC interface).

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
- The :ref:`LoadMuonNexus v2<algm-LoadMuonNexus-v2>` algorithm no longer chooses a different Muon loader if appropriate. Users should call the :ref:`Load <algm-Load>` algorithm directly if they want the appropriate loader to be chosen for them.

:ref:`Release 6.12.0 <v6.12.0>`

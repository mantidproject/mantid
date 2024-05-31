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
- Fixed a bug when switching between "Normal" fitting and "TF Asymmetry" fitting modes.
- Fixed an unreliable error when loading a new group of runs after doing a simultaneous fit.
- Fixed a bug affecting the pairing and the difference tables of the groupings tab in the Muon Analysis Interface. The bug can crash Mantid if after deleting any rows other than the last one from the table, the `Alpha Guess` button or the `Group 1`, `Group 2` combo boxes are clicked.
- Fixed a bug when you click 'Load Current Run' for the MUSR instrument.
- Improved the warning message displayed when a file fails to load.
- Fixed an error that would occur during simultaneous fitting on the :ref:`Muon Analysis interface <Muon_Analysis-ref>`.
- Fixed a freeze in the Muon Analysis interface after deleting a workspace which is not loaded into the interface.
- Fixed a crash caused by unselecting all loaded data in the Grouping tab before adding a Fit Function.


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


:ref:`Release 6.10.0 <v6.10.0>`
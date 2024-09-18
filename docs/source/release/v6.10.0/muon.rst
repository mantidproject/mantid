============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Muon Analysis
-------------

Bugfixes
############

- Fixed a bug when switching between "Normal" fitting and "TF Asymmetry" fitting modes.
- Loading a new group of runs after doing a simultaneous fit no longer throws an unreliable error.
- :ref:`Muon Analysis interface <Muon_Analysis-ref>` no longer crashes when clicking `Alpha Guess`, `Group 1` or `Group2` after deleting any rows other than the last one from the difference table.
- Fixed a bug when you click `Load Current Run` for the MUSR instrument.
- Improved the warning message displayed when a file fails to load.
- Fixed an error that would occur during simultaneous fitting on the :ref:`Muon Analysis interface <Muon_Analysis-ref>`.
- Fixed a freeze in the :ref:`Muon Analysis interface <Muon_Analysis-ref>` after deleting a workspace which is not loaded into the interface.
- Fixed a crash caused by unselecting all loaded data in the Grouping tab before adding a Fit Function.

:ref:`Release 6.10.0 <v6.10.0>`

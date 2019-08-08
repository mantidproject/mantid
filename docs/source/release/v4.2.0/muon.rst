============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

New
###


Improvements
############
  * Improved the speed of plotting during sequential fits.

Removed
#######


Bug Fixes
#########
  * Fixed an issue where changeing the normalisation on a plot with autoscale disabled throws an exception.
  * Fixed an issue where warnings about adding workspaces to workspace groups multiple times were appearing in the log.

Known Issues
############

Algorithms
----------

Improvements
############

- Improve the handling of :ref:`LoadPSIMuonBin<algm-LoadPSIMuonBin-v1>` where a poor date is provided.

Interfaces
----------

Muon Analysis 2
###############

- When loading PSI data if the groups given are poorly stored in the file, it should now produce unique names in the grouping tab for groups.
- When switching between data sets groups selected to fit are remembered.
- The FFT tab now uses the group pair selection to make a guess at the users selection for workspaces.

Algorithms
----------

Improvements
############

- :ref:`LoadPSIMuonBin <algm-LoadPSIMuonBin>` has been improved to correctly load data other than data from Dolly at the SmuS/PSI.

:ref:`Release 4.2.0 <v4.2.0>`

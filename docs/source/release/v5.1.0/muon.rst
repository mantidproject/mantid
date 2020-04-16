============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Muon Analysis 2
################

New Features
------------
- The plotting logic within the Muon Analysis 2 GUI has updated to prevent all the workspaces from a
  fit being plotted at the same time. Instead, the choice of which fit workspace to plot can be made in
  fitting and sequential fitting tabs.
- When the user switches to the fitting tab, the workspace present in the fit display box is plotted.
  To switch back to a view of all the data, the user can switch to the home, grouping or phase table tabs.
- The sequential fitting table now allows multiple selections to be made.

:ref:`Release 5.1.0 <v5.1.0>`
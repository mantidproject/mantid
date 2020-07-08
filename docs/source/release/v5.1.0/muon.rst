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
- Addition of an external plotting button to the Muon Analysis 2 GUI.
  This allows the user to create a standalone Workbench (or MantidPlot) plot of the displayed data.
  The user may then perform standard operations on the plot, e.g drag and drop workspaces onto the figure.
- The loading in the Muon and Frequency domain interfaces has been sped up by reducing the number of calls made to algorithms.
  On average, this should result in a 50% decrease in load times. This reduction in the number of algorithms also improves
  the clarity of the workspace history, as the number of algorithms present in the history is now reduced.
- On the fitting tab, only one fit object (fit output and input workspaces) will be shown at a time.
- Addition of background correction algorithm (PSIBackgroundCorrection) to remove the background present in
  PSI bin data loaded using LoadPSIMuonBin.
- Addition of a LoadMuonNexusV2 algorithm to load the new Muon V2 files, see :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>`.
- Updated rounding for time zero and first good data to be 3 decimal places.
- Added double pulse analysis, see :ref:`Muon home tab <muon_home_tab-ref>`.

Improvements
-------------
- Improved the plotting code the Muon Analysis interface, with a significant amount of redundant code removed.
  This code improvement will lead to more maintainable code, which will be more stable and
  easier to add new functionality in the future.
- Improved the maintainability of the fitting code in the Muon Analysis interface by removing redundant and duplicated code.
  These changes will make the addition of new functionality in the future easier.
- The plot guess option within the fitting tab will now update when a parameter is changed.

Bug fixes
---------
- Fixed an issue where ties set in Muon Analysis were not being respected.

:ref:`Release 5.1.0 <v5.1.0>`
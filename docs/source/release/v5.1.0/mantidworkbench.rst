=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

Improvements
############

.. figure:: ../../images/Plot1DSelectionDialog5-1.png
   :align: right

- The plot selection dialog now correctly shows the full range of valid spectra to plot, not just the min to max range.
- Tile plots are now reloaded correctly by project recovery.


Bugfixes
########

- Fixed a bug where setting columns to Y error in table workspaces wasn't working. The links between the Y error and Y columns weren't being set up properly.
- Fixed a crash when you selected a spectra to plot that was not present in a workspace.

:ref:`Release 5.1.0 <v5.1.0>`

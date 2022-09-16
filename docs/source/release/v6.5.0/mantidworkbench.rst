========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------



Bugfixes
--------
- Scaling for waterfall plots has been fixed for scaling in log scale plots.
- Opening multiple instances of Mantid in quick succession will no longer cause errors in the shared settings.
- A large number of crashes have been fixed caused by opening an Algorithm Dialog and populating it with a WorkspaceGroup.
- It is no longer possible to crash mantid when opening an interface's help documentation for the second time.
- The ISIS logos have been updated to match the UKRI rebrand.
- Fixed a change in behaviour after upgrading Qt, where floating point numbers in the matrix data view were not displayed unless the column was wide enough to show all digits.


InstrumentViewer
----------------

New features
############


Bugfixes
############
- The instrument view startup time has been improved by avoid checking for masking if it is not present.


SliceViewer
-----------

New features
############


Bugfixes
############
- Fix for colorbar when using log scale that causes slice viewer to throw exception with all masked slice.
- The colorbar now remains autoscaled when switching between the different normalisation options.
- SliceViewer will now support `W_MATRIX` log for nonorthogonal axes with MDHistoWorkspaces. This prevents a bug related where basis vectors are used to calculate the projection.
- Fixes a bug in the SliceViewer where a workspace is replaced by an algorithm with more than one output workspace.
- For 3d+ datasets, SliceViewer will now automatically open to the zero-slice if present.
- Ensure that all nan/inf and all zero slices work while using log-scaling.


:ref:`Release 6.5.0 <v6.5.0>`
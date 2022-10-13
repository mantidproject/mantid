========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

Bugfixes
--------
- Log-scaling has been fixed in waterfall plots.
- Opening multiple instances of Mantid in quick succession will no longer cause errors in the shared settings.
- A large number of crashes, caused by opening an Algorithm Dialog and populating it with a :ref:`WorkspaceGroup <WorkspaceGroup>`, have been fixed.
- It is no longer possible to crash mantid when opening an interface's help documentation for the second time.
- The ISIS logos have been updated to match the UKRI rebrand.

.. image:: ../../../../images/ISIS_Logo_Transparent_UKRI.png
    :align: center

- Fixed a change in behaviour after upgrading Qt, where floating point numbers in the matrix data view were not displayed unless the column was wide enough to show all digits.


InstrumentViewer
----------------

Bugfixes
############
- The instrument view startup time has been improved by avoid checking for masking if it is not present.
- Fix crash when overlaying peaks in side-by-side view of instrument viewer which is not supported.


SliceViewer
-----------

Bugfixes
############
- Fix for the colorbar bug where using log scale causes slice viewer to throw exception with all masked slice.
- The colorbar now remains autoscaled when switching between the different normalisation options.
- :ref:`SliceViewer <sliceviewer>` will now support ``W_MATRIX`` log for nonorthogonal axes with :ref:`MDHistoWorkspaces <MDHistoWorkspace>`. This prevents a bug related where basis vectors are used to calculate the projection.
- Fixes a bug in the :ref:`SliceViewer <sliceviewer>` where a workspace is replaced by an algorithm with more than one output workspace.
- For 3d+ datasets, :ref:`SliceViewer <sliceviewer>` will now automatically open to the zero-slice if present.
- Ensure that all nan/inf and zero slices work while using log-scaling.
- Fix a bug where the axes limits of 1D cut viewer plot were not autoscaling on cut update after the user had zoomed or panned.
- Added a check on toggling non-orthogonal view off that ensures the non-axis cutting tool is not enabled if the workspace does not support non-axis cuts (e.g. is 4D).


:ref:`Release 6.5.0 <v6.5.0>`

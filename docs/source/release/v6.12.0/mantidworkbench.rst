========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Add ``mantidworkbench`` as an entry point to launch the mantidworkbench Conda package.
- Configure the Conda ``workbench`` and ``mantidworkbench`` entry points to launch workbench with jemalloc on Linux.
- Enabled surface and contour plotting in the `Plot Advanced` dialog when multiple single-spectrum workspaces are selected.
- Fixed a bug where selecting multiple workspaces in the ADS and plotting a wireframe would only result in one of the workspaces being plotted.
- A crosshair toggle option has been added in mantidplots
- Added a link to the mantid-help email to the Help menu.
- The setting widget no longer immediately writes to file upon changing a setting. Instead click 'Okay' or 'Apply' to save the changes.
- Updated compiler on macOS from version 16 to version 18, which should result in performance improvements. See https://releases.llvm.org for release notes.
- Updated the definition of VULCAN for six panels as of 2022-05-15
- Added a Move to top button in the Manage User Directories UI.
- Matrix Workspaces can now be plotted as plots, markers, or errorbars by specifying the :ref:`plot type <MatrixWorkspace_Plotting>`.


Bugfixes
--------
- Fixed the reported name of the operating system for macOS users when an error report is generated.
- Legends now automatically snap back to the default position if dragged off-screen.
- The colorbar (used in Sliceviewer) has been updated to again allow the registration of custom matplotlib colormaps in scripts.


InstrumentViewer
----------------

New features
############


Bugfixes
############



SliceViewer
-----------

New features
############


Bugfixes
############
- Fixed a bug in the :ref:sliceviewer which was not showing data with correct ranges when doing a Transpose in non-orthogonal view.
- Fix bug producing invalid data errors when plotting MDHistoWorkspaces with an original workspace in SliceViewer
- Fixed a bug where the workspace normalization is not preserved when one has more than two dimensions and one uses the slider to change the slice point.


:ref:`Release 6.12.0 <v6.12.0>`

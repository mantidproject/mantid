========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- ``mantidworkbench`` is now an additional entry point for launching the ``mantidworkbench`` Conda package.
- ``workbench`` and ``mantidworkbench`` Conda entry points now launch workbench with ``jemalloc`` configured as the memory allocator on Linux.
- Surface and contour plots can now be generated more easily for multiple single-spectrum workspaces (via the `Plot Advanced` dialog when multiple single-spectrum workspaces are selected).
- A crosshair tool has been added to plot toolbars.
- Added an `Email mantid-help@mantidproject.org` action to the Help menu.
- The settings dialog now `Okay`, `Apply` and `Cancel` buttons. Settings are no longer immediately applied when parameters are changed.
- MacOS compiler updated from version 16 to version 18, which should result in performance improvements. See https://releases.llvm.org for release notes.
- The VULCAN instrument definition file now has six panels starting from 2022-05-15.
- The Manage User Directories dialog now has a `Move to Top` button.
- ``setPlotType()`` can now be called on Matrix Workspaces to specify the :ref:`plot type <MatrixWorkspace_Plotting>`.


Bugfixes
--------
- Wireframe plots will now display multiple workspaces when multiple workspaces are selected.
- Legends now automatically snap back to the default position if dragged off the edge of a plot.
- The :ref:`Sample Transmission Calculator <sample_transmission_calculator>` now restricts entering commas mixed with decimal points in the text boxes for ``Low``, ``Width`` and ``High`` fields.
- Standalone Linux installations should now launch without a segfault on Ubuntu systems.


SliceViewer
-----------

Bugfixes
############
- Custom ``colormaps`` registered with ``matplotlib`` are now available in the SliceViewer.
- Transposed data in the non-orthogonal view will now show with the correct ranges.
- ``MDHistoWorkspaces`` open in SliceViewer will no longer produce ``Variable invalidated`` error messages if the underlying workspace is changed.
- Normalisation is now correctly preserved when moving the slider with MD workspaces that contain more than one dimension.


:ref:`Release 6.12.0 <v6.12.0>`

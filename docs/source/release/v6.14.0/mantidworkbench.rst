========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- The ability to scale axis and colorbars to ``symlog`` has been added to plots. An option to specify the extent of the linear portion of the ``symlog`` axis (``linthresh``) has been added to the axes tab widget in the plot config dialog.
- Sequential fit output workspace names now include the numeric (spectra) axis to distinguish between different results.
- Sequential fit interface now has an option to create the results output in a workspace 2D format which could be plotted.
- Created :ref:`PythonObjectProperty <PythonObjectProperty>` which allows using generic python objects as inputs to algorithms defined through the python API
- Created :ref:`PythonObjectTypeValidator <PythonObjectTypeValidator>` for use with :ref:`PythonObjectProperty <PythonObjectProperty>`, to enforce correct typing on python objects
- The crosshair button is now enabled in tiled plots(subplots).
- The crosshair has now been optimized using matplotlib's blitting function for fast rendering. It can very closely tracks mouse movement with minimal lag now.

MantidWorkbench now features a new, integrated HTML-based help viewer, replacing the previous Qt Assistant-based system. This modernization provides a more streamlined user experience and aligns with current web standards.

**Key Enhancements & Foundational Changes:**

-   **Modern Help Viewer**: A new help window directly renders HTML documentation within MantidWorkbench, offering improved navigation and display.
-   **Standardized Documentation Path**: To support the new viewer, HTML documentation now installs to a consistent location: ``<prefix>/share/doc/html/``.
    *   Build systems (CMake, conda recipes) have been updated for this standard.
-   **Reliable Document Discovery**:
    *   The new help viewer uses an absolute path (``docs.html.root`` in `Mantid.properties`) to reliably locate local documentation.
    *   Legacy internal components (C++ ``MantidHelpWindow`` and Python ``gui_helper.py`` fallbacks, during their transition phase) have also been updated to recognize the new ``share/doc/html/`` directory.
-   **Phasing out Qt Assistant**:
    *   Conda build scripts for `mantiddocs` and related packages now disable the generation of QtHelp/QtAssistant components (e.g., ``.qhc`` files), as part of the move away from Qt Assistant.
-   **CMake Configuration Streamlined**:
    *   Related cleanup of some unused CMake variables in `Framework/Kernel/CMakeLists.txt` was performed.

**Impact:**

-   Delivers a more modern and integrated help experience within MantidWorkbench.
-   Ensures consistent and reliable access to locally installed documentation.
-   Progresses Mantid's codebase by removing dependencies on the older Qt Assistant technology.

Bugfixes
--------
- Opening the help window will no longer cause a crash in Windows conda installs of mantidworkbench.
- ``Show Invisible Workspaces`` settings can now be updated from the settings menu.
- Fixed workspaces starting with "_" not populating the legend of plots.
- Python warning for `np.bool` scalars interpreted as index is no longer present when using the MantidPlot widget.


InstrumentViewer
----------------

New features
############


Bugfixes
############
- Add a warning to the `Pick` tab of the Instrument View for a workspace with blocksize 1. In this case, the instrument view will not provide a line plot until the workspace has been rebinned.


SliceViewer
-----------

New features
############
- Added a masking feature to Sliceviewer for Matrix Workspaces with a non numeric y-axis. This enables direct application of the mask to the underlying workspace, or the outputting of a table workspace that can be applied subsequently using ``MaskFromTableWorkspace``.

Bugfixes
############


:ref:`Release 6.14.0 <v6.14.0>`

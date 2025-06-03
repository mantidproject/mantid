========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
.. amalgamate:: Workbench/New_features

MantidWorkbench now features a new, integrated HTML-based help viewer, replacing the previous Qt Assistant-based system. This modernization provides a more streamlined user experience and aligns with current web standards.

**Key Enhancements & Foundational Changes:**

-   **Modern Help Viewer**: A new help window directly renders HTML documentation within MantidWorkbench, offering improved navigation and display.
-   **Standardized Documentation Path**: To support the new viewer, HTML documentation now installs to a consistent location: ``<prefix>/share/doc/html/``.
    *   Build systems (CMake, Conda recipes) have been updated for this standard.
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
.. amalgamate:: Workbench/Bugfixes


InstrumentViewer
----------------

New features
############
.. amalgamate:: Workbench/InstrumentViewer/New_features

Bugfixes
############
.. amalgamate:: Workbench/InstrumentViewer/Bugfixes


SliceViewer
-----------

New features
############
.. amalgamate:: Workbench/SliceViewer/New_features

Bugfixes
############
.. amalgamate:: Workbench/SliceViewer/Bugfixes

:ref:`Release 6.14.0 <v6.14.0>`

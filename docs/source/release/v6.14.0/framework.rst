=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- :ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>` has the new option ``AppendIdxToOutputName`` that, when enabled with the ``CreateOutput`` option, will append indices of output workspace names (spectrum, workspace or numeric) based on the input format.
- :ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>` has the new option ``Output2D`` that, when enabled with the ``CreateOutput`` option, will create a 2D workspace of the results table that could be plotted.
- New algorithm :ref:`algm-EstimateScatteringVolumeCentreOfMass`, estimates the centre of mass of the intersection between an illumination volume and a sample shape.
- ``jemalloc`` has been added to the conda activation/deactivation scripts to enable use when running using the python interface (and not Workbench). Previously it was only applied in the Workbench startup script. This should improve the performance of many algorithms.
- Add ``UpdateUB`` option to :ref:`algm-IndexPeaks` that saves the optimized UB matrix in the case where there is a single run and ``CommonUBForAll=False``.
- Added new algorithm, :ref:`algm-TimeDifference-v1`, for calculating the time difference between a series of runs and a reference run.

Bugfixes
############
- Fixed a performance regression in :ref:`SaveMD <algm-SaveMD>` that was leading to dramatically increased save times.

Deprecated
############
- :ref:`LoadPreNexusLive <algm-LoadPreNexusLive>` has been deprecated. There is no replacement.
- :ref:`algm-SCDCalibratePanels-v1` has been deprecated, use :ref:`algm-SCDCalibratePanels-v2` instead.
- Removed the experimental multiprocess loader from :ref:`algm-LoadEventNexus` and deprecated the ``LoadType`` property.

Removed
############
- The algorithm ``ConvertUnitsUsingDetectorTable``, deprecated in :ref:`Release 3.9.0 <v3.9.0>`, has now been removed.

Fit Functions
-------------

New features
############
- Performance optimisations have been made to the process of adding ties to a fit function.


Data Objects
------------

Bugfixes
############
- When loading monitors with period data using :ref:`LoadNexusMonitors v2 <algm-LoadNexusMonitors-v2>`, period sample logs are now added to the resultant workspace; this is in line with the creation of Event Workspaces. This fixes a bug that occurred when :ref:`algm-NormaliseByCurrent` was used on the monitor workspace.
- Inconsistencies sometimes occur in period-related logs within Event NeXus files, such as duplicated entries with the same timestamps. Therefore, these repeated entries are reduced to a single entry. This can lead to runtime errors when loading the file which, in the past, have lead to crashes. In order to prevent this, the event workspace is now created without the period logs if issues are detected.


Python
------

New features
############
- The large offline documentation is now an optional install, reducing installer/download size significantly. For users that prefer online docs, this saves considerable disk space (potentially hundreds of MB). Those who prefer local/offline usage can still opt to install the documentation package and continue working without internet access. A clear indicator has been added to the Help Window's toolbar to show whether Mantid is displaying ``Local Docs`` or ``Online Docs``.
- Introduced a prototype "side-by-side" help system that includes both the legacy QtHelp-based viewer and a new Python-based Help Window using an embedded web browser (QWebEngine) to display documentation within Mantid Workbench.
- macOS users with Apple Silicon (Arm-based architecture) are now warned if they have installed the Intel-based Mantid package.
- Created :ref:`PythonObjectProperty <PythonObjectProperty>`, which allows using generic python objects as inputs to algorithms defined through the python API.
- Created :ref:`PythonObjectTypeValidator <PythonObjectTypeValidator>` for use with :ref:`PythonObjectProperty <PythonObjectProperty>`, to enforce correct typing on python objects.

Bugfixes
############
- Previously, Mantid could crash if the material set on a sample already existed, rather than being created from scratch. Improvements have now been made in an attempt to rectify the issues.

Dependencies
------------------

New features
############
- This release has removed all reliance on the `NeXus API <https://github.com/nexusformat/code>`_ .  Instead the ``File`` class was rewritten to use direct calls to the `HDF5 API <https://github.com/HDFGroup/hdf5/tree/4f1c3b6a4c7f2af6b617aede8dfb0ff1a6c58850>`_. In some places the C++ API (``H5Cpp``) is used for compatibility with other Mantid packages that also use the C++ API, but otherwise the low-level C API (``hdf5``) is used.  This decision was made because:

  1. The C++ API is unsupported by the HDF Group (in private correspondence they suggested we use a 3rd party API).
  2. The C++ API is not properly documented.
  3. The C++ API does not have access to every possible feature or option that might be needed.
  4. The C API is fully documented.
  5. The C API is regularly maintained.
  6. The C API is the "base" API for HDF5, and so automatically supports all available features natively.

  Throughout this release cycle, we have been methodically replacing and modernizing the ``Nexus`` abstraction layer, one piece of functionality at a time.  This has included modern objects such as vectors and strings in place of arrays and C-strings, fixing many instances that apparently led to memory leaks due to unclosed objects, and simplifying the logic inside many of the methods for saving or loading data.

  This should not impact the observed behavior, except that memory should be more cleanly freed in Workbench, and loading or saving operations might run slightly faster.

  Loading of HDF4 files is still handled by the read-only ``LegacyNexus`` layer.

  Those interested in the details of the changes can see them in the (developer centric)
  `github issue <https://github.com/mantidproject/mantid/issues/38332>`_ or by following the `Nexus tag <https://github.com/mantidproject/mantid/pulls?q=is%3Apr+is%3Aclosed+label%3ANexus>`_.
- Pin build 2 of ``seekpath`` v2.1.0 which removes an erroneous dependency on the ``future`` package. ``seekpath`` is a dependency of ``euphonic``. ``future`` is not used and has a known vulnerability `CVE-2025-50817 <https://github.com/advisories/GHSA-xqrq-4mgf-ff32>`_ .
- Updated header and source files for ADARA packets from v1.5.1 to v1.10.3


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 6.14.0 <v6.14.0>`

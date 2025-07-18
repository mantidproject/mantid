========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- The :ref:`Python script editor <WorkbenchScriptWindow>` now supports ``bool`` and f-string syntax highlighting.
- Users can now save multiple selected workspaces in the ASCII format.

.. image:: ../../images/6_13_release/crosshair-example.png
   :class: screenshot
   :width: 500px
   :align: right

- A crosshair toggle option has been added in mantid plots on the top right of the toolbar area (see image). The
  crosshair button is disabled (invisible) in tiled and 3D plots.
- The :ref:`WorkbenchSettings` widget's ``Apply`` button is now disabled when there are no pending changes.
- Deprecated parameter ``Unwrap Ref`` has been removed from the Powder Diffraction Reduction Interface.
- Added property ``errorreports.core_dumps``.

  - Linux users can set this to the directory on their system where core dump files are put after a crash (e.g
    ``errorreports.core_dumps=/var/lib/apport/coredump``). Workbench will then be able to use this property to extract
    useful information from the core dump file created after a crash and give that to the error reporting service. This
    will help us to diagnose some problems where previously no stacktrace was available
    after a crash.
  - On Linux, core dumps are now always turned on for the workbench process.
  - This property and additional stack trace information are not currently available on macOS or Windows.

Bugfixes
--------
- The ``...`` button for a :ref:`user defined function <user_defined_function>` is no longer obscured by the scroll bar
  in the :ref:`Fit Property Browser <WorkbenchPlotWindow_Fitting>` on macOS.
- Minimizing the sample log plot on the ``SampleLogs`` widget so that it is not visible no longer throws an error.

SliceViewer
-----------

New features
############
- When plotting peaks in HKL to use the peak positions calculated from Q\ :sub:`sample`, it is now possible to use the
  UB instead of peak index.

Bugfixes
############
- The :ref:`sliceviewer` no longer raises an error when exporting a pixel cut from an :ref:`MDHistoWorkspace`

:ref:`Release 6.13.0 <v6.13.0>`

========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- The :ref:`Python script editor <WorkbenchScriptWindow>` now supports Bool syntax highlighting.
- The :ref:`Python script editor <WorkbenchScriptWindow>` now supports f-string syntax highlighting.
- Users can now save multiple selected workspaces as ASCII.
- The crosshair button is disabled (invisible) in 3D plots, which include surface plot, wireframe plot and mesh plot.
- The settings widget 'Apply' button is now disabled when there are no pending changes.
- A crosshair toggle option has been added in mantidplots on the top right of the toolbar area. The crosshair button is disabled (invisible) in titled plots.
- Deprecated parameter ``Unwrap Ref`` has been removed from the Powder
  Diffraction Reduction Interface.
- Added property ``errorreports.core_dumps``. Linux users can set this to the directory on their system where core dump files are put after a crash (e.g ``errorreports.core_dumps=/var/lib/apport/coredump``).
  Workbench will then be able to use this property to extract useful information from the core dump file created after a crash and give that to the error reporting service.
  This will help us to diagnose some problems where previously no stacktrace was available after a crash. On Linux, core dumps are now always turned on for the workbench process.


Bugfixes
--------
- The ``...`` button for a :ref:`user defined function <user_defined_function>` is no longer obscured by the scroll bar in the :ref:`Fit Property Browser <WorkbenchPlotWindow_Fitting>` on macOS.
- Minimizing the sample log plot on the ``SampleLogs`` widget so that it is not visible no longer throws an error.


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
- Added the option when plotting peaks in HKL to use the peak positions calculated from Q\ :sub:`sample` using the UB instead of peak index.

Bugfixes
############
- Sliceviwer no longer raises an error when exporting a pixel cut from a MDHistoWorkspace

:ref:`Release 6.13.0 <v6.13.0>`

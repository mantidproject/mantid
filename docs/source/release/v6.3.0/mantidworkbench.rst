========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------
- Add ability to rotate Ellipse and Rectangle shapes in :ref:`InstrumentViewer`.
- In the pick tab of the instrument viewer, integration is now by default over the entire detector unless some other curve is requested (such as by drawing a shape or picking a detector).
- **In the pick tab of the instrument viewer, a new panel allowing for direct rebinning of the workspace now exists.**
.. figure:: ../../images/iview_insitu_rebin.png
    :width: 500px
    :align: center
- The instrument is now loaded in a background thread when opening the instrument viewer which prevents running scripts from hanging.
- The integration slider in the instrument viewer now support discrete steps when the axis has discrete values.
- The algorithm browser has been tidied to reduce the number of single algorithm categories.
- Table workspaces can now have read-only columns added to them (`ws.addReadOnlyColumn(<TYPE>, <NAME>)`). Existing columns can also be set to be read-only (`ws.setColumnReadOnly(<INDEX>, <TRUE/FALSE>)`).
- Cells containing vector data in a table workspace can now be viewed in the table workspace display.

- **The Error Reporter can now remember and prefill the user's name and email.**

.. image::  ../../images/ErrorReporter_RememberMe.png
    :align: center

- The browse dialog in the file finder widget now opens at the path specified in the widget's edit box (if the edit box contains a full path)

Bugfixes
--------
- Fixed arbitrary values not being accepted as the "Start Time" in StartLiveDataDialog.
- Fixed a bug where the option "SignedInPlaneTwoTheta" in :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis-v2>` would not give signed results.
- Fixed a bug where the toggle state of the "Grids on/off" toolbar button was incorrect when opening a 3D surface plot.
- Fixed issue in DrILL when ASCII output was requested but the logs to save were not defined for that instrument.
- The "About Mantid" page now appears on a new full release, even if a recent nightly was previously launched.
- Fixed a bug where copying data from a table displaying a matrix workspace was not working.
- Fixed plot bins not working on data with numeric X-axis.
- Calls to :ref:`EvaluateFunction <algm-EvaluateFunction>` when plotting a guess or fit result in the fit browser of a figure correctly ignores invalid data when requested.
- Fixed a bug where the z-axis editor dialog was being initialised from the y-axis for a 3D plot.
- Fixed a bug with autoscaling of colorfill plots from within the figure options.
- Fixed an issue to plot negative values with logarithm scaling in slice view.
- Workbench will no longer hang if an algorithm was running when workbench was closed.
- Stopped workbench from ignoring GUIs that want to cancel closing


:ref:`Release 6.3.0 <v6.3.0>`

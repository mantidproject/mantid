========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------
- Add ability to rotate Ellipse and Rectangle shapes in :ref:`InstrumentViewer`.
- In the pick tab of the instrument viewer, integration is now by default over the entire detector unless some other curve is requested (such as by drawing a shape or picking a detector).
- In the pick tab of the instrument viewer, a new panel allowing for direct rebinning of the workspace now exists.
.. figure:: ../../images/iview_insitu_rebin.png
    :width: 500px
    :align: center
- The integration slider in the instrument viewer now support discrete steps when the axis has discrete values.
- The algorithm browser has been tidied to reduce the number of single algorithm categories.
- Table workspaces can now have read-only columns added to them (`ws.addReadOnlyColumn(<TYPE>, <NAME>)`). Existing columns can also be set to be read-only (`ws.setColumnReadOnly(<INDEX>, <TRUE/FALSE>)`).

Bugfixes
--------
- Fixed arbitrary values not being accepted as the "Start Time" in StartLiveDataDialog.
- Fixed a bug where the option "SignedInPlaneTwoTheta" in :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis-v2>` would not give signed results.
- Fixed a bug where the toggle state of the "Grids on/off" toolbar button was incorrect when opening a 3D surface plot.
- Fixed issue in DrILL when ASCII output was requested but the logs to save were not defined for that instrument.
- Fixed a bug where copying data from a table displaying a matrix workspace was not working.
- Fixed plot bins not working on data with numeric X-axis.

:ref:`Release 6.3.0 <v6.3.0>`

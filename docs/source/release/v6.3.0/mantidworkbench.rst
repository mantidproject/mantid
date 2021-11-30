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
- The instrument is now loaded in a background thread when opening the instrument viewer which prevents running scripts from hanging.
- The integration slider in the instrument viewer now support discrete steps when the axis has discrete values.

Bugfixes
--------
- Fixed an issue when save_as a running script leads to crash upon script completion.
- Fixed arbitrary values not being accepted as the "Start Time" in StartLiveDataDialog.
- Calls to :ref:`EvaluateFunction <algm-EvaluateFunction>` when plotting a guess or fit result in the fit browser of a figure correctly ignores invalid data when requested.
- Fixed an issue to plot negative values with logarithm scaling in slice view.

:ref:`Release 6.3.0 <v6.3.0>`

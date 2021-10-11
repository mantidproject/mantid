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
- Normalisation (by maximum value) is available in Superplot

Bugfixes
--------
- Fixed arbitrary values not being accepted as the "Start Time" in StartLiveDataDialog.


:ref:`Release 6.3.0 <v6.3.0>`

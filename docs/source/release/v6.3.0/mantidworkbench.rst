========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New features
------------
* The :ref:`InstrumentViewer` has several new features.

  * **In the pick tab, a new panel allowing for direct rebinning of the workspace now exists.**
   .. figure:: ../../images/iview_insitu_rebin.png
      :width: 500px
      :align: center
  * The ability to rotate Ellipse and Rectangle shapes has been added.
  * The integration slider now supports discrete steps when the axis has discrete values.

* Table workspaces can now have read-only columns added to them (``ws.addReadOnlyColumn(<TYPE>, <NAME>)``). Existing columns can also be set to be read-only (``ws.setColumnReadOnly(<INDEX>, <TRUE/FALSE>)``).
* **The Error Reporter can now remember and prefill the user's name and email.**
.. image::  ../../images/ErrorReporter_RememberMe.png
    :align: center

Improvements
------------
* The :ref:`InstrumentViewer` has also been improved in several ways

  * In the pick tab integration is now by default over the entire detector unless some other curve is requested (such as by drawing a shape or picking a detector).
  * The instrument is now loaded in a background thread when opening the :ref:`InstrumentViewer` which prevents running scripts from hanging.
  * The Y-position of the HKL labels on the miniplot is now fixed in Axes coordinates so that the label remains visible as the zoom level changes.

* The algorithm browser has been tidied to reduce the number of single algorithm categories.
* Cells containing vector data in a table workspace can now be viewed in the table workspace display.
* The browse dialog in the file finder widget now opens at the path specified in the widget's edit box (if the edit box contains a full path).
* The font in python editor and IPython console are ensured to be monospace. It also ensures monospace on KDE Neon distributions too.

Bugfixes
--------
* Fixed an issue when ``save_as`` of a running script leads to crash upon script completion.
* Fixed arbitrary values not being accepted as the ``Start Time`` in ``StartLiveDataDialog``.
* Fixed a bug where the option ``SignedInPlaneTwoTheta`` in :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis-v2>` would not give signed results.
* A number of plotting bugfixes have been made.

  * Fixed a bug where the toggle state of the ``Grids on/off`` toolbar button was incorrect when opening a 3D surface plot.
  * Fixed plot bins not working on data with numeric X-axis.
  * Fixed a bug where the z-axis editor dialog was being initialised from the y-axis for a 3D plot.
  * Fixed a bug with autoscaling of colorfill plots from within the figure options.
  * Calls to :ref:`EvaluateFunction <algm-EvaluateFunction>` when plotting a guess or fit result in the fit browser of a figure correctly ignores invalid data when requested.
  * The axes limits of Waterfall plots will now scale correctly upon initial plotting and overplotting.

* Fixed issue in :ref:`DrILL <DrILL-ref>` when ``ASCII`` output was requested but the logs to save were not defined for that instrument.
* The ``About Mantid`` page now appears on a new full release, even if a recent nightly was previously launched.
* Fixed a bug where copying data from a table displaying a matrix workspace was not working.
* Workbench will no longer hang if an algorithm was running when workbench was closed.
* Fixed a bug in the editor where uncommenting using 'ctrl+/' wasn't working correctly for lines of the form ``<optional whitespace>#code_here # inline comment``.
* Commenting code in the editor using ``ctrl+/`` will preserve indenting (i.e. ``# `` will be inserted at the position of the first non-whitespace character in the line).
* Fixed a bug where folding the pick tab in the :ref:`InstrumentViewer` crashed Mantid.

SliceViewer
-----------

Bugfixes
########
- Fixed the ``out-of-range`` error when trying to access the projection matrix for a workspace with a non-Q axis before other Q axes.
- Fixed an issue to plot negative values with logarithm scaling.
- Fixed a bug in :ref:`Run <Run>` goniometer when using :ref:`algm-Plus`.
- Fixed the issue in ``SNSLiveEventDataListener`` when the instrument doesn't have monitors.
- When entering a specific value for the center of the slicepoint of an integrated dimension/axis it will no longer jump to the nearest bin-center (this fix also affects ``MDEvent`` workspaces as it was assumed each dimension had 100 bins for the purpose of updating the slider for a integrated dimension/axis).
- For ``MDHisto`` workspaces the projection matrix will be derived from the basis vectors on the workspace rather than searching for the ``W_MATRIX`` log.
- Slicepoint center is now set to the correct initial value (consistent with position of slider) for ``MDHisto`` workspaces.

:ref:`Release 6.3.0 <v6.3.0>`

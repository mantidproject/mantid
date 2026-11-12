========================
Mantid Workbench Changes
========================

New Features
------------
- (`#41652 <https://github.com/mantidproject/mantid/pull/41652>`_) Upgraded from Qt 5 to Qt 6.
- (`#42141 <https://github.com/mantidproject/mantid/pull/42141>`_) A new property ``CheckMantidVersion.NotifyUpdateOnStartup`` has been added to the Mantid.properties and is enabled by default.
  This setting controls whether Mantid Workbench prompts users to update on startup.
  It can be changed in ``Mantid.properties`` (set to 0 to disable the prompt) or via the "Prompt to update Mantid Workbench on startup" checkbox in :ref:`General Settings <GeneralSettings>`.
  Users can also disable future prompts by clicking "Don't show this again" in the update notification dialog.
- (`#41746 <https://github.com/mantidproject/mantid/pull/41746>`_) The sample logs viewer now allows users to copy the selected variable name to the clipboard.
- (`#41895 <https://github.com/mantidproject/mantid/pull/41895>`_) The :ref:`Script Repository <WorkbenchScriptRepository>` is now read-only. Scripts can still be downloaded and updated, but uploading and deleting scripts through Mantid is no longer possible. To contribute or remove scripts, use the `script repository on GitHub <https://github.com/mantidproject/scriptrepository>`_ directly. The ``Delete`` column has been removed from the interface, and the ``UploaderWebServer`` property has been removed from ``Mantid.properties``.
- (`#41928 <https://github.com/mantidproject/mantid/pull/41928>`_) A new ``--qt-rm-lockfiles`` command line argument forcibly removes lockfiles created by ``QSettings``. This option is not available on Windows.
- (`#42145 <https://github.com/mantidproject/mantid/pull/42145>`_) The default font sizes for plot titles, axis labels, and tick labels can now be configured in the :ref:`Plots tab of the Settings <PlotSettings>`. The values are saved in the user properties ``plots.titleFontSize``, ``plots.axesLabelFontSize`` and ``plots.ticks.labelSize``.

Bugfixes
--------
- (`#41715 <https://github.com/mantidproject/mantid/pull/41715>`_) The :menuselection:`File --> Save Project as...` dialog no longer crashes when monitors are loaded and ``Save Altered Workspaces Only`` is selected. Unmodified :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>` objects are now correctly skipped in this case.
- (`#42001 <https://github.com/mantidproject/mantid/pull/42001>`_) :ref:`Script generation<generate_plot_script>` for mantid plots now accounts for the device pixel ratio and figure layout.
- (`#41804 <https://github.com/mantidproject/mantid/pull/41804>`_) The Error Reporter dialog now handles remembered contact information more consistently. When ``Remember Me`` is ticked, the name and email address are remembered regardless of which button is clicked. Unticking ``Remember Me`` clears the remembered information.
- (`#42041 <https://github.com/mantidproject/mantid/pull/42041>`_) Fixed a crash in the :ref:`Fit Property Browser <WorkbenchPlotWindow_Fitting>` when finding peaks in HRP training course data.
- (`#41938 <https://github.com/mantidproject/mantid/pull/41938>`_) :ref:`Project Recovery<Project Recovery>` no longer crashes when encountering an unexpected error while listing files.
- (`#41949 <https://github.com/mantidproject/mantid/pull/41949>`_) Opening interfaces or changing default directories no longer unnecessarily rewrites and locks the Mantid Workbench settings file.
- (`#42037 <https://github.com/mantidproject/mantid/pull/42037>`_) On Linux systems with an NFS-backed configuration directory, Workbench now stages its ``QSettings files`` in the local cache. This avoids lock-file failures when running multiple Workbench instances and preserves conflicting settings for recovery. Existing settings for the Mantid Reduction interface must be migrated once to the staged INI file.
- (`#42396 <https://github.com/mantidproject/mantid/pull/42396>`_) Fixed a memory leak where the most recently opened window, such as a plot or the new :ref:`Instrument View<InstrumentViewer>`, was not released from memory when closed.


InstrumentViewer
----------------

The new :ref:`Instrument View<InstrumentViewer>` is now the default. All the following features and bugfixes relate to the new Instrument View. The previous version remains available from the same menu via ``Show Instrument (Legacy)``.

New features
############
- (`#41615 <https://github.com/mantidproject/mantid/pull/41615>`_) Added an option to draw the sample position, and labelled the ``Monitors`` and ``Sample`` checkboxes with a coloured circle to indicate the colour of the drawn items.
- (`#41731 <https://github.com/mantidproject/mantid/pull/41731>`_) When drawing the sample position, the defined sample shape is now drawn instead of a simple point.
- (`#41754 <https://github.com/mantidproject/mantid/pull/41754>`_) Improved performance for large instruments by creating the component tree only when a component is expanded, reducing initial loading time by several seconds.
- (`#41767 <https://github.com/mantidproject/mantid/pull/41767>`_) Rectangle zoom and scroll wheel zoom are now both supported.
- (`#42022 <https://github.com/mantidproject/mantid/pull/42022>`_) Right-clicking a workspace and selecting ``Show Instrument`` now opens the new Instrument View, and it is no longer labelled as experimental. The previous Instrument View is still available from the same menu via ``Show Instrument (Legacy)``.
- (`#42022 <https://github.com/mantidproject/mantid/pull/42022>`_) The :ref:`ALFView <ALFView-ref>` interface now uses the new Instrument View by default. The ``Use legacy Instrument View in interfaces`` option under ``File`` → ``Settings`` → ``General`` allows users to switch back to the legacy view.
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) Added user documentation for the new Instrument View under :ref:`InstrumentViewer`. Documentation for the legacy view is available under :ref:`LegacyInstrumentViewer`.
- (`#42007 <https://github.com/mantidproject/mantid/pull/42007>`_) Picked detectors are marked in magenta, with outlines around shapes and halos around points. The markers remain a fixed size on screen, making selections easy to see even when zoomed out.
- (`#41995 <https://github.com/mantidproject/mantid/pull/41995>`_) The line plot now updates to show the spectra covered by an overlaid ROI or mask shape, and follows the shape as it is dragged, resized, rotated or as the projection is zoomed.
- (`#41981 <https://github.com/mantidproject/mantid/pull/41981>`_) The line plot can now be zoomed and panned while adding or removing peaks.
- (`#42010 <https://github.com/mantidproject/mantid/pull/42010>`_) Detectors can now be picked while using rectangle zoom by holding ``Ctrl`` or ``Shift``.
- (`#42010 <https://github.com/mantidproject/mantid/pull/42010>`_) The detector with peaks closest to the mouse position can now be picked.
- (`#42113 <https://github.com/mantidproject/mantid/pull/42113>`_) The ``Grouping`` and ``Masking`` tabs now have a ``Create From Current Selection`` button, which creates an ROI or mask from the detectors currently selected in the projection. For example, detectors can be picked with the ``Select Bank/Tube`` option and then masked, without having to draw a shape around them.
- (`#42270 <https://github.com/mantidproject/mantid/pull/42270>`_) Added an option to rotate a projection across the screen, similar to the ``U Correction`` option in the old Instrument View.
- (`#42270 <https://github.com/mantidproject/mantid/pull/42270>`_) Improved the layout of the left-hand pane by arranging the buttons in two columns.
- (`#42413 <https://github.com/mantidproject/mantid/pull/42413>`_) ``Hover Pick`` now respects ``Select Bank/Tube``, showing the summed spectrum of the tube or bank under the cursor.

Bugfixes
############
- (`#41706 <https://github.com/mantidproject/mantid/pull/41706>`_) Fixed an error when overlaying a peaks workspace containing more peaks than there are detectors in the instrument, which can occur when viewing the output of ``ExtractSpectra``.
- (`#41706 <https://github.com/mantidproject/mantid/pull/41706>`_) Fixed an error when drawing workspaces containing detectors that are not assigned to any spectra.
- (`#41707 <https://github.com/mantidproject/mantid/pull/41707>`_) Peak lines in the line plot are now displayed correctly when the units are changed or the spectra are summed.
- (`#41799 <https://github.com/mantidproject/mantid/pull/41799>`_) Fixed a bug where certain detector shapes were not drawn correctly, and improved the performance of drawing these shapes.
- (`#41807 <https://github.com/mantidproject/mantid/pull/41807>`_) Improved performance when opening and building the detector mesh.
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) The tooltip for the render mode now matches the options shown in the drop-down list.
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) Detector picking now works when the Instrument View is opened from a Jupyter notebook.
- (`#42158 <https://github.com/mantidproject/mantid/pull/42158>`_) The rubber band zoom box is now visible while dragging, following an update to a third-party rendering dependency.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) Workspaces with x-axis units that cannot be converted are now displayed using their own x values and units. Unit selection is disabled for these workspaces, and the Instrument View continues to work when units are missing or the instrument has no sample or source position.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) Overlaying peaks workspaces is disabled when the workspace's units are incompatible with peaks, and Adding/Deleting Peaks Mode is disabled when the units cannot be converted.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) The unit selectors now display the workspace's own unit, such as energy, when it is not one of the listed units, rather than defaulting to time-of-flight.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) Workbench no longer hangs when an algorithm, such as ``ConvertUnits``, overwrites the workspace displayed in the Instrument View.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) Summing selected spectra now falls back to plotting them individually when their units prevent them from sharing a common binning, such as momentum transfer for a detector in the path of the beam.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) The contour and integration range controls now remain visible when all detectors have the same counts or the range is empty. They are disabled until there is a range to adjust.
- (`#42396 <https://github.com/mantidproject/mantid/pull/42396>`_) Fixed a memory leak that could occur when the Instrument View was closed.
- (`#42407 <https://github.com/mantidproject/mantid/pull/42407>`_) Zooming with the mouse wheel in the 3D projection now keeps the point under the cursor in place rather than zooming into the centre of the view. Zooming out now stops at the full view of the instrument while preserving the current viewing direction.
- (`#42407 <https://github.com/mantidproject/mantid/pull/42407>`_) Mouse-wheel zooming now starts at the same rate in all projections. Previously, the initial zoom in the spherical and cylindrical projections was much larger than in the other projections.
- (`#42609 <https://github.com/mantidproject/mantid/pull/42609>`_) Shape drawing controls are now disabled when the Instrument View opens in the 3D projection.
- (`#42598 <https://github.com/mantidproject/mantid/issues/42598>`_) In the ``Approximated Shapes (Fast)`` render mode, box-shaped detectors defined with their width or height along a different axis, for example on MUSR, EMU, NIMROD, INTER and the OSIRIS silicon analyser, are now drawn in full. Previously they were not drawn.


SliceViewer
-----------

Bugfixes
############
- (`#41780 <https://github.com/mantidproject/mantid/pull/41780>`_) :ref:`SliceViewer <sliceviewer>` now displays data correctly in non-orthogonal views when L is selected as a projection axis.
- (`#42428 <https://github.com/mantidproject/mantid/pull/42428>`_) :ref:`SliceViewer <sliceviewer>` can now be reopened after using the non-axis-aligned cutting tool without raising an error.
- (`#42428 <https://github.com/mantidproject/mantid/pull/42428>`_) Moving the mouse over a plot no longer raises an error after its :ref:`SliceViewer <sliceviewer>` window has been closed.
- (`#42533 <https://github.com/mantidproject/mantid/pull/42533>`_) The right-click context menu now opens on line-cut plots in :ref:`SliceViewer <sliceviewer>`.

:ref:`Release 7.0.0 <v7.0.0>`

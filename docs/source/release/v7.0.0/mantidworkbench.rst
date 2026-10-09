========================
Mantid Workbench Changes
========================

New Features
------------
- (`#41652 <https://github.com/mantidproject/mantid/pull/41652>`_) Mantid on Linux has moved from Qt5 to Qt6.
- (`#42141 <https://github.com/mantidproject/mantid/pull/42141>`_) A new property `CheckMantidVersion.NotifyUpdateOnStartup` has been added to the Mantid.properties file which is enabled by default.
- (`#42141 <https://github.com/mantidproject/mantid/pull/42141>`_) The same property can be updated via the check box in General Settings as "Prompt to update Mantid Workbench on startup" in the mantid workbench.
- (`#42141 <https://github.com/mantidproject/mantid/pull/42141>`_) This introduces a more persuasive prompt on the startup to update mantid workbench and can be disabled by setting with `0` in your Mantid.properties file.
- (`#42141 <https://github.com/mantidproject/mantid/pull/42141>`_) The user can also disable the prompt by unchecking the check box in General Settings or by clicking on the "Don't show this again" button in the update notification dialog.
- (`#41746 <https://github.com/mantidproject/mantid/pull/41746>`_) The sample logs viewer allows copying the selected variable name to clipboard
- (`#41895 <https://github.com/mantidproject/mantid/pull/41895>`_) The :ref:`Script Repository <WorkbenchScriptRepository>` is now read-only. Scripts can still be downloaded and kept up to date, but they can no longer be uploaded to, or deleted from, the central repository through Mantid. To contribute or remove scripts, use the `script repository on GitHub <https://github.com/mantidproject/scriptrepository>`_ directly. The ``Delete`` column has been removed from the interface, and the ``UploaderWebServer`` property has been removed from the properties file.
- (`#41928 <https://github.com/mantidproject/mantid/pull/41928>`_) new command line argument ``--qt-rm-lockfiles`` forcibly removes lockfiles created by QSettings. This is not available on Windows.
- (`#42145 <https://github.com/mantidproject/mantid/pull/42145>`_) The default font size for the plot title, axis labels, and tick labels on plots can now be set on the :ref:`Plots tab of the Settings <PlotSettings>`. The values are saved to the user properties as ``plots.titleFontSize``, ``plots.axesLabelFontSize`` and ``plots.ticks.labelSize``.


Bugfixes
--------
- (`#41715 <https://github.com/mantidproject/mantid/pull/41715>`_) The :menuselection:`File --> Save Project as...` dialog now correctly skips an unmodified :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>` when monitors are loaded and ``Save Altered Workspaces Only`` is selected without resulting in mantid crashing.
- (`#42001 <https://github.com/mantidproject/mantid/pull/42001>`_) :ref:`Script generation<generate_plot_script>` of a mantid plot now considers the device pixel ratio and figure layout to generate the script.
- (`#41804 <https://github.com/mantidproject/mantid/pull/41804>`_) The Error Reporter Dialog now stores remembered contact information in its own settings file instead of the Workbench settings file. Previously saved Workbench contact information remains
  available as a read-only fallback for values not yet set in the new file. With ``Remember Me`` ticked, the name and email are remembered whichever button is clicked, and they are never sent
  if ``Don't share any information`` is clicked. Unticking ``Remember Me`` clears any remembered contact information. The settings file is only updated when this information changes.
- (`#42041 <https://github.com/mantidproject/mantid/pull/42041>`_) Fixed a crash in Fit Property Browser ocurring when trying to find peaks for HRP data due to an unhandled exception raised from the :ref:`Levenberg-MarquardtMD minimizer <LevenbergMarquardtMD>`.
- (`#41938 <https://github.com/mantidproject/mantid/pull/41938>`_) The :ref:`Project Recovery <Project Recovery>` feature is now guarded against a ``TypeError`` emanating from the ``listdir_fullpath`` method.
- (`#41948 <https://github.com/mantidproject/mantid/pull/41948>`_) In the ISIS Reflectometry interface, fixed a bug where the (new) Instrument View would not fill the available space in the dock when first opened. The view now correctly fills the dock area upon initial display.
- (`#41948 <https://github.com/mantidproject/mantid/pull/41948>`_) In the ALF View interface, fixed bug where the "Add ROI" button was not working. The button now correctly adds a new ROI to the view when clicked.
- (`#41948 <https://github.com/mantidproject/mantid/pull/41948>`_) In the ALF View interface, fixed an error on opening the interface.
- (`#41949 <https://github.com/mantidproject/mantid/pull/41949>`_) Opening interfaces or changing default directories no longer unnecessarily rewrites and locks the Mantid Workbench settings file.
- (`#42037 <https://github.com/mantidproject/mantid/pull/42037>`_) On Linux systems with an NFS-backed configuration directory, Workbench now stages its QSettings files in the local cache. This avoids lock-file failures when running multiple Workbench instances and preserves conflicting settings for recovery. Existing settings for the Mantid Reduction interface must be migrated once to the staged INI file.
- (`#42355 <https://github.com/mantidproject/mantid/pull/42355>`_) Opening the About dialog on Windows no longer prints a warning about font size in the messages window.
- (`#42396 <https://github.com/mantidproject/mantid/pull/42396>`_) Fixed a bug in Workbench where the most recently opened window, such as a plot or the new Instrument View, could stay in memory after it was closed.


InstrumentViewer
----------------

New features
############
- (`#41615 <https://github.com/mantidproject/mantid/pull/41615>`_) In the new Instrument View, added an option to draw the sample position, and labelled the ``Monitors`` and ``Sample`` checkboxes with a coloured circle to indicate the colour of the drawn items.
- (`#41731 <https://github.com/mantidproject/mantid/pull/41731>`_) In the new Instrument View, when drawing the sample position, if there is a sample shape defined, then that shape will be drawn instead of a simple point.
- (`#41754 <https://github.com/mantidproject/mantid/pull/41754>`_) Sped up new Instrument View by creating the component tree lazily, i.e. only when a component in the tree is expanded. This will give a speed-up of a few seconds for large instruments.
- (`#41767 <https://github.com/mantidproject/mantid/pull/41767>`_) New Instrument View now allows for both rectangle zoom and scroll wheel zoom.
- (`#42022 <https://github.com/mantidproject/mantid/pull/42022>`_) The new Instrument View is now the default. Right-clicking a workspace and selecting ``Show Instrument`` opens the new Instrument View, and it is no longer labelled as experimental. The previous Instrument View is still available from the same menu as ``Show Instrument (Legacy)``.
- (`#42022 <https://github.com/mantidproject/mantid/pull/42022>`_) The setting controlling which Instrument View the ALFView and ISIS Reflectometry interfaces use has been replaced by ``Use legacy Instrument View in interfaces``, found under ``File`` → ``Settings`` → ``General``. These interfaces now use the new Instrument View unless this option is ticked.
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) The new Instrument View is now documented. See :ref:`InstrumentViewer` for the user documentation. The documentation for the previous instrument view has moved to :ref:`LegacyInstrumentViewer`.
- (`#42007 <https://github.com/mantidproject/mantid/pull/42007>`_) In the new Instrument View, picked detectors are now marked in magenta: shapes are given an outline and points a surrounding halo. These markers are drawn at a fixed size on screen, so a selection stays easy to spot even when zoomed out on a large instrument.
- (`#41995 <https://github.com/mantidproject/mantid/pull/41995>`_) In the new Instrument View, the line plot now updates to show the spectra covered by an overlaid ROI or mask shape, and follows the shape as it is dragged, resized, rotated or as the projection is zoomed.
- (`#41981 <https://github.com/mantidproject/mantid/pull/41981>`_) New Instrument View now allows for zooming and panning in lineplot when adding/removing peaks.
- (`#42010 <https://github.com/mantidproject/mantid/pull/42010>`_) New instrument view now allows picking in rectangle zoom by pressing the `Ctrl` or `Shift` keys.
- (`#42010 <https://github.com/mantidproject/mantid/pull/42010>`_) New instrument view now allows picking the closest detector with peaks to the mouse position.
- (`#42113 <https://github.com/mantidproject/mantid/pull/42113>`_) In the new Instrument View, the ``Grouping`` and ``Masking`` tabs now have a ``Create From Current Selection`` button, which creates an ROI or mask from the detectors currently selected in the projection. For example, detectors can be picked with the ``Select Bank/Tube`` option and then masked, without having to draw a shape around them.
- (`#42270 <https://github.com/mantidproject/mantid/pull/42270>`_) In the new Instrument View, add an option to rotate a projection across the screen, similar to the ``U Correction`` option in the old Instrument View.
- (`#42270 <https://github.com/mantidproject/mantid/pull/42270>`_) In the new Instrument View, improve the layout of the left-hand pane by arranging the buttons in two columns.
- (`#42413 <https://github.com/mantidproject/mantid/pull/42413>`_) In the new Instrument View, ``Hover Pick`` now respects ``Select Bank/Tube``, showing the summed spectrum of the tube or bank under the cursor.

Bugfixes
############
- (`#41706 <https://github.com/mantidproject/mantid/pull/41706>`_) In the new Instrument View, fixed a bug when overlaying a peaks workspace with more peaks than the number of detectors in the instrument. This could happen if viewing e.g. the result of ``ExtractSpectra``.
- (`#41706 <https://github.com/mantidproject/mantid/pull/41706>`_) In the new Instrument View, fixed a bug with drawing workspaces where some detectors are not assigned to any spectra.
- (`#41707 <https://github.com/mantidproject/mantid/pull/41707>`_) New Instrument View now has correct peak lines in lineplot when the units are changed and/or the spectra in the lineplot are summed.
- (`#41799 <https://github.com/mantidproject/mantid/pull/41799>`_) In the new Instrument View, fixed a bug where certain detector shapes were not drawing correctly, and sped up the drawing of these shapes.
- (`#41807 <https://github.com/mantidproject/mantid/pull/41807>`_) In new Instrument View, sped up initial opening and building of the detector mesh
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) In the new Instrument View, the tooltip for the render mode now matches the options shown in the drop-down list.
- (`#42202 <https://github.com/mantidproject/mantid/pull/42202>`_) Fixed an error when picking detectors in the new Instrument View from a Jupyter notebook.
- (`#42158 <https://github.com/mantidproject/mantid/pull/42158>`_) In the new Instrument View, fixed a bug where the rubber band zoom box was not visible while dragging, following an update to a third-party rendering dependency. The zoom itself was unaffected.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) In the new Instrument View, workspaces whose x axis units cannot be converted no longer cause errors. This covers workspaces with no units at all, workspaces with a unit such as a label or degrees, and workspaces whose instrument has no sample or source position. Unit selection is disabled for these workspaces, which are shown in their own x values and labelled with their own unit, and everything else continues to work as normal.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) In the new Instrument View, peaks workspaces cannot be overlaid on a workspace in a unit peaks cannot be shown in, such as energy, and Adding/Deleting Peaks Mode is also disabled for workspaces whose units cannot be converted, since peaks there could not be seen or added.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) In the new Instrument View, the unit selectors now show the workspace's own unit when it is not one of the listed units, such as energy, rather than time-of-flight.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) The new Instrument View no longer hangs Workbench when an algorithm, such as ConvertUnits, overwrites the workspace it is showing.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) In the new Instrument View, summing the selected spectra no longer fails for units in which the spectra cannot share a common binning, such as momentum transfer for a detector in the path of the beam. The spectra are plotted unsummed in this case.
- (`#42380 <https://github.com/mantidproject/mantid/pull/42380>`_) In the new Instrument View, the contour and integration range controls no longer disappear when every detector has the same counts or the range is empty. They are greyed out until there is a range to adjust.
- (`#42396 <https://github.com/mantidproject/mantid/pull/42396>`_) In the new Instrument View, memory is now freed more effectively when the interface is closed.


SliceViewer
-----------

New features
############

Bugfixes
############
- (`#41780 <https://github.com/mantidproject/mantid/pull/41780>`_) Fixed an issue of :ref:`SliceViewer <sliceviewer>` not showing data in non-orthogonal view when L is selected as one of the project axes by setting proper limits for the axes.

:ref:`Release 7.0.0 <v7.0.0>`

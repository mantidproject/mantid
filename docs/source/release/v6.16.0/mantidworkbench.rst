========================
Mantid Workbench Changes
========================

New Features
------------
- (`#41263 <https://github.com/mantidproject/mantid/pull/41263>`_) Plots can now be normalised by :math:`Q^{-4}` (provided the X dimension is in :math:`Q`) via the right-click menu.


Bugfixes
--------
- (`#41083 <https://github.com/mantidproject/mantid/pull/41083>`_) The documentation of :ref:`Superplot <WorkbenchSuperplot>` is launched when the help button is clicked instead of the Basic 1D and Tiled Plots documentation.
- (`#41303 <https://github.com/mantidproject/mantid/pull/41303>`_) Fixed a bug where error bar settings (CapSize, Cap Thickness, Width, Error Every) from :menuselection:`File --> Settings --> Plots --> Error Bars` were not applied when plotting a spectrum with errors.
- (`#41295 <https://github.com/mantidproject/mantid/pull/41295>`_) When attempting to over plot a Table Workspace onto a colour fill plot, a warning will now be raised instead of an exception.
- (`#40996 <https://github.com/mantidproject/mantid/pull/40996>`_) Help documentation now opens in the system's default web browser instead of an embedded viewer, improving Qt6 compatibility. Removed `qtwebengine` dependency and related code. This change was necessary in order to resolve issues with the render limitations of `qttextbrowser` and to ensure that the help documentation is displayed correctly across different platforms.
- (`#41234 <https://github.com/mantidproject/mantid/pull/41234>`_) The :ref:`Script Editor<main_window>` of Mantid Workbench can now be toggled between the default light and a new dark theme via the General Settings on all platforms (macOS, Windows, Linux).
- (`#41389 <https://github.com/mantidproject/mantid/pull/41389>`_) The :ref:`Messages Window <WorkbenchMessagesWindow>` now prints logs that are more readable in both light and dark mode at all logging levels.
-  Dialog menus will no longer crash when populating algorithms.


InstrumentViewer
----------------

New features
############
- (`#40811 <https://github.com/mantidproject/mantid/pull/40811>`_) New Instrument View now displays the component tree from the IDF, and allows you to select one or more components in that tree.
- (`#41221 <https://github.com/mantidproject/mantid/pull/41221>`_) The New Instrument Viewer now updates the counts data when the integration range has min == max.
- (`#40850 <https://github.com/mantidproject/mantid/pull/40850>`_) In the new Instrument View, added an option to draw detectors using their actual geometric shapes instead of as points.
- (`#40841 <https://github.com/mantidproject/mantid/pull/40841>`_) In the new Instrument View, a new ellipse widget is available for selecting detectors.
- (`#41001 <https://github.com/mantidproject/mantid/pull/41001>`_) In the new Instrument View, added an option to display counts on a log scale.
- (`#40830 <https://github.com/mantidproject/mantid/pull/40830>`_) In the new Instrument View, added reset buttons for the contour and integration range sliders.
- (`#41169 <https://github.com/mantidproject/mantid/pull/41169>`_) In New Instrument Viewer, peak selection no longer requires constantly clicking buttons for adding and deleting peaks. Peak selection now is triggered with a peak selection mode that lets you continually select detectors and add (left mouse click) and delete (right mouse click) peaks without extra clicking or moving the mouse around.
- (`#41054 <https://github.com/mantidproject/mantid/pull/41054>`_) New Instrument View now allows Groupings and Masks to be saved as `.cal` files.
- (`#41437 <https://github.com/mantidproject/mantid/pull/41437>`_) Detector Info box now shows up at the bottom and is hidden when no detectors are selected or when too many detectors (more than 3) are selected. This should reduce the visual clutter on the GUI.
- (`#41184 <https://github.com/mantidproject/mantid/pull/41184>`_) In the new Instrument View, the "Draw Shapes" option is now stored in the Mantid properties file, and the option is applied when the view is opened. This means that if you check "Draw Shapes", close the view, and open it again, it will still be checked and shapes will be drawn. The option is stored under the key "InstrumentView.DrawShapes".
- (`#41348 <https://github.com/mantidproject/mantid/pull/41348>`_) In new Instrument View, flip the projection along the beam axis (from the instrument) instead of the z-axis.
- (`#41315 <https://github.com/mantidproject/mantid/pull/41315>`_) Add the new Instrument View as an option for the ISIS Reflectometry interface.

Bugfixes
############
- (`#41221 <https://github.com/mantidproject/mantid/pull/41221>`_) The New Instrument Viewer no longer crashes when the integration range is edited through text boxes and window is closed.
- (`#40834 <https://github.com/mantidproject/mantid/pull/40834>`_) In new Instrument View, mouse wheel events on the projection and units combo boxes are no longer triggered when the mouse is hovering over them. This prevents the projection and units from being changed unintentionally when scrolling with the mouse wheel.
- (`#41051 <https://github.com/mantidproject/mantid/pull/41051>`_) New Instrument View no longer crashes when a `LOQ` workspace is open and side-by-side projection is selected.
-  In the new Instrument View, fix a bug where an error message could appear if the interface is closed while certain operations were still processing.
- (`#41413 <https://github.com/mantidproject/mantid/pull/41413>`_) Fixed a crash that occurred when masking all detectors in the :ref:`Instrument Viewer <InstrumentViewer>`.


SliceViewer
-----------

New features
############
- (`#41314 <https://github.com/mantidproject/mantid/pull/41314>`_) Updated the :ref:`SliceViewer <sliceviewer>` to support inverted masking using the rectangular, elliptical and polygonal masking selectors.

Bugfixes
############

:ref:`Release 6.16.0 <v6.16.0>`

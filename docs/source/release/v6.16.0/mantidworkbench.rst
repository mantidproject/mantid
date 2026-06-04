========================
Mantid Workbench Changes
========================

New Features
------------
- (`#41234 <https://github.com/mantidproject/mantid/pull/41234>`_) The :ref:`Script Editor <main_window>` of Mantid Workbench can now be toggled between the default *light* and a new *dark* theme via the :ref:`General Settings <GeneralSettings>` on all platforms (macOS, Windows, Linux).
- (`#41263 <https://github.com/mantidproject/mantid/pull/41263>`_) Plots can now be normalised by :math:`Q^{-4}` (provided the X dimension is in :math:`Q`) via the right-click menu.
- (`#40996 <https://github.com/mantidproject/mantid/pull/40996>`_) The Help documentation now opens in the system's default web browser instead of an embedded viewer, improving ``Qt6`` compatibility. The ``qtwebengine`` dependency has been removed. This change was necessary in order to resolve issues with the render limitations of ``qttextbrowser`` and to ensure that the help documentation is displayed correctly across different platforms.


Bugfixes
--------
- (`#41303 <https://github.com/mantidproject/mantid/pull/41303>`_) The :ref:`Error Bars Settings <PlotSettings>` (CapSize, Cap Thickness, Width, Error Every) are now correctly applied when plotting a spectrum with errors.
- (`#41295 <https://github.com/mantidproject/mantid/pull/41295>`_) The :ref:`Colorfill Plots <Colorfill_Plots>` now support over plotting with a ``TableWorkspace``, issuing a warning instead of raising an exception.
- (`#41515 <https://github.com/mantidproject/mantid/pull/41515>`_) The :ref:`Algorithm toolbox <WorkbenchAlgorithmToolbox>` now allows populating algorithms without the previously open dialog menus crashing.
- (`#41389 <https://github.com/mantidproject/mantid/pull/41389>`_) The :ref:`Messages Window <WorkbenchMessagesWindow>` now prints logs that are more readable in both *light* and *dark* mode at all logging levels.
- (`#41083 <https://github.com/mantidproject/mantid/pull/41083>`_) The :ref:`Superplot <WorkbenchSuperplot>` documentation is launched when the plot is ``Superplot`` and the *Help* button is clicked.


InstrumentViewer
----------------

New features
############
- (`#41169 <https://github.com/mantidproject/mantid/pull/41169>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, peak selection no longer requires constantly clicking buttons for adding and deleting peaks. Now peak selection is triggered with a peak selection mode that lets you continually select detectors and add (left mouse click) and delete (right mouse click) peaks without extra clicking or moving the mouse around.
- (`#41437 <https://github.com/mantidproject/mantid/pull/41437>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, Detector Info box now shows up at the bottom and is hidden when no detectors are selected or when too many detectors (more than 3) are selected. This should reduce the visual clutter on the GUI.
- (`#41184 <https://github.com/mantidproject/mantid/pull/41184>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, the "Draw Shapes" option is now stored in the Mantid properties file, and the option is applied when the view is opened. This means that if you check "Draw Shapes", close the view, and open it again, it will still be checked and shapes will be drawn. The option is stored under the key "InstrumentView.DrawShapes".
- (`#40841 <https://github.com/mantidproject/mantid/pull/40841>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, a new ellipse widget for selecting detectors has been added.
- (`#40850 <https://github.com/mantidproject/mantid/pull/40850>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, an option to draw detectors using their actual geometric shapes instead of as points has been added.
- (`#41001 <https://github.com/mantidproject/mantid/pull/41001>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, an option to display counts on a log scale is added.
- (`#40830 <https://github.com/mantidproject/mantid/pull/40830>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, reset buttons for the contour and integration range sliders have been added.
- (`#40811 <https://github.com/mantidproject/mantid/pull/40811>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` now displays the component tree from the IDF and allows you to select one or more components in that tree.
- (`#41221 <https://github.com/mantidproject/mantid/pull/41221>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` now updates the counts data when the integration range has min == max.
- (`#41054 <https://github.com/mantidproject/mantid/pull/41054>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` now allows Groupings and Masks to be saved as `.cal` files.
- (`#41348 <https://github.com/mantidproject/mantid/pull/41348>`_) The projection is flipped in the **new** :ref:`Instrument Viewer <InstrumentViewer>` along the beam axis (from the instrument) instead of the z-axis.
- (`#41315 <https://github.com/mantidproject/mantid/pull/41315>`_) The :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now supports the **new** :ref:`Instrument Viewer <InstrumentViewer>` as an option.

Bugfixes
############
- (`#41201 <https://github.com/mantidproject/mantid/pull/41201>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` exits without an error message when the interface is closed while certain operations are still processing.
- (`#41051 <https://github.com/mantidproject/mantid/pull/41051>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` no longer crashes when a ``LOQ`` workspace is open and side-by-side projection is selected.
- (`#41221 <https://github.com/mantidproject/mantid/pull/41221>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` no longer crashes when the integration range is edited through text boxes and window is closed.
- (`#41413 <https://github.com/mantidproject/mantid/pull/41413>`_) The **new** :ref:`Instrument Viewer <InstrumentViewer>` no longer crashes when masking all detectors.
- (`#40834 <https://github.com/mantidproject/mantid/pull/40834>`_) In the **new** :ref:`Instrument Viewer <InstrumentViewer>`, mouse wheel events on the projection and units combo boxes are no longer triggered when the mouse is hovering over them. This prevents the projection and units from being changed unintentionally when scrolling with the mouse wheel.


SliceViewer
-----------

New features
############
- (`#41314 <https://github.com/mantidproject/mantid/pull/41314>`_) The :ref:`SliceViewer <sliceviewer>` now supports ``inverted`` masking using the *rectangular, elliptical and polygonal* masking selectors.

Bugfixes
############

:ref:`Release 6.16.0 <v6.16.0>`

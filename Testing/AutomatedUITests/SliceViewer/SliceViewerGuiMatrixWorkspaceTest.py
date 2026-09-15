# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for SliceViewer on a MatrixWorkspace.

Replaces the "Basic Usage / MatrixWorkspace" section of
``dev-docs/source/Testing/SliceViewer/SliceViewer.rst`` - the toolbar checklist the rest of the
guide keeps referring back to.

The guide runs the checklist twice, once with an EventWorkspace (``CNCS_7860_event.nxs``) and once
with a Workspace2D (``MAR11060.raw``). It is written here as a mixin holding the checks and two
``TestCase`` subclasses that supply the file, because a class is the unit of skipping - a build with
only one of the two files still reports the other. The mixin deliberately does *not* inherit from
``TestCase``: a base class that did would be collected and run in its own right, with no file.

Two of the guide's steps have no automated equivalent and are recorded in FUTURE_WORK.md rather
than approximated: step 4 (saving the figure, which opens a native file dialog) and step 8 (resizing
the window and judging whether the widgets are "still visible and clear").
"""

import unittest

from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText

from slice_viewer_gui_test_base import EVENT_WORKSPACE_FILE, HISTOGRAM_WORKSPACE_FILE, SliceViewerGuiTestBase
from qt_interaction_helpers import process_events

# the three tools the guide says must be greyed out for a MatrixWorkspace
MD_ONLY_TOOLS = (ToolItemText.OVERLAY_PEAKS, ToolItemText.NONORTHOGONAL_AXES, ToolItemText.NONAXISALIGNEDCUTS)


class _MatrixWorkspaceChecklist:
    """The guide's toolbar checklist, as a mixin over ``SliceViewerGuiTestBase``.

    Subclasses set ``FILENAME`` and ``WORKSPACE`` and inherit every check.
    """

    FILENAME = None
    WORKSPACE = None

    def setUp(self):
        super(_MatrixWorkspaceChecklist, self).setUp()
        self.require_files(self.FILENAME)
        from mantid.simpleapi import Load

        self.workspace = Load(Filename=self.FILENAME, OutputWorkspace=self.WORKSPACE)
        self.presenter = self.open_slice_viewer(self.workspace)

    # ------------------------------------------------------------------ step 2-3

    def test_toolbar_checklist(self):
        self._check_md_only_tools_are_disabled()
        self._check_grid_lines()
        self._check_line_plots()
        self._check_exported_cuts()
        self._check_region_of_interest_tool()

    def _check_md_only_tools_are_disabled(self):
        """Step 2: the Peak overlay, Nonorthogonal view and Non-axis aligned cutting buttons are
        disabled for a MatrixWorkspace."""
        for tool in MD_ONLY_TOOLS:
            with self.subTest(f"MatrixWorkspace / step 2 ({tool} is greyed out)"):
                enabled, _checked = self.toolbar_state(self.presenter, tool)
                self.assertFalse(enabled, f"{tool} should not be available for a MatrixWorkspace")

    def _check_grid_lines(self):
        """Step 3b: toggle grid lines on and off."""
        axes = self.data_view(self.presenter).ax

        self.trigger(self.presenter, ToolItemText.GRID)
        with self.subTest("MatrixWorkspace / step 3b (grid lines can be turned on)"):
            self.assertTrue(self.toolbar_state(self.presenter, ToolItemText.GRID)[1])
            self.assertTrue(any(line.get_visible() for line in axes.get_xgridlines()))

        self.trigger(self.presenter, ToolItemText.GRID)
        with self.subTest("MatrixWorkspace / step 3b (and off again)"):
            self.assertFalse(self.toolbar_state(self.presenter, ToolItemText.GRID)[1])
            self.assertFalse(any(line.get_visible() for line in axes.get_xgridlines()))

    def _check_line_plots(self):
        """Step 3c: enabling line plots adds the two cut axes beside the image."""
        self.trigger(self.presenter, ToolItemText.LINEPLOTS)

        with self.subTest("MatrixWorkspace / step 3c (line plots can be enabled)"):
            self.assertTrue(self.toolbar_state(self.presenter, ToolItemText.LINEPLOTS)[1])

        with self.subTest("MatrixWorkspace / step 3c (the line plot tool is active)"):
            self.assertTrue(self.data_view(self.presenter).line_plots_active)

        with self.subTest("MatrixWorkspace / step 3c (the curves follow the cursor)"):
            self.move_cursor(self.presenter)
            # the tool holds the LinePlots, which owns the two cut axes
            plots = self.data_view(self.presenter).line_plotter.plotter
            self.assertTrue(plots._axx.get_lines(), "no curve was drawn on the X line plot")
            self.assertTrue(plots._axy.get_lines(), "no curve was drawn on the Y line plot")

    def _check_exported_cuts(self):
        """Step 3d: the x, y and c keys export cuts to the ADS.

        The guide then has the tester plot the exported workspaces and check they agree with the
        SliceViewer curves. That comparison is not made here - see FUTURE_WORK.md - but the exported
        workspace is checked to be a single spectrum, which is what a cut is.
        """
        from mantid.api import AnalysisDataService as ADS

        # the pixel line plot tool cuts through wherever the cursor is, and knows nothing until a
        # motion event has told it
        self.move_cursor(self.presenter)

        for key, suffix in (("x", "_cut_x"), ("y", "_cut_y")):
            self.press_key(self.presenter, key)
            with self.subTest(f"MatrixWorkspace / step 3d (the '{key}' key exports a {suffix} workspace)"):
                exported = [name for name in ADS.getObjectNames() if name.endswith(suffix)]
                self.assertTrue(exported, f"no workspace ending {suffix} was created")
                self.assertEqual(1, ADS.retrieve(exported[0]).getNumberHistograms())

        self.press_key(self.presenter, "c")
        with self.subTest("MatrixWorkspace / step 3d (the 'c' key exports both cuts at once)"):
            names = ADS.getObjectNames()
            self.assertTrue(any(name.endswith("_cut_x") for name in names))
            self.assertTrue(any(name.endswith("_cut_y") for name in names))

    def _check_region_of_interest_tool(self):
        """Steps 3e-3g: the ROI tool, the `r` key, and what enabling and disabling it does to the
        line plot button."""
        from mantid.api import AnalysisDataService as ADS

        # step 3e starts from line plots disabled
        if self.toolbar_state(self.presenter, ToolItemText.LINEPLOTS)[1]:
            self.trigger(self.presenter, ToolItemText.LINEPLOTS)

        self.trigger(self.presenter, ToolItemText.REGIONSELECTION)

        with self.subTest("MatrixWorkspace / step 3e (enabling the ROI tool enables line plots too)"):
            self.assertTrue(self.toolbar_state(self.presenter, ToolItemText.REGIONSELECTION)[1])
            self.assertTrue(self.toolbar_state(self.presenter, ToolItemText.LINEPLOTS)[1])

        # step 3f: draw the rectangle before exporting it - the rectangle tool has nothing to export
        # until one has been dragged out
        self.draw_region(self.presenter)
        self.press_key(self.presenter, "r")
        process_events(2)

        with self.subTest("MatrixWorkspace / step 3f (the 'r' key exports a _roi workspace)"):
            exported = [name for name in ADS.getObjectNames() if name.endswith("_roi")]
            self.assertTrue(exported, "no workspace ending _roi was created")

        with self.subTest("MatrixWorkspace / step 3f (the exported ROI lies within the axes)"):
            exported = [name for name in ADS.getObjectNames() if name.endswith("_roi")]
            roi = ADS.retrieve(exported[0])
            x_low, x_high = self.data_view(self.presenter).ax.get_xlim()
            # the guide's "move it off the axes - it should just clip itself"; whatever the region
            # was, what comes back must be inside the data being viewed
            self.assertGreaterEqual(roi.readX(0)[0], x_low - 1e-6)
            self.assertLessEqual(roi.readX(0)[-1], x_high + 1e-6)

        self.trigger(self.presenter, ToolItemText.REGIONSELECTION)

        with self.subTest("MatrixWorkspace / step 3g (disabling the ROI tool leaves line plots on)"):
            self.assertFalse(self.toolbar_state(self.presenter, ToolItemText.REGIONSELECTION)[1])
            self.assertTrue(self.toolbar_state(self.presenter, ToolItemText.LINEPLOTS)[1])

    # ------------------------------------------------------------------ step 5

    def test_colour_bar(self):
        """Step 5: normalisation, scale type, colour map and auto-scale options."""
        from matplotlib.colors import LogNorm, Normalize

        colorbar = self.colorbar(self.presenter)

        with self.subTest("MatrixWorkspace / step 5b (the scale type can be changed to Log)"):
            colorbar.norm.setCurrentText("Log")
            process_events(2)
            self.assertIsInstance(colorbar.get_norm(), LogNorm)

        with self.subTest("MatrixWorkspace / step 5b (and back to Linear)"):
            colorbar.norm.setCurrentText("Linear")
            process_events(2)
            self.assertIsInstance(colorbar.get_norm(), Normalize)

        with self.subTest("MatrixWorkspace / step 5c (the colour map can be changed)"):
            before = self.data_view(self.presenter).image.get_cmap().name
            colorbar.cmap.setCurrentText("plasma")
            process_events(2)
            self.assertNotEqual(before, self.data_view(self.presenter).image.get_cmap().name)

        with self.subTest("MatrixWorkspace / step 5d (the colour map can be reversed)"):
            colorbar.crev.setChecked(True)
            process_events(2)
            self.assertTrue(self.data_view(self.presenter).image.get_cmap().name.endswith("_r"))

        with self.subTest("MatrixWorkspace / step 5e (the auto-scale option can be changed to 3-Sigma)"):
            colorbar.autotype.setCurrentText("3-Sigma")
            process_events(2)
            self.assertEqual("3-Sigma", colorbar.autotype.currentText())

        with self.subTest("MatrixWorkspace / step 5a (with autoscale off the colour limits do not move)"):
            colorbar.autoscale.setChecked(False)
            process_events(2)
            before = self.image_limits(self.presenter)
            self.presenter.normalization_changed("By bin width")
            process_events(2)
            self.assertEqual(before, self.image_limits(self.presenter))

    # ------------------------------------------------------------------ steps 6-7

    def test_transposing_axes(self):
        """Step 6: the Y button transposes the image and relabels the axes."""
        data_view = self.data_view(self.presenter)
        before = (data_view.ax.get_xlabel(), data_view.ax.get_ylabel())

        self.dimensions(self.presenter).dims[0].y.click()
        process_events(3)

        with self.subTest("MatrixWorkspace / step 6 (the axis labels swap over)"):
            after = (data_view.ax.get_xlabel(), data_view.ax.get_ylabel())
            self.assertEqual((before[1], before[0]), after)

    def test_cursor_information(self):
        """Step 7: the cursor information table follows a click when Track Cursor is unticked."""
        data_view = self.data_view(self.presenter)
        table = data_view.image_info_widget

        data_view.track_cursor.setChecked(False)
        process_events(2)

        x_low, x_high = data_view.ax.get_xlim()
        y_low, y_high = data_view.ax.get_ylim()
        self.click_plot(self.presenter, 0.5 * (x_low + x_high), 0.5 * (y_low + y_high))

        with self.subTest("MatrixWorkspace / step 7b (clicking the plot fills the cursor table)"):
            self.assertGreater(table.rowCount(), 0, "the cursor information table stayed empty")
            self.assertGreater(table.columnCount(), 0)

        self.dimensions(self.presenter).dims[0].y.click()
        process_events(3)
        self.click_plot(self.presenter, 0.5 * (x_low + x_high), 0.5 * (y_low + y_high))

        with self.subTest("MatrixWorkspace / step 7c (and still does so after transposing the axes)"):
            self.assertGreater(table.rowCount(), 0)


class SliceViewerGuiEventWorkspaceTest(_MatrixWorkspaceChecklist, SliceViewerGuiTestBase):
    """The toolbar checklist run against an EventWorkspace, as the guide's first case."""

    FILENAME = EVENT_WORKSPACE_FILE
    WORKSPACE = "CNCS_7860_event"


class SliceViewerGuiWorkspace2DTest(_MatrixWorkspaceChecklist, SliceViewerGuiTestBase):
    """The same checklist against a Workspace2D, as the guide's second case."""

    FILENAME = HISTOGRAM_WORKSPACE_FILE
    WORKSPACE = "MAR11060"


if __name__ == "__main__":
    unittest.main()

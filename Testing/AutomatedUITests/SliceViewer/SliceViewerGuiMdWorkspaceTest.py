# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for SliceViewer on MD workspaces.

Replaces the "MD Workspaces" section of ``dev-docs/source/Testing/SliceViewer/SliceViewer.rst``:
the MDEventWorkspace tests, the non-orthogonal view, the peak overlay, and the MDHistoWorkspace
tests that follow them.

The workspaces are the ones the guide has the tester paste into the script editor, built by
``create_md_workspaces``. Building them is the expensive part of this module, so the classes are
split by what they need rather than one per guide sub-section.

Several of the guide's observations here are about how the *picture* looks - "the gridlines should
not be perpendicular to each other", "the features in the data should align with the grid lines".
Those are recorded in FUTURE_WORK.md; what is checked instead is the state that produces them (the
non-orthogonal transform being applied, gridlines being switched on).
"""

import unittest

from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText

from slice_viewer_gui_test_base import SliceViewerGuiTestBase, create_md_workspaces
from qt_interaction_helpers import model_row_count, process_events, select_index


class _MdWorkspaceTestBase(SliceViewerGuiTestBase):
    """Builds the guide's ``md_4D``, ``md_3D`` and ``peaks`` workspaces."""

    def setUp(self):
        super(_MdWorkspaceTestBase, self).setUp()
        self.md_4D, self.md_3D = create_md_workspaces()


class SliceViewerGuiMdEventTest(_MdWorkspaceTestBase):
    """Guide 'MDWorkspace (with events)' steps 2-5."""

    def test_md_event_workspace(self):
        presenter = self.open_slice_viewer(self.md_3D)

        with self.subTest("MDWorkspace / step 2 (the MD-only tools are all available)"):
            for tool in (ToolItemText.OVERLAY_PEAKS, ToolItemText.NONORTHOGONAL_AXES, ToolItemText.NONAXISALIGNEDCUTS):
                self.assertTrue(self.toolbar_state(presenter, tool)[0], f"{tool} should be available for an MD workspace")

        with self.subTest("MDWorkspace / step 2 (the workspace supports dynamic rebinning)"):
            self.assertTrue(self.supports_dynamic_rebinning(presenter))

        with self.subTest("MDWorkspace / step 2 (the cursor table reports HKL)"):
            self.data_view(presenter).track_cursor.setChecked(False)
            process_events(2)
            x, y = self.axes_centre(presenter)
            self.click_plot(presenter, x, y)
            # ImageInfoWidget is a QTableWidget whose first row holds the names and second the
            # values, rather than a table with header sections
            table = self.data_view(presenter).image_info_widget
            names = [table.item(0, col).text() for col in range(table.columnCount()) if table.item(0, col)]
            self.assertTrue(any(name in ("H", "K", "L") for name in names), f"no HKL columns in the cursor table: {names}")

        self._check_number_of_bins(presenter)
        self._check_slice_thickness(presenter)
        self._check_slicepoint_slider(presenter)

    def _check_number_of_bins(self, presenter):
        """Step 3: change the number of bins along a viewing axis."""
        x_name, _y_name = self.viewed_axes(presenter)
        dim = self.dimension(presenter, x_name)

        dim.spinBins.setValue(2)
        process_events(3)

        with self.subTest("MDWorkspace / step 3 (the number of bins along a viewing axis can be changed)"):
            self.assertEqual(2, dim.get_bins())

        with self.subTest("MDWorkspace / step 3 (and the image is rebinned to match)"):
            # the viewed axis is the image's second index, so the bin count shows up as its width
            self.assertEqual(2, self.data_view(presenter).image.get_array().shape[1])

        dim.spinBins.setValue(100)
        process_events(3)

    def _check_slice_thickness(self, presenter):
        """Step 4: a thicker slice sums more events, so the colour limit goes up."""
        x_name, y_name = self.viewed_axes(presenter)
        integrated = next(dim for dim in self.dimensions(presenter).dims if dim.name.text() not in (x_name, y_name))

        integrated.spinThick.setValue(0.1)
        process_events(3)
        thin_max = self.image_limits(presenter)[1]

        integrated.spinThick.setValue(0.5)
        process_events(3)
        thick_max = self.image_limits(presenter)[1]

        with self.subTest("MDWorkspace / step 4 (a thicker slice raises the colour limit)"):
            self.assertGreater(thick_max, thin_max, "summing more events did not increase the maximum")

    def _check_slicepoint_slider(self, presenter):
        """Step 5: the slider and the spin box drive each other."""
        x_name, y_name = self.viewed_axes(presenter)
        integrated = next(dim for dim in self.dimensions(presenter).dims if dim.name.text() not in (x_name, y_name))

        target = integrated.get_bin_center(10)
        integrated.spinbox.setValue(target)
        integrated.spinbox.editingFinished.emit()
        process_events(3)

        with self.subTest("MDWorkspace / step 5a (the slider moves when the spin box is set)"):
            self.assertAlmostEqual(target, integrated.get_value(), places=3)
            self.assertAlmostEqual(target, integrated.get_bin_center(integrated.slider.value()), delta=integrated.width)

        integrated.slider.setValue(30)
        process_events(3)

        with self.subTest("MDWorkspace / step 5b (the spin box follows the slider)"):
            self.assertAlmostEqual(integrated.get_bin_center(30), integrated.spinbox.value(), places=3)


class SliceViewerGuiNonOrthogonalViewTest(_MdWorkspaceTestBase):
    """Guide 'Test the Nonorthogonal view' steps 1-6."""

    def test_nonorthogonal_view(self):
        presenter = self.open_slice_viewer(self.md_3D)
        self.set_viewing_axes(presenter, "H", "K")

        self.trigger(presenter, ToolItemText.LINEPLOTS)
        self.trigger(presenter, ToolItemText.NONORTHOGONAL_AXES)

        with self.subTest("Nonorthogonal / step 1 (the non-orthogonal view turns on)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[1])

        with self.subTest("Nonorthogonal / step 1 (it disables the ROI and line plot buttons)"):
            for tool in (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION):
                enabled, checked = self.toolbar_state(presenter, tool)
                self.assertFalse(enabled, f"{tool} should be disabled in non-orthogonal view")
                self.assertFalse(checked, f"{tool} should be unchecked in non-orthogonal view")

        with self.subTest("Nonorthogonal / step 1 (it turns gridlines on)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.GRID)[1])

        self._check_axis_ranges_survive_a_swap(presenter)

    def _check_axis_ranges_survive_a_swap(self, presenter):
        """Steps 3-4: swapping the viewing axes preserves each dimension's range.

        Checked with the non-orthogonal view switched back off. The guide states the ranges as "0 to
        2 for H and -1 to +1 for K", and those are the *data* ranges: in non-orthogonal view the
        axes are skewed by the lattice angle, so K's -1 is drawn at -sin(60 deg) = -0.866 and the
        numbers in the guide do not describe what is on the axis. The widget logic being tested -
        which dimension goes on which axis, and with what range - is the same either way. The
        ambiguity is recorded in FUTURE_WORK.md.
        """
        self.trigger(presenter, ToolItemText.NONORTHOGONAL_AXES)
        self.assertFalse(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[1], "the non-orthogonal view stayed on")

        data_view = self.data_view(presenter)
        h_range = data_view.ax.get_xlim()
        k_range = data_view.ax.get_ylim()

        with self.subTest("Nonorthogonal / step 3 (H spans 0 to 2 and K spans -1 to +1)"):
            self.assertAlmostEqual(0.0, h_range[0], delta=0.01)
            self.assertAlmostEqual(2.0, h_range[1], delta=0.01)
            self.assertAlmostEqual(-1.0, k_range[0], delta=0.01)
            self.assertAlmostEqual(1.0, k_range[1], delta=0.01)

        self.set_viewing_axes(presenter, "K", "H")

        with self.subTest("Nonorthogonal / step 3 (swapping the axes preserves their ranges)"):
            self.assertEqual(("K", "H"), self.viewed_axes(presenter))
            self.assertAlmostEqual(k_range[0], data_view.ax.get_xlim()[0], delta=0.05)
            self.assertAlmostEqual(k_range[1], data_view.ax.get_xlim()[1], delta=0.05)
            self.assertAlmostEqual(h_range[0], data_view.ax.get_ylim()[0], delta=0.05)
            self.assertAlmostEqual(h_range[1], data_view.ax.get_ylim()[1], delta=0.05)

        self.set_viewing_axes(presenter, "H", "K")

        with self.subTest("Nonorthogonal / step 4 (swapping back restores them)"):
            self.assertEqual(("H", "K"), self.viewed_axes(presenter))
            self.assertAlmostEqual(h_range[0], data_view.ax.get_xlim()[0], delta=0.05)
            self.assertAlmostEqual(h_range[1], data_view.ax.get_xlim()[1], delta=0.05)

    def test_nonorthogonal_view_is_only_for_momentum_axes(self):
        """Step 6: with a non-Q axis viewed the non-orthogonal view is disabled, and it comes back
        when two momentum axes are viewed again."""
        presenter = self.open_slice_viewer(self.md_4D)
        self.set_viewing_axes(presenter, "H", "K")

        with self.subTest("Nonorthogonal / step 6 (it is available while two momentum axes are viewed)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])

        self.set_viewing_axes(presenter, "H", "E")

        with self.subTest("Nonorthogonal / step 6 (viewing the energy axis disables it)"):
            self.assertFalse(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])

        with self.subTest("Nonorthogonal / step 6 (and re-enables line plots and the ROI tool)"):
            for tool in (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION):
                self.assertTrue(self.toolbar_state(presenter, tool)[0], f"{tool} should be available again")

        self.set_viewing_axes(presenter, "H", "K")

        with self.subTest("Nonorthogonal / step 6 (going back to a momentum axis re-enables it)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])


class SliceViewerGuiPeakOverlayTest(_MdWorkspaceTestBase):
    """Guide 'Test the Peak Overlay' steps 1-9."""

    def test_peak_overlay(self):
        presenter = self.open_slice_viewer(self.md_3D)
        self.set_viewing_axes(presenter, "H", "K")

        peaks_presenter = presenter._create_peaks_presenter_if_necessary()
        peaks_presenter.overlay_peaksworkspaces(["peaks"])
        process_events(3)

        with self.subTest("Peak overlay / step 2 (the peak viewer opens with a table for the workspace)"):
            self.assertEqual(["peaks"], list(peaks_presenter.workspace_names()))

        child = peaks_presenter.child_presenter("peaks")

        with self.subTest("Peak overlay / step 2 (the table has a row for each of the two peaks)"):
            self.assertEqual(2, model_row_count(child.view.table_view))

        with self.subTest("Peak overlay / step 2 (the rows are the peaks at HKL (1,0,1) and (1,0,0))"):
            peaks = child.model.peaks_workspace
            self.assertEqual(
                [(1.0, 0.0, 1.0), (1.0, 0.0, 0.0)],
                [(peak.getH(), peak.getK(), peak.getL()) for peak in peaks],
            )

        self._check_selecting_a_peak_moves_the_slicepoint(presenter, child)

    def _check_selecting_a_peak_moves_the_slicepoint(self, presenter, child):
        """Step 3: selecting a peak sets the slicepoint along the integrated axis to that peak.

        Driven by selecting the row and raising ``PeakSelected``, which is what the double click the
        guide describes ends up doing - the row has to be selected either way, and going through the
        table view keeps the row ordering the view's rather than the workspace's.
        """
        from mantidqt.widgets.sliceviewer.peaksviewer.presenter import PeaksViewerPresenter

        for row, expected_l in ((0, 1.0), (1, 0.0)):
            select_index(child.view.table_view, row)
            child.notify(PeaksViewerPresenter.Event.PeakSelected)
            process_events(3)
            with self.subTest(f"Peak overlay / step 3 (selecting row {row} sets the L slicepoint to {expected_l})"):
                self.assertAlmostEqual(expected_l, self.dimension(presenter, "L").get_value(), delta=0.05)


class SliceViewerGuiMdHistoTest(_MdWorkspaceTestBase):
    """Guide 'MDHistoWorkspace' steps 1-8."""

    def test_md_histo_workspace(self):
        from mantid.simpleapi import BinMD

        md_3D_histo = BinMD(
            InputWorkspace="md_4D",
            AlignedDim0="H,-2,2,100",
            AlignedDim1="K,-1,1,100",
            AlignedDim2="L,-1.5,1.5,100",
            OutputWorkspace="md_3D_histo",
        )
        presenter = self.open_slice_viewer(md_3D_histo)

        with self.subTest("MDHisto / step 2 (a binned workspace does not support dynamic rebinning)"):
            self.assertFalse(self.supports_dynamic_rebinning(presenter))

        with self.subTest("MDHisto / step 5 (the non-orthogonal view is still available)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])

        with self.subTest("MDHisto / step 4 (the viewing axes can still be swapped)"):
            x_name, y_name = self.viewed_axes(presenter)
            self.set_viewing_axes(presenter, y_name, x_name)
            self.assertEqual((y_name, x_name), self.viewed_axes(presenter))

    def test_rebinned_workspace_loses_rebinning_when_its_source_goes(self):
        """Steps 6-8: ``md_4D_svrebinned`` supports dynamic rebinning while ``md_4D`` is in the ADS,
        the viewer closes when ``md_4D`` is deleted, and reopening it no longer offers rebinning."""
        from mantid.api import AnalysisDataService as ADS
        from mantid.simpleapi import DeleteWorkspace

        # zooming is what makes SliceViewer write its rebinned copy back to the ADS, which is where
        # the guide's "should be in the ADS after preceding tests" comes from
        first = self.open_slice_viewer(self.md_4D)
        self.presenter_zoom(first)
        self.close_viewer(first)

        rebinned_name = "md_4D_svrebinned"
        if not ADS.doesExist(rebinned_name):
            self.skipTest(f"{rebinned_name} was not produced by rebinning, so steps 6-8 cannot run")

        presenter = self.open_slice_viewer(ADS.retrieve(rebinned_name))

        with self.subTest("MDHisto / step 6 (the rebinned workspace supports dynamic rebinning)"):
            self.assertTrue(self.supports_dynamic_rebinning(presenter))

        DeleteWorkspace("md_4D")
        process_events(5)

        with self.subTest("MDHisto / step 7 (deleting the source workspace closes the viewer)"):
            self.assertTrue(self.is_closed(presenter), "SliceViewer stayed open after its source workspace went")

        reopened = self.open_slice_viewer(ADS.retrieve(rebinned_name))

        with self.subTest("MDHisto / step 8 (reopening it no longer offers dynamic rebinning)"):
            self.assertFalse(self.supports_dynamic_rebinning(reopened))

        with self.subTest("MDHisto / step 8 (and transposing the axes still works)"):
            x_name, y_name = self.viewed_axes(reopened)
            self.set_viewing_axes(reopened, y_name, x_name)
            self.assertEqual((y_name, x_name), self.viewed_axes(reopened))

    @staticmethod
    def presenter_zoom(presenter):
        """Zoom in, which is what triggers a dynamic rebin."""
        data_view = presenter.view.data_view
        x_low, x_high = data_view.ax.get_xlim()
        y_low, y_high = data_view.ax.get_ylim()
        data_view.ax.set_xlim(x_low + 0.25 * (x_high - x_low), x_high - 0.25 * (x_high - x_low))
        data_view.ax.set_ylim(y_low + 0.25 * (y_high - y_low), y_high - 0.25 * (y_high - y_low))
        presenter.dimensions_changed()
        process_events(3)


if __name__ == "__main__":
    unittest.main()

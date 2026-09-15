# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for SliceViewer's "Specific Tests".

Replaces specific tests 1-7 of ``dev-docs/source/Testing/SliceViewer/SliceViewer.rst``: the
representation of integrated peaks, the ADS observers for the peak overlay, for the workspace and
for non-orthogonal support, and the three checks on basis vectors and non-Q axes.

These are the guide's regression tests rather than its walk-through, and most of them are about what
SliceViewer does when the ADS changes underneath it - which is exactly the kind of thing a manual
tester is least likely to notice and most likely to skip.

Specific test 1 asks for judgements about the drawn shapes ("the ellipse should be smaller than the
circle", "a transparent shell", "no gap between the background shell and the dashed line"). What is
checked here is that the representations are drawn at all and that both peak workspaces contribute
one each; the geometric comparisons are recorded in FUTURE_WORK.md.
"""

import unittest

from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText

from slice_viewer_gui_test_base import SliceViewerGuiTestBase, create_md_workspaces
from qt_interaction_helpers import process_events


def integrate_peaks():
    """The guide's specific test 1 snippet: fake ellipsoidal and spherical peaks, integrated three
    ways.

    ``IntegratePeaksMD`` logs an error from ``MaskBTP`` because the simulated workspace has no real
    instrument. The guide says so explicitly and says the integration is executed anyway, so the
    error is expected and is not a failure here either.
    """
    from mantid.simpleapi import FakeMDEventData, IntegratePeaksMD

    md_3D = "md_3D"
    FakeMDEventData(md_3D, EllipsoidParams="1e4,1,0,1,1,0,0,0,1,0,0,0,1,0.005,0.005,0.015,0", RandomSeed="3873875")
    FakeMDEventData(md_3D, EllipsoidParams="1e4,1,0,0,1,0,0,0,1,0,0,0,1,0.005,0.005,0.005,0", RandomSeed="3873875")
    common = dict(
        InputWorkspace=md_3D,
        PeakRadius="0.25",
        PeaksWorkspace="peaks",
        IntegrateIfOnEdge=False,
        UseOnePercentBackgroundCorrection=False,
    )
    IntegratePeaksMD(
        BackgroundInnerRadius="0.25", BackgroundOuterRadius="0.32", OutputWorkspace="peaks_int_ellip", Ellipsoid=True, **common
    )
    IntegratePeaksMD(
        BackgroundInnerRadius="0.25", BackgroundOuterRadius="0.32", OutputWorkspace="peaks_int_sphere", Ellipsoid=False, **common
    )
    IntegratePeaksMD(BackgroundInnerRadius="0", BackgroundOuterRadius="0", OutputWorkspace="peaks_int_no_bg", Ellipsoid=False, **common)


class SliceViewerGuiPeakOverlayObserverTest(SliceViewerGuiTestBase):
    """Specific tests 1 and 2: integrated peak representations, and the peak overlay's ADS observer."""

    def setUp(self):
        super(SliceViewerGuiPeakOverlayObserverTest, self).setUp()
        self.md_4D, self.md_3D = create_md_workspaces()
        integrate_peaks()
        self.presenter = self.open_slice_viewer(self.md_3D)
        self.peaks_presenter = self.presenter._create_peaks_presenter_if_necessary()

    def test_integrated_peak_representations(self):
        """Specific test 1: two integrated peak workspaces overlaid together."""
        self.peaks_presenter.overlay_peaksworkspaces(["peaks_int_ellip", "peaks_int_sphere"])
        process_events(3)

        with self.subTest("Specific test 1 / step 4 (both peak workspaces get a table)"):
            self.assertEqual(["peaks_int_ellip", "peaks_int_sphere"], list(self.peaks_presenter.workspace_names()))

        with self.subTest("Specific test 1 / step 5 (each workspace is drawn in its own colour)"):
            colours = {self.peaks_presenter.child_presenter(name).model.fg_color for name in self.peaks_presenter.workspace_names()}
            self.assertEqual(2, len(colours), "the two peak workspaces were given the same colour")

        with self.subTest("Specific test 1 / step 5 (the peaks are drawn on the plot)"):
            for name in self.peaks_presenter.workspace_names():
                child = self.peaks_presenter.child_presenter(name)
                self.assertTrue(child.model.has_representations_drawn(), f"{name} drew no peak representations")

    def test_renaming_an_overlaid_peaks_workspace(self):
        """Specific test 2 step 1: 'Confirm the name changes in the peak viewer table.'

        This currently fails, and deliberately so - the suite replicates the manual guide rather
        than working around it. ``PeaksViewerCollectionPresenter.rename_handle`` does run (the
        tables are rebuilt, and their order changes), but the peak viewer is left listing the *old*
        name, which no longer exists in the ADS. See FUTURE_WORK.md.

        It is a test of its own so that the deletion half of specific test 2 still runs: the steps
        that follow it in the guide would otherwise be reaching for a table under a name this
        defect has already made wrong.
        """
        from mantid.simpleapi import RenameWorkspace

        self.peaks_presenter.overlay_peaksworkspaces(["peaks_int_ellip", "peaks_int_no_bg"])
        process_events(3)
        self.assertEqual(["peaks_int_ellip", "peaks_int_no_bg"], list(self.peaks_presenter.workspace_names()))

        RenameWorkspace(InputWorkspace="peaks_int_ellip", OutputWorkspace="peaks_int_ellipse")
        process_events(5)

        with self.subTest("Specific test 2 / step 1a (a rename is reflected in the peak viewer)"):
            self.assertIn("peaks_int_ellipse", self.peaks_presenter.workspace_names())

        with self.subTest("Specific test 2 / step 1a (and the old name is gone from it)"):
            self.assertNotIn("peaks_int_ellip", self.peaks_presenter.workspace_names())

    def test_deleting_an_overlaid_peaks_workspace(self):
        """Specific test 2 steps 3-4: deleting the overlaid workspaces one at a time."""
        from mantid.simpleapi import DeleteWorkspace

        self.peaks_presenter.overlay_peaksworkspaces(["peaks_int_ellip", "peaks_int_no_bg"])
        process_events(3)

        DeleteWorkspace("peaks_int_no_bg")
        process_events(5)

        with self.subTest("Specific test 2 / step 3 (deleting a workspace removes its table)"):
            self.assertEqual(["peaks_int_ellip"], list(self.peaks_presenter.workspace_names()))

        DeleteWorkspace("peaks_int_ellip")
        process_events(5)

        with self.subTest("Specific test 2 / step 4 (deleting the last one empties the peak viewer)"):
            self.assertEqual([], list(self.peaks_presenter.workspace_names()))


class SliceViewerGuiWorkspaceObserverTest(SliceViewerGuiTestBase):
    """Specific tests 3 and 4: the ADS observers for the workspace itself."""

    def setUp(self):
        super(SliceViewerGuiWorkspaceObserverTest, self).setUp()
        self.md_4D, self.md_3D = create_md_workspaces()

    def test_workspace_ads_observer(self):
        """Specific test 3: rename, scale, clone and delete the workspace under the viewer."""
        from mantid.api import AnalysisDataService as ADS
        from mantid.simpleapi import CloneWorkspace, DeleteWorkspace, RenameWorkspace, mtd

        presenter = self.open_slice_viewer(self.md_3D)

        RenameWorkspace(InputWorkspace="md_3D", OutputWorkspace="md_3Dim")
        process_events(5)

        with self.subTest("Specific test 3 / step 1 (the window title follows the rename)"):
            self.assertIn("md_3Dim", presenter.view.windowTitle())

        self.colorbar(presenter).autoscale.setChecked(True)
        process_events(2)
        before = self.image_limits(presenter)[1]

        mtd["md_3Dim"] *= 2
        process_events(5)

        with self.subTest("Specific test 3 / step 3 (doubling the data doubles the colour bar maximum)"):
            self.assertAlmostEqual(2.0, self.image_limits(presenter)[1] / before, delta=0.2)

        CloneWorkspace(InputWorkspace="md_3Dim", OutputWorkspace="md_3D")
        self.assertTrue(ADS.doesExist("md_3D"), "the clone was not created")

        DeleteWorkspace("md_3Dim")
        process_events(5)

        with self.subTest("Specific test 3 / step 5 (deleting the workspace closes the viewer)"):
            self.assertTrue(self.is_closed(presenter), "SliceViewer stayed open after its workspace was deleted")

    def test_nonorthogonal_support_ads_observer(self):
        """Specific test 4: clearing the UB removes non-orthogonal support, so the viewer closes."""
        from mantid.simpleapi import ClearUB

        presenter = self.open_slice_viewer(self.md_3D)
        self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0], "the UB was not picked up")

        ClearUB("md_3D")
        process_events(5)

        with self.subTest("Specific test 4 (clearing the UB closes the viewer)"):
            self.assertTrue(self.is_closed(presenter), "SliceViewer stayed open after the UB was cleared")


class SliceViewerGuiBasisVectorTest(SliceViewerGuiTestBase):
    """Specific tests 5, 6 and 7: non-axis-aligned binning and non-Q axes."""

    def test_binmd_without_normalised_basis_vectors_keeps_integer_hkl(self):
        """Specific test 5: peaks put at integer HKL must still be at integer HKL after binning."""
        import numpy as np
        from mantid.simpleapi import BinMD, CreateMDWorkspace, FakeMDEventData, mtd

        ws = CreateMDWorkspace(
            Dimensions="3",
            Extents="-3,3,-3,3,-3,3",
            Names="H,K,L",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
            SplitInto="2",
            SplitThreshold="10",
            OutputWorkspace="ws",
        )
        for h in range(-3, 4):
            for k in range(-3, 4):
                for l_index in range(-3, 4):
                    FakeMDEventData(ws, PeakParams=f"1e+02,{h},{k},{l_index},0.02", RandomSeed="3873875")

        BinMD(
            InputWorkspace=ws,
            AxisAligned=False,
            BasisVector0="[00L],U,0,0,1",
            BasisVector1="[HH0],U,1,1,0",
            BasisVector2="[-HH0],U,-1,1,0",
            OutputExtents="-4,4,-4,4,-0.25,0.25",
            OutputBins="101,101,1",
            OutputWorkspace="BinMD_out",
            NormalizeBasisVectors=False,
        )
        presenter = self.open_slice_viewer(mtd["BinMD_out"])

        with self.subTest("Specific test 5 (the peaks sit at integer HKL, not multiples of root two)"):
            signal = presenter.view.data_view.image.get_array()
            x_low, x_high = presenter.view.data_view.image.get_extent()[:2]
            columns = np.asarray(signal).sum(axis=0)
            brightest = int(np.argmax(columns))
            position = x_low + (brightest + 0.5) * (x_high - x_low) / len(columns)
            self.assertAlmostEqual(position, round(position), delta=0.1, msg=f"the brightest column is at {position}")

    def test_basis_vectors_of_a_non_axis_aligned_mdhisto(self):
        """Specific test 6: a slice taken along [00L] and [HH0] still supports the non-orthogonal
        view, because the interface picks up its basis vectors."""
        from mantid.simpleapi import BinMD, CreateMDWorkspace, CreateSampleWorkspace, SetUB, mtd

        ws = CreateMDWorkspace(
            Dimensions="3",
            Extents="-3,3,-3,3,-3,3",
            Names="H,K,L",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
            SplitInto="2",
            SplitThreshold="10",
            OutputWorkspace="ws",
        )
        expt_info = CreateSampleWorkspace(OutputWorkspace="expt_info")
        ws.addExperimentInfo(expt_info)
        SetUB(ws, 1, 1, 2, 90, 90, 120)
        BinMD(
            InputWorkspace=ws,
            AxisAligned=False,
            BasisVector0="[00L],r.l.u.,0,0,1",
            BasisVector1="[HH0],r.l.u.,1,1,0",
            BasisVector2="[-HH0],r.l.u.,-1,1,0",
            OutputExtents="-4,4,-4,4,-0.25,0.25",
            OutputBins="101,101,1",
            OutputWorkspace="ws_slice",
            NormalizeBasisVectors=False,
        )
        presenter = self.open_slice_viewer(mtd["ws_slice"])

        with self.subTest("Specific test 6 / step 2 (the non-orthogonal view is available)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])

        self.trigger(presenter, ToolItemText.NONORTHOGONAL_AXES)

        with self.subTest("Specific test 6 / step 3 (turning it on switches gridlines on)"):
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[1])
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.GRID)[1])

    def test_nonorthogonal_view_is_disabled_for_a_non_q_axis(self):
        """Specific test 7: with energy as the first dimension the non-orthogonal view is disabled
        while energy is viewed and available again once two momentum axes are."""
        from mantid.simpleapi import CreateMDWorkspace, CreateSampleWorkspace, SetUB

        ws_4D = CreateMDWorkspace(
            Dimensions=4,
            Extents=[-1, 1, -1, 1, -1, 1, -1, 1],
            Names="E,H,K,L",
            Frames="General Frame,HKL,HKL,HKL",
            Units="meV,r.l.u.,r.l.u.,r.l.u.",
            OutputWorkspace="ws_4D",
        )
        expt_info_4D = CreateSampleWorkspace(OutputWorkspace="expt_info_4D")
        ws_4D.addExperimentInfo(expt_info_4D)
        SetUB(ws_4D, 1, 1, 2, 90, 90, 120)

        presenter = self.open_slice_viewer(ws_4D)

        with self.subTest("Specific test 7 / step 3 (viewing the energy axis disables it)"):
            self.set_viewing_axes(presenter, "E", "H")
            self.assertFalse(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])

        with self.subTest("Specific test 7 / step 4 (viewing two momentum axes re-enables it)"):
            self.set_viewing_axes(presenter, "H", "K")
            self.assertTrue(self.toolbar_state(presenter, ToolItemText.NONORTHOGONAL_AXES)[0])


if __name__ == "__main__":
    unittest.main()

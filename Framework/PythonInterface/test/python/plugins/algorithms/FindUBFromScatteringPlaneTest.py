import unittest
from mantid.simpleapi import (
    SetUB,
    CreatePeaksWorkspace,
    FindUBFromScatteringPlane,
    LoadEmptyInstrument,
    CombinePeaksWorkspaces,
    AddPeakHKL,
    ClearUB,
    IndexPeaks,
    AnalysisDataService,
)
import numpy as np
from unittest.mock import patch


class FindUBFromScatteringPlaneTest(unittest.TestCase):
    def setUp(self):
        # load instrument + empty peakworkspace table
        self.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")
        self.peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        self.vertical_dir = self.ws.getInstrument().getReferenceFrame().vecPointingUp()

    def index_peaks_helper(self, peak_ws, tolerance):
        nindexed, *_ = IndexPeaks(peak_ws, tolerance, RoundHKLs=False)
        self.assertEqual(nindexed, 1)

    def up_helper(self, vertical_dir, set_vertical, peak_ws, angle_tol=np.radians(5)):
        lattice = peak_ws.sample().getOrientedLattice()
        res_vertical_angle = lattice.recAngle(*set_vertical, *vertical_dir)
        self.assertLessEqual(np.sin(np.radians(res_vertical_angle)), np.sin(angle_tol))

    def tearDown(self):
        AnalysisDataService.clear()

    def test_find_correct_ub_cubic(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[1, 0.83, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        vertical_dir = peaks1.getInstrument().getReferenceFrame().vecPointingUp()
        self.index_peaks_helper(peaks1, 0.01)
        self.up_helper(vertical_dir, self.vertical_dir, peaks1)

    def test_find_correct_ub_orthorhombic(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[0, 0, 1], v=[1, 0, 0], a=5.395, b=5.451, c=20.530, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [1, 0, 1])
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[0, 0, 1], Vector2=[1, 0, 0], a=5.395, b=5.451, c=20.530, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        vertical_dir = peaks1.getInstrument().getReferenceFrame().vecPointingUp()
        self.index_peaks_helper(peaks1, 0.05)
        self.up_helper(vertical_dir, self.vertical_dir, peaks1)

    def test_find_correct_ub_hexagonal(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[-0.853618, 0, 6.52176], v=[-3.4555, 0, -1.61601], a=4.15, b=4.15, c=6.719, alpha=90, beta=90, gamma=120)
        AddPeakHKL(peaks1, [0, 0, 1])
        ClearUB(peaks1)
        FindUBFromScatteringPlane(
            Vector1=[1, 0, 0], Vector2=[0, 0, 1], a=4.15, b=4.15, c=6.719, alpha=90, beta=90, gamma=120, PeaksWorkspace="peaks1"
        )
        ## call index peak instead
        vertical_dir = peaks1.getInstrument().getReferenceFrame().vecPointingUp()
        self.index_peaks_helper(self.peaks1, 0.01)
        self.up_helper(vertical_dir, self.vertical_dir, peaks1)

    def test_uses_peak_reference_frame_not_workspace_instrument(self):
        r"""
        Regression test: the algorithm must derive 'vertical_dir' from the peak's own
        reference frame (peak.getReferenceFrame()), not from peaks_workspace.getInstrument()
        .getReferenceFrame(). SXD's reference frame has Y pointing up; POLREF's has Z pointing
        up. CombinePeaksWorkspaces clones the LHS workspace (here, an empty POLREF one) for the
        output's own instrument, while each combined peak keeps its own originating instrument
        (here, the SXD peak from RHS) — so the output's getInstrument() and its getPeak(0)
        disagree on which way is "up".

        'vertical_dir' is used internally as the rotation axis that aligns the arbitrary UB's
        calculated Q-sample vector to the peak's observed one; picking the wrong axis (e.g.
        POLREF's Z instead of SXD's Y here) generally cannot converge that alignment to a tight
        tolerance. index_peaks_helper's tight tolerance (0.01) is therefore a real, discriminating
        check here, unlike up_helper (see its use elsewhere in this file), which is a no-op:
        UnitCell.recAngle(v, v) is mathematically always 0 by the identity v^T*Gstar*v = |B*v|^2,
        regardless of the lattice's orientation, so it cannot distinguish a correct UB from a
        wrong one when both arguments come from the same reference frame.
        """
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[1, 0.83, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        ClearUB(peaks1)
        sxd_up = peaks1.getPeak(0).getReferenceFrame().vecPointingUp()

        empty_polref = LoadEmptyInstrument(InstrumentName="POLREF", OutputWorkspace="empty_POLREF")
        polref_peaks = CreatePeaksWorkspace(InstrumentWorkspace=empty_polref, NumberOfPeaks=0, OutputWorkspace="polref_peaks")
        polref_up = polref_peaks.getInstrument().getReferenceFrame().vecPointingUp()
        # sanity check: the two candidate reference frames genuinely disagree
        self.assertFalse(np.allclose(np.array(sxd_up), np.array(polref_up)))

        combined = CombinePeaksWorkspaces(LHSWorkspace=polref_peaks, RHSWorkspace=peaks1, OutputWorkspace="combined")
        self.assertTrue(np.allclose(np.array(combined.getInstrument().getReferenceFrame().vecPointingUp()), np.array(polref_up)))
        self.assertTrue(np.allclose(np.array(combined.getPeak(0).getReferenceFrame().vecPointingUp()), np.array(sxd_up)))

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="combined"
        )
        self.index_peaks_helper(combined, 0.01)

    def test_multiple_peaks_provided(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks2")
        SetUB(peaks2, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks2, [2, 2, 0])
        AddPeakHKL(peaks2, [1, 1, 1])
        ClearUB(peaks1)
        ClearUB(peaks2)

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks2"
        )
        u_vector_1 = abs(np.array(peaks1.sample().getOrientedLattice().getuVector()))
        u_vector_2 = abs(np.array(peaks2.sample().getOrientedLattice().getuVector()))

        self.assertTrue(np.allclose(u_vector_1, u_vector_2, 0.1))

    def test_reverse_inputted_vectors(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        ## need to check how it's tied to output workspace in algorithm
        self.index_peaks_helper(peaks1, 0.15)
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[1, 1, 0], Vector2=[1, -1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        self.index_peaks_helper(peaks1, 0.15)
        ClearUB(peaks1)

    def test_peak_outside_plane_in_tolerance(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0.004])

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )

        self.index_peaks_helper(peaks1, 0.15)

    @patch("mantid.kernel.logger.warning")
    def test_peak_outside_plane_outside_tolerance(self, mock_logger_warning):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [1, 1, 1])

        (
            FindUBFromScatteringPlane(
                Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
            ),
        )
        mock_logger_warning.assert_called_once_with("given peak hkl does not lie in the plane")

    def test_vector_zero(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])

        with self.assertRaisesRegex(RuntimeError, "Vector cannot be 0"):
            FindUBFromScatteringPlane(
                Vector1=[0, 0, 0], Vector2=[1, -1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
            )

    def test_vector_collinear(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        with self.assertRaisesRegex(RuntimeError, "Vectors cannot be collinear"):
            FindUBFromScatteringPlane(
                Vector1=[2, -2, 0], Vector2=[1, -1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
            )


if __name__ == "__main__":
    unittest.main()

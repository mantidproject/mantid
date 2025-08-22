import unittest
from mantid.simpleapi import SetUB, CreatePeaksWorkspace, FindUBFromScatteringPlane, LoadEmptyInstrument, AddPeakHKL, ClearUB, IndexPeaks
import numpy as np
from unittest.mock import patch


class FindUBFromScatteringPlaneTest(unittest.TestCase):
    def setUp(self):
        # load instrument + empty peakworkspace table
        self.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")
        self.peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")

    def assert_vector(self, ref_vector, calculated_vector, tol):
        self.assertTrue(np.allclose(ref_vector, calculated_vector, atol=tol))

    def index_peaks_helper(self, PeaksWorkspace, tolerance):
        nindexed, avg_hkl_er, *_ = IndexPeaks(PeaksWorkspace, tolerance, RoundHKLs=False)
        self.assertLessEqual(avg_hkl_er, 0.5)
        return nindexed

    def test_find_correct_ub_cubic(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[1, -0.83, 0], v=[1, 0.83, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        nindexed = self.index_peaks_helper(peaks1, 0.2)
        self.assertEqual(nindexed, 1)

    def test_find_correct_ub_orthorhombic(self):
        peaks1 = self.peaks1
        SetUB(peaks1, u=[0, 0, 1], v=[1, 0, 0])
        AddPeakHKL(peaks1, [1, 0, 1])
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[0, 0, 1], Vector2=[1, 0, 0], a=5.395, b=5.451, c=20.530, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        nindexed = self.index_peaks_helper(peaks1, 0.45)
        self.assertEqual(nindexed, 1)

    def test_find_correct_ub_hexagonal(self):
        peaks1 = self.peaks1
        SetUB(
            peaks1,
            u=[-0.853618, -0.0212085, 6.52176],
            v=[-3.4555, -0.0651004, -1.61601],
        )
        AddPeakHKL(peaks1, [0, 0, 1])

        ClearUB(peaks1)
        FindUBFromScatteringPlane(
            Vector1=[1, 0, 0], Vector2=[0, 0, 1], a=4.15, b=4.15, c=6.719, alpha=90, beta=90, gamma=120, PeaksWorkspace="peaks1"
        )
        ## call index peak instead
        nindexed = self.index_peaks_helper(self.peaks1, 0.3)
        self.assertEqual(nindexed, 1)

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

        self.assert_vector(u_vector_1, u_vector_2, 0.5)

    def test_reverse_inputted_vectors(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0])
        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        ## need to check how it's tied to output workspace in algorithm
        nindexed = self.index_peaks_helper(peaks1, 0.15)
        self.assertEqual(nindexed, 1)
        ClearUB(peaks1)

        FindUBFromScatteringPlane(
            Vector1=[1, 1, 0], Vector2=[1, -1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )
        nindexed = self.index_peaks_helper(peaks1, 0.15)
        self.assertEqual(nindexed, 1)
        ClearUB(peaks1)

    def test_peak_outside_plane_in_tolerance(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [2, 2, 0.004])

        FindUBFromScatteringPlane(
            Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
        )

        nindexed = self.index_peaks_helper(peaks1, 0.15)
        self.assertEqual(nindexed, 1)

    @patch("mantid.kernel.logger.warning")
    def test_peak_outside_plane_outside_tolerance(self, mock_logger_warning):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
        AddPeakHKL(peaks1, [1, 1, 1])

        (
            FindUBFromScatteringPlane(
                Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace="peaks1"
            ),
        )
        mock_logger_warning.assert_called_once_with("given peak hkl does not lie in the plane")


if __name__ == "__main__":
    unittest.main()

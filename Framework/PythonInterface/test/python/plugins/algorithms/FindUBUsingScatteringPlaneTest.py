import numpy as np
import unittest
from mantid.simpleapi import SetUB, CreatePeaksWorkspace, FindUBUsingScatteringPlane, LoadEmptyInstrument


def add_peakHKL(ws, h, k, L):
    ws.createPeakHKL([float(h), float(k), float(L)])


class FindUBUsingScatteringPlaneTest(unittest.TestCase):
    def setUp(self):
        # load empty instrument so can create a peak table
        self.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")

    def assert_matrix(self, ref_mat, extracted_mat, delta):
        mat = extracted_mat
        for ii in range(0, 3):
            for jj in range(0, 3):
                self.assertAlmostEqual(mat[ii, jj], ref_mat[ii, jj], delta=delta)

    def assert_vector(self, ref_vector, calculated_vector, tol):
        for i in range(0, 3):
            self.assertAlmostEqual(ref_vector[i], calculated_vector[i], delta=tol)

    def test_find_correct_ub(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        UB = np.array([[0.11530531, 0.14359483, 0.00093547], [-0.00244413, 0.00076298, 0.18414426], [0.14357708, -0.11530658, 0.00238344]])
        add_peakHKL(peaks1, 2, 2, 0)
        SetUB(peaks1, UB=UB)
        print(peaks1.sample().getOrientedLattice().getUB())
        FindUBUsingScatteringPlane(
            vector_1=[1, -1, 0], vector_2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeakWorkspace="peaks1"
        )
        ## need to check how it's tied to output workspace in algorithm
        self.assert_matrix(UB, peaks1.sample().getOrientedLattice().getUB(), 0.05)

    def test_multiple_peaks_provided(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        UB = np.array([[0.11530531, 0.14359483, 0.00093547], [-0.00244413, 0.00076298, 0.18414426], [0.14357708, -0.11530658, 0.00238344]])
        add_peakHKL(peaks1, 2, 2, 0)
        add_peakHKL(peaks1, 1, 1, 1)
        SetUB(peaks1, UB=UB)
        FindUBUsingScatteringPlane(
            vector_1=[1, -1, 0], vector_2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeakWorkspace="peaks1"
        )
        ## need to check how it's tied to output workspace in algorithm
        self.assert_matrix(UB, peaks1.sample().getOrientedLattice().getUB(), 0.05)

    def test_no_scattering_plane_provided(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        UB = np.array([[0.11530531, 0.14359483, 0.00093547], [-0.00244413, 0.00076298, 0.18414426], [0.14357708, -0.11530658, 0.00238344]])
        add_peakHKL(peaks1, 2, 2, 0)
        add_peakHKL(peaks1, 1, 1, 1)
        SetUB(peaks1, UB=UB)
        self.assertRaises(
            ValueError,
            FindUBUsingScatteringPlane(
                vector_1=[1, -1, 0], vector_2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeakWorkspace="peaks1"
            ),
        )

    def test_reverse_inputted_vectors(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
        UB = np.array([[0.11530531, 0.14359483, 0.00093547], [-0.00244413, 0.00076298, 0.18414426], [0.14357708, -0.11530658, 0.00238344]])
        add_peakHKL(peaks1, 2, 2, 0)
        SetUB(peaks1, UB=UB)
        ref_uVector = peaks1.sample().getOrientedLattice().getuVector()
        FindUBUsingScatteringPlane(
            vector_1=[1, 1, 0], vector_2=[1, -1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeakWorkspace="peaks1"
        )
        ## need to check how it's tied to output workspace in algorithm
        self.assert_vector(ref_uVector, peaks1.sample().getOrientedLattice().getuVector(), 0.5)


if __name__ == "__main__":
    unittest.main()


## v1,v2 make negative axis

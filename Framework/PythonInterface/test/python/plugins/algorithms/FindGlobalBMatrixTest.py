import unittest
from testhelpers import create_algorithm
from mantid.simpleapi import (
    FindGlobalBMatrix,
    AnalysisDataService,
    SetGoniometer,
    SetUB,
    ClearUB,
    CreatePeaksWorkspace,
    LoadEmptyInstrument,
    CloneWorkspace,
    TransformHKL,
)
import numpy as np
from plugins.algorithms.FindGoniometerFromUB import getR


def add_peaksHKL(ws_list, Hs, Ks, L):
    for h in Hs:
        for k in Ks:
            for peaks in ws_list:
                pk = peaks.createPeakHKL([h, k, L])
                peaks.addPeak(pk)


def getBMatrix(ws):
    return ws.sample().getOrientedLattice().getB()


def getUMatrix(ws):
    return ws.sample().getOrientedLattice().getU()


class FindGlobalBMatrixTest(unittest.TestCase):
    def setUp(self):
        # load empty instrument so can create a peak table
        self.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")

    def tearDown(self):
        AnalysisDataService.clear()

    def test_finds_average_lattice_parameter(self):
        # create two peak tables with UB corresponding to different lattice constant, a
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks1")
        UB = np.diag([1.0 / 3.9, 0.25, 0.1])  # alatt = [3.9, 4, 10]
        SetUB(peaks1, UB=UB)
        peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks2")
        UB = np.diag([1.0 / 4.1, 0.25, 0.1])  # alatt = [4.1, 4, 10]
        SetUB(peaks2, UB=UB)
        # Add some peaks
        add_peaksHKL([peaks1, peaks2], range(0, 3), range(0, 3), 4)

        FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15)

        # check lattice  - should have average a=4.0
        self.assert_lattice([peaks1, peaks2], 4.0, 4.0, 10.0, 90.0, 90.0, 90.0, delta_latt=5e-2, delta_angle=2.5e-1)
        self.assert_matrix([peaks1], getBMatrix(peaks2), getBMatrix, delta=1e-10)  # should have same B matrix
        self.assert_matrix([peaks1, peaks2], np.eye(3), getUMatrix, delta=5e-2)

    def test_handles_inaccurate_goniometer(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks3")
        peaks2 = CloneWorkspace(InputWorkspace=peaks1, OutputWorkspace="SXD_peaks4")
        # set different gonio on each run
        rot = 5
        SetGoniometer(Workspace=peaks1, Axis0=f"{-rot},0,1,0,1")
        SetGoniometer(Workspace=peaks2, Axis0=f"{rot},0,1,0,1")
        # Add peaks at QLab corresponding to slightly different gonio rotations
        UB = np.diag([0.25, 0.25, 0.1])  # alatt = [4,4,10]
        for h in range(0, 3):
            for k in range(0, 3):
                hkl = np.array([h, k, 4])
                qlab = 2 * np.pi * np.matmul(np.matmul(getR(-(rot + 1), [0, 1, 0]), UB), hkl)
                pk = peaks1.createPeak(qlab)
                peaks1.addPeak(pk)
                qlab = 2 * np.pi * np.matmul(np.matmul(getR(rot + 1, [0, 1, 0]), UB), hkl)
                pk = peaks2.createPeak(qlab)
                peaks2.addPeak(pk)

        FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.15, b=3.95, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15)

        # check lattice - shouldn't be effected by error in goniometer
        self.assert_lattice([peaks1, peaks2], 4.0, 4.0, 10.0, 90.0, 90.0, 90.0, delta_latt=2e-2, delta_angle=2.5e-1)
        self.assert_matrix([peaks1], getBMatrix(peaks2), getBMatrix, delta=1e-10)  # should have same B matrix
        self.assert_matrix([peaks1, peaks2], np.eye(3), getUMatrix, delta=5e-2)

    def test_requires_more_than_one_peak_workspace(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks4")
        UB = np.diag([0.25, 0.25, 0.1])
        SetUB(peaks1, UB=UB)
        # Add some peaks
        add_peaksHKL([peaks1], range(0, 3), range(0, 3), 4)

        alg = create_algorithm(
            "FindGlobalBMatrix", PeakWorkspaces=[peaks1], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15
        )

        with self.assertRaisesRegex(RuntimeError, "Accept only PeaksWorkspaces with either:*"):
            alg.execute()

    def test_peak_workspaces_need_at_least_six_peaks_each_without_ub(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks5")
        UB = np.diag([0.25, 0.25, 0.1])
        SetUB(peaks1, UB=UB)
        # Add 5 peaks
        add_peaksHKL([peaks1], range(0, 5), [0], 4)
        peaks2 = CloneWorkspace(InputWorkspace=peaks1, OutputWorkspace="SXD_peaks6")
        ClearUB(peaks2)

        alg = create_algorithm(
            "FindGlobalBMatrix", PeakWorkspaces=[peaks1, peaks2], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15
        )

        with self.assertRaisesRegex(RuntimeError, "Accept only PeaksWorkspaces with either:*"):
            alg.execute()

    def test_peak_workspaces_need_at_least_two_peaks_each_with_ub(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks7")
        UB = np.diag([0.25, 0.25, 0.1])
        SetUB(peaks1, UB=UB)
        peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks8")
        SetUB(peaks2, UB=UB)
        # Add 5 peaks
        add_peaksHKL([peaks1], range(0, 5), [0], 4)
        add_peaksHKL([peaks2], range(0, 1), [0], 4)

        alg = create_algorithm(
            "FindGlobalBMatrix", PeakWorkspaces=[peaks1, peaks2], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15
        )

        with self.assertRaisesRegex(RuntimeError, "Accept only PeaksWorkspaces with either:*"):
            alg.execute()

    def test_performs_correct_transform_to_ensure_consistent_indexing(self):
        # create peaks tables
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks9")
        UB = np.diag([0.2, 0.25, 0.1])
        SetUB(peaks1, UB=UB)
        # Add some peaks
        add_peaksHKL([peaks1], range(0, 3), range(0, 3), 4)
        # Clone ws and transform
        peaks2 = CloneWorkspace(InputWorkspace=peaks1, OutputWorkspace="SXD_peaks10")
        peaks2.removePeak(0)  # peaks1 will have most peaks indexed so will used as reference
        transform = np.array([[0, 1, 0], [1, 0, 0], [0, 0, -1]])
        TransformHKL(PeaksWorkspace=peaks2, HKLTransform=transform, FindError=False)

        FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.15, b=3.95, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15)

        # check lattice - shouldn't be effected by error in goniometer
        self.assert_lattice([peaks1, peaks2], 5.0, 4.0, 10.0, 90.0, 90.0, 90.0, delta_latt=5e-2, delta_angle=2.5e-1)
        self.assert_matrix([peaks1], getBMatrix(peaks2), getBMatrix, delta=1e-10)  # should have same B matrix
        self.assert_matrix([peaks1, peaks2], np.eye(3), getUMatrix, delta=5e-2)

    def test_any_workspaces_with_no_peaks_are_excluded(self):
        # Setup

        SetGoniometer(Workspace=self.ws, Axis0="-5,0,1,0,1")
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks11")
        UB = np.diag([0.3333, 0.25, 0.09091])  # alatt = [3,4,11]
        SetUB(peaks, UB=UB)

        peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks12")
        UB = np.diag([0.30, 0.25, 0.1])  # alatt = [3.33,4,10]
        SetUB(peaks2, UB=UB)

        SetGoniometer(Workspace=self.ws, Axis0="5,0,1,0,1")
        peaks3 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks13")
        UB = np.diag([0.3333, 0.25, 0.09091])  # alatt = [3,4,11]
        SetUB(peaks3, UB=UB)

        peaks_list = [peaks, peaks2, peaks3]
        for K in range(1, 6):
            for L in range(1, 6):
                for peak_ws in peaks_list:
                    pk = peak_ws.createPeakHKL([2, K, L])
                    if pk.getDetectorID() > 0:
                        peak_ws.addPeak(pk)

        FindGlobalBMatrix(PeakWorkspaces=peaks_list, a=3, b=4, c=11, alpha=90, beta=90, gamma=90, Tolerance=0.15)

        self.assertWarnsRegex(RuntimeWarning, "Workspace 1 removed")

    def assert_lattice(self, ws_list, a, b, c, alpha, beta, gamma, delta_latt=2e-2, delta_angle=2.5e-1):
        for ws in ws_list:
            cell = ws.sample().getOrientedLattice()
            self.assertAlmostEqual(cell.a(), a, delta=delta_latt)
            self.assertAlmostEqual(cell.b(), b, delta=delta_latt)
            self.assertAlmostEqual(cell.c(), c, delta=delta_latt)
            self.assertAlmostEqual(cell.alpha(), alpha, delta=delta_angle)
            self.assertAlmostEqual(cell.beta(), beta, delta=delta_angle)
            self.assertAlmostEqual(cell.gamma(), gamma, delta=delta_angle)

    def assert_matrix(self, ws_list, ref_mat, extract_mat_func, delta=5e-2):
        for ws in ws_list:
            mat = extract_mat_func(ws)
            for ii in range(0, 3):
                for jj in range(0, 3):
                    self.assertAlmostEqual(mat[ii, jj], ref_mat[ii, jj], delta=delta)


if __name__ == "__main__":
    unittest.main()

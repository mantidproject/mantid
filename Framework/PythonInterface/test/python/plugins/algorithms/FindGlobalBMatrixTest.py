import unittest
from testhelpers import create_algorithm
from mantid.simpleapi import (FindGlobalBMatrix, AnalysisDataService, SetGoniometer, SetUB, CreatePeaksWorkspace,
                              LoadEmptyInstrument, CloneWorkspace)
import numpy as np
from FindGoniometerFromUB import getR

class FindGlobalBMatrixTest(unittest.TestCase):

    def setUp(self):
        # laod empty instrument so can create a peak table
        self.ws = LoadEmptyInstrument(InstrumentName='SXD', OutputWorkspace='empty_SXD')
        axis = self.ws.getAxis(0)
        axis.setUnit("TOF")

    def tearDown(self):
        AnalysisDataService.clear()

    def test_finds_average_lattice_parameter(self):
        # create two peak tables with UB corresponding to different lattice constant, a
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks1")
        UB = np.diag([1.0/3.8, 0.25, 0.1])  # alatt = [3.8, 4, 10]
        SetUB(peaks1, UB=UB)
        peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks2")
        UB = np.diag([1.0/4.2, 0.25, 0.1])  # alatt = [4.2, 4, 10]
        SetUB(peaks2, UB=UB)
        # Add some peaks
        for h in range(0, 3):
            for k in range(0, 3):
                pk = peaks1.createPeakHKL([h, k, 4])
                peaks1.addPeak(pk)
                pk = peaks2.createPeakHKL([h, k, 4])
                peaks2.addPeak(pk)

        FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89,
                          Tolerance=0.15)

        # check lattice - should have average a=4.0
        self.assert_lattice([peaks1, peaks2], 4.0, 4.0, 10.0, 90.0, 90.0, 90.0, delta_latt=2e-2, delta_angle=2.5e-1)

    def test_handles_inaccurate_goniometer(self):
        # create two peak tables with UB corresponding to different lattice constant, a
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks3")
        peaks2 = CloneWorkspace(InputWorkspace=peaks1, OutputWorkspace="SXD_peaks4")
        # set different gonio on each run
        rot = 5
        SetGoniometer(Workspace=peaks1, Axis0=f'{-rot},0,1,0,1')
        SetGoniometer(Workspace=peaks2, Axis0=f'{rot},0,1,0,1')
        # Add peaks at QLab corresponding to slightly different gonio rotations
        UB = np.diag([0.25, 0.25, 0.1])  # alatt = [4,4,10]
        for h in range(0, 3):
            for k in range(0, 3):
                hkl = np.array([h, k, 4])
                qlab = 2 * np.pi * np.matmul(np.matmul(getR(-(rot+1), [0, 1, 0]), UB), hkl)
                pk = peaks1.createPeak(qlab)
                peaks1.addPeak(pk)
                qlab = 2 * np.pi * np.matmul(np.matmul(getR(rot+1, [0, 1, 0]), UB), hkl)
                pk = peaks2.createPeak(qlab)
                peaks2.addPeak(pk)

        FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.15, b=3.95, c=10, alpha=88, beta=88, gamma=89,
                          Tolerance=0.15)

        # check lattice - shouldn't be effected by error in goniometer
        self.assert_lattice([peaks1, peaks2], 4.0, 4.0, 10.0, 90.0, 90.0, 90.0, delta_latt=2e-2, delta_angle=2.5e-1)

    def test_requires_more_than_one_peak_workspace(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks4")
        UB = np.diag([1.0 / 3.8, 0.25, 0.1])  # alatt = [3.8, 4, 10]
        SetUB(peaks1, UB=UB)
        # Add some peaks
        for h in range(0, 3):
            for k in range(0, 3):
                pk = peaks1.createPeakHKL([h, k, 4])
                peaks1.addPeak(pk)

        alg = create_algorithm('FindGlobalBMatrix', PeakWorkspaces=[peaks1],
                               a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15)

        with self.assertRaises(RuntimeError):
            alg.execute()

    def test_peak_workspaces_need_at_least_six_peaks(self):
        peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace="SXD_peaks5")
        UB = np.diag([1.0 / 3.8, 0.25, 0.1])  # alatt = [3.8, 4, 10]
        SetUB(peaks1, UB=UB)
        # Add some peaks
        for h in range(0, 5):
            pk = peaks1.createPeakHKL([h, 0, 4])
            peaks1.addPeak(pk)
        peaks2 = CloneWorkspace(InputWorkspace=peaks1, OutputWorkspace="SXD_peaks6")

        alg = create_algorithm('FindGlobalBMatrix', PeakWorkspaces=[peaks1, peaks2],
                               a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, Tolerance=0.15)

        with self.assertRaises(RuntimeError):
            alg.execute()


    def assert_lattice(self, ws_list, a, b, c, alpha=90, beta=90, gamma=90, delta_latt=2e-2, delta_angle=2.5e-1):
        for ws in ws_list:
            cell = ws.sample().getOrientedLattice()
            self.assertAlmostEqual(cell.a(), a, delta=delta_latt)
            self.assertAlmostEqual(cell.b(), b, delta=delta_latt)
            self.assertAlmostEqual(cell.c(), c, delta=delta_latt)
            self.assertAlmostEqual(cell.alpha(), alpha, delta=delta_angle)
            self.assertAlmostEqual(cell.beta(), beta, delta=delta_angle)
            self.assertAlmostEqual(cell.gamma(), gamma, delta=delta_angle)

if __name__ == '__main__':
    unittest.main()

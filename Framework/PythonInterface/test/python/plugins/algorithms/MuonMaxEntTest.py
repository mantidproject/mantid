# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math
import unittest
from mantid.simpleapi import *
from mantid.api import *


def genData():
    x_data = np.linspace(0, 30.0, 100)
    e_data = np.cos(0.0 * x_data)
    y_data = np.sin(2.3 * x_data + 0.1) * np.exp(-x_data / 2.19703)
    inputData = CreateWorkspace(DataX=x_data, DataY=y_data, DataE=e_data, UnitX="Time", StoreInADS=False)
    return inputData


def genData2():
    x_data = np.linspace(0, 30.0, 100)
    e_data = []
    y_data = []
    for xx in x_data:
        y_data.append(np.sin(2.3 * xx + 0.1) * np.exp(-xx / 2.19703))
        e_data.append(np.cos(0.2 * xx))
    for xx in x_data:
        y_data.append(np.sin(4.3 * xx + 0.2) * np.exp(-xx / 2.19703))
        e_data.append(np.cos(0.2 * xx))

    inputData = CreateWorkspace(DataX=x_data, DataY=y_data, DataE=e_data, NSpec=2, UnitX="Time", StoreInADS=False)
    return inputData


def genDataWithDeadDetectors():
    inputData = CreateSimulationWorkspace("MUSR", "0,1,32")
    xData = (inputData.dataX(0)[1:] + inputData.dataX(0)[:-1]) / 2.0
    for j in range(inputData.getNumberHistograms()):
        if j == 42 or j == 24:
            inputData.dataY(j)[:] = np.zeros(len(xData))
            inputData.dataE(j)[:] = np.cos(0.3 * xData)
        else:
            phi = 2.0 * math.pi * (j + 1) / (inputData.getNumberHistograms() + 1)
            inputData.dataY(j)[:] = np.sin(2.3 * xData + phi)
            inputData.dataE(j)[:] = np.cos(0.3 * xData)
    return inputData


class MuonMaxEntTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._gen_data = genData()
        cls._gen_data2 = genData2()
        cls._gen_data_with_dead_detectors = genDataWithDeadDetectors()

    @classmethod
    def tearDownClass(cls) -> None:
        AnalysisDataService.clear()

    def test_executes(self):
        MuonMaxent(
            InputWorkspace=self._gen_data,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=True,
            OuterIterations=1,
            InnerIterations=1,
            OutputWorkspace="freq",
            ReconstructedSpectra="time",
            OutputPhaseTable="phase",
        )
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(), 1)
        self.assertEqual(time.getNumberHistograms(), 1)
        self.assertEqual(phase.rowCount(), 1)

    def test_deadDetectors(self):
        # check input data has 2 dead detectors
        for k in range(self._gen_data_with_dead_detectors.getNumberHistograms()):
            if k == 24 or k == 42:
                self.assertEqual(np.count_nonzero(self._gen_data_with_dead_detectors.readY(k)), 0)
            else:
                self.assertNotEqual(np.count_nonzero(self._gen_data_with_dead_detectors.readY(k)), 0)
        MuonMaxent(
            InputWorkspace=self._gen_data_with_dead_detectors,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=False,
            OuterIterations=5,
            InnerIterations=5,
            OutputWorkspace="freq",
            ReconstructedSpectra="time",
            OutputPhaseTable="phase",
            PhaseConvergenceTable="con",
        )
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        con = AnalysisDataService.retrieve("con")
        self.assertEqual(freq.getNumberHistograms(), 1)
        for j in range(time.getNumberHistograms()):
            if j == 42 or j == 24:
                self.assertEqual(np.count_nonzero(time.readY(j)), 0)
                self.assertEqual(np.count_nonzero(con.readY(j)), 0)
                self.assertEqual(phase.cell(j, 2), 0.0)
                self.assertEqual(phase.cell(j, 1), 999.0)
            else:
                self.assertNotEqual(np.count_nonzero(time.readY(j)), 0)
                self.assertNotEqual(np.count_nonzero(con.readY(j)), 0)
                self.assertNotEqual(phase.cell(j, 1), 999.0)

        self.assertEqual(phase.rowCount(), 64)

    def test_raises_when_stuck_looping_due_to_nan_values(self):
        with self.assertRaisesRegex(RuntimeError, "MuonMaxent-v1: invalid value: a=418.1113271717654 b=nan c=nan"):
            MuonMaxent(
                InputWorkspace=self._gen_data,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=10,
                InnerIterations=10,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_multipleSpec(self):
        MuonMaxent(
            InputWorkspace=self._gen_data2,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=True,
            OuterIterations=1,
            InnerIterations=1,
            OutputWorkspace="freq",
            ReconstructedSpectra="time",
            OutputPhaseTable="phase",
        )
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(), 1)
        self.assertEqual(time.getNumberHistograms(), 2)
        self.assertEqual(phase.rowCount(), 2)

    def test_raises_when_start_is_greater_than_end(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                FirstGoodTime=10.0,
                LastGoodTime=1.0,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_start_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                FirstGoodTime=-10.0,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_inner_iterations_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=0,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_outer_iterations_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=0,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_maxfield_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                Npts=32768,
                MaxField=-10.0,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_factor_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                Npts=32768,
                Factor=-10.0,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_defaultlevel_is_less_than_zero(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._gen_data2,
                Npts=32768,
                DefaultLevel=-10.0,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )


if __name__ == "__main__":
    unittest.main()

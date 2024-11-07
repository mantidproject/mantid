# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math
import unittest
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSimulationWorkspace, CreateWorkspace, MuonMaxent


def create_workspace():
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


def create_workspace_with_dead_detectors():
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
        cls._workspace = create_workspace()
        cls._workspace_with_dead_detectors = create_workspace_with_dead_detectors()

    @classmethod
    def tearDownClass(cls) -> None:
        AnalysisDataService.clear()

    def test_executes_on_multiple_spectra(self):
        MuonMaxent(
            InputWorkspace=self._workspace,
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

    def test_deadDetectors(self):
        # check input data has 2 dead detectors
        for k in range(self._workspace_with_dead_detectors.getNumberHistograms()):
            if k == 24 or k == 42:
                self.assertEqual(np.count_nonzero(self._workspace_with_dead_detectors.readY(k)), 0)
            else:
                self.assertNotEqual(np.count_nonzero(self._workspace_with_dead_detectors.readY(k)), 0)
        MuonMaxent(
            InputWorkspace=self._workspace_with_dead_detectors,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=False,
            OuterIterations=1,
            InnerIterations=1,
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
        with self.assertRaisesRegex(RuntimeError, "MuonMaxent-v1: invalid value: a=[0-9]*.[0-9]* b=nan c=nan"):
            MuonMaxent(
                InputWorkspace=self._workspace,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=2,
                InnerIterations=1,
                OutputWorkspace="freq",
                ReconstructedSpectra="time",
                OutputPhaseTable="phase",
            )

    def test_raises_when_start_is_greater_than_end(self):
        with self.assertRaisesRegex(RuntimeError, "Some invalid Properties found"):
            MuonMaxent(
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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
                InputWorkspace=self._workspace,
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

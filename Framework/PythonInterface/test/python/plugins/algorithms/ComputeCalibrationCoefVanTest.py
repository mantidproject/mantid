# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    DeleteWorkspace,
    CreateSampleWorkspace,
    AddSampleLog,
    AddTimeSeriesLog,
    EditInstrumentGeometry,
    CloneWorkspace,
    CompareWorkspaces,
    FindEPP,
    SetInstrumentParameter,
)
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from scipy.constants import N_A, hbar, k
import numpy as np


class ComputeCalibrationCoefVanTest(unittest.TestCase):
    def setUp(self):
        input_ws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=LinearBackground, " + "A0=0.3;name=Gaussian, PeakCentre=5, Height=10, Sigma=0.3",
            NumBanks=2,
            BankPixelWidth=1,
            XMin=0,
            XMax=10,
            BinWidth=0.1,
            BankDistanceFromSample=4.0,
        )
        self._input_ws = input_ws
        self._table = FindEPP(input_ws, OutputWorkspace="table")
        AddSampleLog(self._input_ws, LogName="wavelength", LogText="4.0", LogType="Number", LogUnit="Angstrom")
        for i in range(input_ws.getNumberHistograms()):
            y = input_ws.dataY(i)
            y.fill(0.0)
            y[51] = 100.0
            e = input_ws.dataE(i)
            e.fill(0.0)
            e[51] = 10.0

    def test_output(self):
        outputWorkspaceName = "output_ws"
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        # Output = Vanadium ws
        self.assertEqual(wsoutput.getRun().getLogData("run_title").value, self._input_ws.getRun().getLogData("run_title").value)

        # Size of output workspace
        self.assertEqual(wsoutput.getNumberHistograms(), self._input_ws.getNumberHistograms())

        DeleteWorkspace(wsoutput)
        return

    def test_sum(self):
        outputWorkspaceName = "output_ws"
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        for i in range(wsoutput.getNumberHistograms()):
            self.assertEqual(100.0, wsoutput.readY(i)[0])
            self.assertEqual(10.0, wsoutput.readE(i)[0])

        DeleteWorkspace(wsoutput)

    def test_dwf_using_default_temperature(self):
        outputWorkspaceName = "output_ws"

        # change theta to make dwf != 1
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        self._checkDWF(wsoutput, 293.0)

        DeleteWorkspace(wsoutput)

    def test_temperature_from_sample_log(self):
        self._input_ws.mutableRun().addProperty("temperature", 0.0, True)
        outputWorkspaceName = "output_ws"
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        self._checkDWF(wsoutput, 0.0)

        DeleteWorkspace(wsoutput)

    def test_temperature_log_is_time_series(self):
        outputWorkspaceName = "output_ws"
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        AddTimeSeriesLog(self._input_ws, "temperature", "2010-09-14T04:20:12", Value="0.0")
        AddTimeSeriesLog(self._input_ws, "temperature", "2010-09-14T04:20:13", Value="0.0")
        AddTimeSeriesLog(self._input_ws, "temperature", "2010-09-14T04:20:14", Value="0.0")
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        self._checkDWF(wsoutput, 0.0)

    def test_temperature_log_name_from_IPF(self):
        self._input_ws.mutableRun().addProperty("sample.temperature", 0.0, True)
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        SetInstrumentParameter(
            Workspace=self._input_ws, ParameterName="temperature_log_entry", ParameterType="String", Value="sample.temperature"
        )
        outputWorkspaceName = "output_ws"
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        self._checkDWF(wsoutput, 0.0)

    def test_temperature_input_overrides_sample_log(self):
        self._input_ws.mutableRun().addProperty("temperature", 567.0, True)
        outputWorkspaceName = "output_ws"
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan",
            VanadiumWorkspace=self._input_ws,
            EPPTable=self._table,
            OutputWorkspace=outputWorkspaceName,
            Temperature=0.0,
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)

        self._checkDWF(wsoutput, 0.0)

        DeleteWorkspace(wsoutput)

    def test_input_not_modified(self):
        backup = CloneWorkspace(self._input_ws)
        outputWorkspaceName = "output_ws"
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan", VanadiumWorkspace=self._input_ws, EPPTable=self._table, OutputWorkspace=outputWorkspaceName
        )
        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(CompareWorkspaces(backup, self._input_ws)[0])
        DeleteWorkspace(backup)

    def test_disabled_debye_waller_correction(self):
        outputWorkspaceName = "output_ws"

        # change theta to make dwf != 1
        EditInstrumentGeometry(self._input_ws, L2="4,8", Polar="0,15", Azimuthal="0,0", DetectorIDs="1,2")
        alg_test = run_algorithm(
            "ComputeCalibrationCoefVan",
            VanadiumWorkspace=self._input_ws,
            EPPTable=self._table,
            OutputWorkspace=outputWorkspaceName,
            EnableDWF=False,
        )
        self.assertTrue(alg_test.isExecuted())
        wsoutput = AnalysisDataService.retrieve(outputWorkspaceName)
        for i in range(wsoutput.getNumberHistograms()):
            self.assertEqual(100.0, wsoutput.readY(i)[0])
            self.assertEqual(10.0, wsoutput.readE(i)[0])

        DeleteWorkspace(wsoutput)

    def tearDown(self):
        if AnalysisDataService.doesExist(self._input_ws.name()):
            DeleteWorkspace(self._input_ws)

        if AnalysisDataService.doesExist(self._table.name()):
            DeleteWorkspace(self._table)

    def _checkDWF(self, wsoutput, temperature):
        self.assertEqual(100.0, wsoutput.readY(0)[0])
        self.assertEqual(10.0, wsoutput.readE(0)[0])
        if temperature == 0.0:
            integral = 0.5
        elif temperature == 293.0:
            integral = 4.736767162094296 / 3.0
        else:
            raise RuntimeError("Unsupported temperature supplied to " + "_checkDWF(). Use 0K or 293K only.")
        mvan = 0.001 * 50.942 / N_A
        Bcoef = 3.0 * integral * 1e20 * hbar * hbar / (2.0 * mvan * k * 389.0)
        dwf = np.exp(-1.0 * Bcoef * (4.0 * np.pi * np.sin(0.5 * np.radians(15.0)) / 4.0) ** 2)
        self.assertAlmostEqual(100.0 / dwf, wsoutput.readY(1)[0], places=12)
        self.assertAlmostEqual(10.0 / dwf, wsoutput.readE(1)[0], places=12)


if __name__ == "__main__":
    unittest.main()

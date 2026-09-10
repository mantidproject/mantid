# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch

import numpy as np

from mantid import config, FileFinder
from mantid.api import AnalysisDataService, IEventWorkspace, MatrixWorkspace, WorkspaceGroup
from mantid.kernel import Logger
from mantid.simpleapi import AddSampleLog, AddTimeSeriesLog, ConvertUnits, CreateSampleWorkspace
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISPreprocess import ReflectometryISISPreprocess
from testhelpers import create_algorithm
from testhelpers.tempfile_wrapper import TemporaryFileHelper


class ReflectometryISISPreprocessTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data_INTER45455.dat")

    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config["default.instrument"]
        config["default.facility"] = "ISIS"
        config["default.instrument"] = "INTER"
        self._temp_calibration_file = None

    def tearDown(self):
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument
        if self._temp_calibration_file:
            del self._temp_calibration_file

    def test_input_run_is_loaded_histo_mode_by_default(self):
        args = {"InputRunList": "INTER13460", "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_histo_mode(self):
        args = {"InputRunList": "INTER13460", "EventMode": False, "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_event_mode(self):
        args = {"InputRunList": "INTER13460", "EventMode": True, "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, IEventWorkspace)

    def test_validation_of_event_workspaces_without_proton_charge_throws(self):
        ws = CreateSampleWorkspace()

        with self.assertRaisesRegex(RuntimeError, "proton_charge"):
            ReflectometryISISPreprocess._validate_event_ws(ws)

    def test_validation_of_event_workspace_group_throws(self):
        ws = WorkspaceGroup()

        with self.assertRaisesRegex(RuntimeError, "Workspace Groups"):
            ReflectometryISISPreprocess._validate_event_ws(ws)

    def test_monitors_are_not_loaded_in_histo_mode(self):
        args = {"InputRunList": "INTER13460", "EventMode": False, "OutputWorkspace": "ws"}
        output_ws, monitor_ws = self._run_test_with_monitors(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertIsNone(monitor_ws)

    def test_monitors_are_loaded_in_event_mode(self):
        args = {"InputRunList": "INTER13460", "EventMode": True, "OutputWorkspace": "ws"}
        output_ws, monitor_ws = self._run_test_with_monitors(args)
        self.assertIsInstance(output_ws, IEventWorkspace)
        self.assertIsInstance(monitor_ws, MatrixWorkspace)

    def test_workspace_group(self):
        args = {"InputRunList": "POLREF14966", "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, WorkspaceGroup)
        self.assertEqual(output_ws.getNumberOfEntries(), 2)

    @patch.object(Logger, "warning")
    def test_already_calibrated_workspace_is_ignored_by_default(self, mock_warning):
        ws = self._workspace_with_calibration_log()
        alg = self._initialized_algorithm()

        alg.handle_if_already_calibrated(ws)

        mock_warning.assert_not_called()

    @patch.object(Logger, "warning")
    def test_already_calibrated_workspace_logs_warning_for_warn(self, mock_warning):
        ws = self._workspace_with_calibration_log()
        alg = self._initialized_algorithm(IfAlreadyCalibrated="WARN")

        alg.handle_if_already_calibrated(ws)

        mock_warning.assert_called_once_with(
            f"Workspace with run no. {ws.getRunNumber()} already has a calibration file log. "
            "The calibration algorithm will be rerun, which may produce erroneous results."
        )

    def test_already_calibrated_workspace_raises_for_throw(self):
        ws = self._workspace_with_calibration_log()
        alg = self._initialized_algorithm(IfAlreadyCalibrated="THROW")

        with self.assertRaisesRegex(RuntimeError, "already has a calibration file log"):
            alg.handle_if_already_calibrated(ws)

    def test_uncalibrated_workspace_does_not_raise_for_throw(self):
        ws = CreateSampleWorkspace()
        alg = self._initialized_algorithm(IfAlreadyCalibrated="THROW")

        alg.handle_if_already_calibrated(ws)

    def test_already_calibrated_workspace_group_members_are_checked(self):
        uncalibrated_ws = CreateSampleWorkspace()
        calibrated_ws = self._workspace_with_calibration_log()
        group = WorkspaceGroup()
        group.addWorkspace(uncalibrated_ws)
        group.addWorkspace(calibrated_ws)
        alg = self._initialized_algorithm(IfAlreadyCalibrated="THROW")

        with self.assertRaisesRegex(RuntimeError, "already has a calibration file log"):
            alg.handle_if_already_calibrated(group)

    def test_polref_workspace_group_uses_consistent_wavelength_bins_after_calibration(self):
        calibration_lines = ["spectrumnumber angle\n"]
        calibration_lines.extend(f"{spectrum_no} {3.5 - spectrum_no * 0.005}\n" for spectrum_no in range(5, 645))
        self._temp_calibration_file = TemporaryFileHelper(fileContent="".join(calibration_lines), extension=".dat")
        args = {
            "InputRunList": "POLREF14966",
            "CalibrationFile": self._temp_calibration_file.getName(),
            "ThetaIn": 0.5,
            "OutputWorkspace": "ws",
        }

        output_ws = self._run_test(args)
        AnalysisDataService.addOrReplace("calibrated", output_ws)
        wavelength_ws = ConvertUnits(InputWorkspace="calibrated", Target="Wavelength", OutputWorkspace="wavelength")

        self.assertIsInstance(output_ws, WorkspaceGroup)
        for workspace_index in range(wavelength_ws[0].getNumberHistograms()):
            np.testing.assert_array_equal(wavelength_ws[0].x(workspace_index), wavelength_ws[1].x(workspace_index))

    def test_experiment_angle_uses_theta_in_when_provided(self):
        ws = CreateSampleWorkspace()
        AddSampleLog(Workspace=ws, LogName="theta", LogText="0.5", LogType="Number", NumberType="Double")
        alg = self._initialized_algorithm(ThetaIn=1.2, ThetaLogName="theta")

        self.assertEqual(1.2, alg._experiment_angle(ws))

    def test_experiment_angle_uses_scalar_log_when_theta_in_is_not_provided(self):
        ws = CreateSampleWorkspace()
        AddSampleLog(Workspace=ws, LogName="theta", LogText="0.7", LogType="Number", NumberType="Double")
        alg = self._initialized_algorithm(ThetaLogName="theta")

        self.assertEqual(0.7, alg._experiment_angle(ws))

    def test_experiment_angle_uses_last_time_series_log_value_when_theta_in_is_not_provided(self):
        ws = CreateSampleWorkspace()
        AddTimeSeriesLog(Workspace=ws, Name="theta", Time="2010-01-01T00:00:00", Value=0.5)
        AddTimeSeriesLog(Workspace=ws, Name="theta", Time="2010-01-01T00:10:00", Value=0.7)
        alg = self._initialized_algorithm(ThetaLogName="theta")

        self.assertEqual(0.7, alg._experiment_angle(ws))

    def test_experiment_angle_throws_when_theta_in_and_theta_log_name_are_not_provided(self):
        ws = CreateSampleWorkspace()
        alg = self._initialized_algorithm()

        with self.assertRaisesRegex(RuntimeError, "ThetaIn or ThetaLogName"):
            alg._experiment_angle(ws)

    def test_fractional_workspace_index_is_converted_to_fractional_spectrum_number(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2)
        ws.getSpectrum(0).setSpectrumNo(100)
        ws.getSpectrum(1).setSpectrumNo(104)

        spectrum_number = ReflectometryISISPreprocess._spectrum_number_for_workspace_index(ws, 0.25)

        self.assertEqual(101.0, spectrum_number)

    @staticmethod
    def _initialized_algorithm(**kwargs):
        alg = ReflectometryISISPreprocess()
        alg.initialize()
        for property_name, value in kwargs.items():
            alg.setProperty(property_name, value)
        return alg

    @staticmethod
    def _workspace_with_calibration_log():
        ws = CreateSampleWorkspace()
        AddSampleLog(Workspace=ws, LogName="reflectometry_calibration_file", LogText="calibration.dat")
        return ws

    def _setup_algorithm(self, args):
        alg = create_algorithm("ReflectometryISISPreprocess", **args)
        alg.setChild(True)
        alg.setRethrows(True)
        return alg

    def _run_test_with_monitors(self, args):
        alg = self._setup_algorithm(args)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value
        monitor_ws = alg.getProperty("MonitorWorkspace").value
        return output_ws, monitor_ws

    def _run_test(self, args):
        output_ws, _ = self._run_test_with_monitors(args)
        return output_ws

    def _assert_run_algorithm_raises_exception(self, args, error_msg_regex):
        """Run the algorithm with the given args and check it raises the expected exception"""
        alg = self._setup_algorithm(args)
        self.assertRaisesRegex(RuntimeError, error_msg_regex, alg.execute)


if __name__ == "__main__":
    unittest.main()

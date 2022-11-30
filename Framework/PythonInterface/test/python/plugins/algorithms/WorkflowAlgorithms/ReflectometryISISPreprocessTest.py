# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid import config, FileFinder
from mantid.api import AnalysisDataService, IEventWorkspace, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISPreprocess import ReflectometryISISPreprocess
from testhelpers import create_algorithm


class ReflectometryISISPreprocessTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data_INTER45455.dat")

    def setUp(self):
        self._oldFacility = config['default.facility']
        if self._oldFacility.strip() == '':
            self._oldFacility = 'TEST_LIVE'
        self._oldInstrument = config['default.instrument']
        config['default.facility'] = 'ISIS'
        config['default.instrument'] = 'INTER'

    def tearDown(self):
        AnalysisDataService.clear()
        config['default.facility'] = self._oldFacility
        config['default.instrument'] = self._oldInstrument

    def test_input_run_is_loaded_histo_mode_by_default(self):
        args = {'InputRunList': 'INTER13460',
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_histo_mode(self):
        args = {'InputRunList': 'INTER13460',
                "EventMode": False,
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_event_mode(self):
        args = {'InputRunList': 'INTER13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}
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
        args = {'InputRunList': 'INTER13460',
                "EventMode": False,
                "OutputWorkspace": "ws"}
        output_ws, monitor_ws = self._run_test_with_monitors(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertIsNone(monitor_ws)

    def test_monitors_are_loaded_in_event_mode(self):
        args = {'InputRunList': 'INTER13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}
        output_ws, monitor_ws = self._run_test_with_monitors(args)
        self.assertIsInstance(output_ws, IEventWorkspace)
        self.assertIsInstance(monitor_ws, MatrixWorkspace)

    def test_workspace_group(self):
        args = {'InputRunList': 'POLREF14966',
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, WorkspaceGroup)
        self.assertEqual(output_ws.getNumberOfEntries(), 2)

    def test_calibration_file_is_applied_when_provided(self):
        args = {'InputRunList': 'INTER45455',
                'CalibrationFile': self._CALIBRATION_TEST_DATA,
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self._check_calibration(output_ws, is_calibrated=True)

    def test_calibration_is_skipped_if_file_not_provided(self):
        args = {'InputRunList': 'INTER45455',
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self._check_calibration(output_ws, is_calibrated=False)

    def _run_test_with_monitors(self, args):
        alg = create_algorithm('ReflectometryISISPreprocess', **args)
        alg.setChild(True)
        alg.setRethrows(True)
        alg.execute()
        output_ws = alg.getProperty('OutputWorkspace').value
        monitor_ws = alg.getProperty('MonitorWorkspace').value
        return output_ws, monitor_ws

    def _run_test(self, args):
        output_ws, _ = self._run_test_with_monitors(args)
        return output_ws

    def _check_calibration(self, ws, is_calibrated):
        self.assertEqual(is_calibrated, AnalysisDataService.doesExist(f"Calib_Table_{str(ws.getRunNumber())}"))


if __name__ == '__main__':
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import IEventWorkspace, MatrixWorkspace
from mantid.simpleapi import ReflectometryISISPreprocess


class ReflectometryISISPreprocessTest(unittest.TestCase):
    def test_input_run_is_loaded_histo_mode_by_default(self):
        args = {'InputRunList': '13460',
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_histo_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": False,
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_event_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, IEventWorkspace)

    def test_validation_of_event_workspaces(self):
        pass


if __name__ == '__main__':
    unittest.main()

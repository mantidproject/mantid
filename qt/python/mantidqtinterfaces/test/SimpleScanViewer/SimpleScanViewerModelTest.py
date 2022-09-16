# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

import numpy as np
from qtpy.QtWidgets import QApplication
from matplotlib.widgets import Rectangle

from mantid.simpleapi import config, CreateWorkspace
from mantid.api import mtd

from mantidqtinterfaces.simplescanviewer.model import SimpleScanViewerModel

app = QApplication(sys.argv)


class SimpleScanViewerModelTest(unittest.TestCase):
    def setUp(self) -> None:
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        config['default.facility'] = "ILL"
        config['default.instrument'] = "D16"

        patch = mock.patch(
                'mantidqtinterfaces.simplescanviewer.presenter.SimpleScanViewerPresenter')
        self.mocked_presenter = patch.start()
        self.addCleanup(patch.stop)

        self.model = SimpleScanViewerModel(self.mocked_presenter)

    def tearDown(self) -> None:
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        mtd.clear()

    @staticmethod
    def create_scan_ws(name, number_two_theta_points=50, number_omega_points=10):
        number_of_points = number_two_theta_points * number_omega_points

        y_values = np.linspace(start=0,
                               stop=number_of_points - 1,
                               num=number_of_points)

        x_values = np.array([i for _ in range(number_omega_points) for i in range(number_two_theta_points)])
        spectrum_axis = np.linspace(start=0, stop=number_omega_points - 1, num=number_omega_points)

        CreateWorkspace(OutputWorkspace=name,
                        DataY=y_values,
                        DataX=x_values,
                        NSpec=number_omega_points,
                        UnitX='Degrees',
                        VerticalAxisUnit='Degrees',
                        VerticalAxisValues=spectrum_axis
                        )

    def test_process_file(self):
        self.model._process_scan = mock.Mock()

        self.model._process_scan.return_value = False
        self.model.process_file('')
        self.mocked_presenter.create_slice_viewer.assert_not_called()

        self.model._process_scan.return_value = True
        CreateWorkspace(OutputWorkspace="_scan", DataY=[1], DataX=[1])
        self.model.process_file('')
        self.mocked_presenter.create_slice_viewer.assert_called_once()
        mtd.clear()

    def test_process_background(self):
        self.model._process_scan = mock.Mock()

        self.model._process_scan.return_value = False
        self.model.process_background('')
        self.mocked_presenter.set_bg_ws.assert_not_called()

        self.model._process_scan.return_value = True
        self.model.process_background('')
        self.mocked_presenter.set_bg_ws.assert_called_once()

    def test_roi_integration(self):
        number_omega_points = 10
        number_two_theta_points = 50

        self.create_scan_ws(name="roi_ws",
                            number_two_theta_points=number_two_theta_points,
                            number_omega_points=number_omega_points)
        ws = mtd["roi_ws"]

        rois = [Rectangle((0, 0), 50, 10),        # the entire workspace
                Rectangle((5, 5), 1, 1),          # a single point
                Rectangle((10, 5), 2, 3),         # a regular roi
                Rectangle((100, 100), 100, 100),  # a roi out of the workspace bounds
                Rectangle((0, 0), 1, 20)]         # partially out of bound roi

        expected_results = [500 * 499 / 2,
                            5 * 50 + 5,
                            sum([50 * i + j for i in range(5, 5+3) for j in range(10, 10+2)]),
                            0,
                            50 * 9 * 10 / 2]

        # first check without a background provided
        integrated_values, corrected_values = self.model.roi_integration(ws, rois)

        for result, corrected, expected in zip(integrated_values, corrected_values, expected_results):
            self.assertEqual(result, expected)
            self.assertEqual(corrected, expected)

        # same check, but now with a background
        self.create_scan_ws(name="bg_ws",
                            number_two_theta_points=number_two_theta_points,
                            number_omega_points=1)
        bg_ws = mtd["bg_ws"]

        integrated_values, corrected_values = self.model.roi_integration(ws, rois, bg_ws)

        # the background is one scan point from 0 to 50 (ie equal to the data first line)
        expected_corrected = [500 * 499 / 2 - 10 * 50 * 49 / 2,
                              5 * 50 + 5 - 5,
                              sum([50 * i + j - j for i in range(5, 5+3) for j in range(10, 10+2)]),
                              0,
                              50 * 10 * 9 / 2]  # first background column is zero so no correction

        for result, expected in zip(integrated_values, expected_results):
            self.assertEqual(result, expected)

        for corrected, expected in zip(corrected_values, expected_corrected):
            self.assertEqual(corrected, expected)


if __name__ == "__main__":
    unittest.main()

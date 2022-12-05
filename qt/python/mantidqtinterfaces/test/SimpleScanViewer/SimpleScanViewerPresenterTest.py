# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys
from os import path, sep, getcwd

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import config, CreateSampleWorkspace, CreateWorkspace
from mantid.api import mtd, AlgorithmManager

from mantidqtinterfaces.simplescanviewer.presenter import SimpleScanViewerPresenter

app = QApplication(sys.argv)


class SimpleScanViewerPresenterTest(unittest.TestCase):
    def setUp(self) -> None:
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        config['default.facility'] = "ILL"
        config['default.instrument'] = "D16"

        patch = mock.patch(
                'mantidqtinterfaces.simplescanviewer.model.SimpleScanViewerModel')
        self.mocked_model = patch.start()
        self.addCleanup(patch.stop)

        self.presenter = SimpleScanViewerPresenter()

    def tearDown(self) -> None:
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        mtd.clear()

    def test_additional_peak_info(self):
        CreateWorkspace(OutputWorkspace="test_ws", DataX=[0, 1, 2, 3], DataY=[1, 2, 3])
        self.presenter._ws = mtd["test_ws"]

        integrated_values = [1, 2, 3]
        corrected_values = [1, 1, 1]

        self.presenter.model.roi_integration = lambda x, y, z: (integrated_values, corrected_values)

        peak_dict = self.presenter.additional_peaks_info([])
        self.assertEqual(peak_dict, {"I": integrated_values})

        CreateWorkspace(OutputWorkspace="test_bg_ws", DataX=[0, 1, 2, 3], DataY=[0, 1, 2])
        self.presenter._bg_ws = mtd["test_bg_ws"]

        peak_dict = self.presenter.additional_peaks_info([])
        self.assertEqual(peak_dict, {"I": integrated_values, "I (bg corrected)": corrected_values})

    def test_on_file_selected(self):
        self.presenter.on_line_edited = mock.MagicMock()
        self.presenter.on_file_selected(__file__)

        self.presenter.on_line_edited.assert_called_once()
        self.assertEqual(self.presenter.view.file_line_edit.text(), __file__)

    def test_on_line_edited(self):
        self.presenter.model.process_file = mock.MagicMock()
        self.presenter.view.file_line_edit.setText(__file__)
        self.presenter.on_line_edited()

        self.presenter.model.process_file.assert_called_once()
        self.presenter.model.process_file.assert_has_calls([mock.call(__file__)])

    def test_on_algorithm_finished(self):
        self.presenter.create_slice_viewer = mock.MagicMock()

        CreateSampleWorkspace(OutputWorkspace="test")
        self.presenter.future_workspace = "test"

        self.presenter.on_algorithm_finished(False, "")

        self.assertEqual(self.presenter.ws.name(), "test")
        self.assertEqual(self.presenter.future_workspace, "")
        self.presenter.create_slice_viewer.assert_called_once()

    def test_set_alg_result_name(self):
        alg = AlgorithmManager.create("CreateSampleWorkspace")
        alg.setProperty("OutputWorkspace", "test_ws")

        self.presenter.set_algorithm_result_name(alg)

        self.assertEqual(self.presenter.future_workspace, "test_ws")

    def test_get_base_directory(self):
        # save data search directories, just in case
        search_dirs = config.getDataSearchDirs()

        config.setDataSearchDirs([])
        self.assertEqual(self.presenter.get_base_directory(), path.abspath(sep))

        config.appendDataSearchDir(getcwd())
        self.assertEqual(path.abspath(self.presenter.get_base_directory()), path.abspath(getcwd()))

        # reset data search directories
        config.setDataSearchDirs(search_dirs)

    def test_set_bg_ws(self):
        CreateSampleWorkspace(OutputWorkspace="test")
        self.presenter.set_bg_ws("test")
        self.assertEqual(self.presenter._bg_ws.name(), "test")
        self.assertEqual(self.presenter.view.background_button.text(), "Replace background")


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import mtd, CreateWorkspace
from mantid.api import PreviewType

from mantidqt.widgets.rawdataexplorer.model import PreviewModel, RawDataExplorerModel
from mantidqt.widgets.rawdataexplorer.PreviewFinder import AcquisitionType

app = QApplication(sys.argv)


class PreviewModelTest(unittest.TestCase):
    def setUp(self) -> None:
        self.ws_name = "ws"
        self.preview_model = PreviewModel(mock.MagicMock(), self.ws_name)

    def test_set_workspace_name(self):
        new_name = "tmp_ws"
        trigger_check = mock.MagicMock()
        self.preview_model.sig_workspace_changed.connect(trigger_check)

        self.preview_model.set_workspace_name(new_name)

        self.assertEqual(self.preview_model._workspace_name, new_name)
        trigger_check.assert_called_once()  # checking the signal has been emitted


class RawDataExplorerModelTest(unittest.TestCase):

    def setUp(self):
        self.mocked_presenter = mock.MagicMock()
        self.mocked_memory_manager = mock.MagicMock()
        self.model = RawDataExplorerModel(self.mocked_presenter, self.mocked_memory_manager)

    def tearDown(self):
        mtd.clear()

    def test_modify_preview(self):
        new_file = "data"
        self.model.load_file = mock.MagicMock()
        self.model.choose_preview = mock.MagicMock()
        self.model.memory_manager = mock.MagicMock()
        sig_new_preview = mock.MagicMock()
        self.model.sig_new_preview.connect(sig_new_preview)
        self.assertEqual(len(self.model._previews), 0)  # checking if there were no previous previews
        self.model.modify_preview(new_file)
        self.model.load_file.assert_called_once_with(new_file, new_file)
        self.model.memory_manager.workspace_interacted_with.assert_called_once_with(new_file)
        self.model.choose_preview.assert_called_once_with(new_file)
        self.assertEqual(len(self.model._previews), 1)  # checking if a new preview has been added
        sig_new_preview.assert_called_once()  # checking the signal has been emitted

    # not testing load_file: it just runs load, it would be slow for nothing

    def test_del_preview_present(self):
        preview = PreviewModel(PreviewType.PLOT1D, "ws")
        self.model._previews.append(preview)
        self.model.del_preview(preview)

        self.assertEqual(len(self.model._previews), 0)

    def test_del_preview_absent(self):
        preview = PreviewModel(PreviewType.PLOT1D, "ws")
        self.model._previews.append(preview)

        self.model.del_preview(PreviewModel(PreviewType.PLOT1D, "not_the_same_ws"))
        self.model.del_preview(PreviewModel(PreviewType.PLOT2D, "ws"))

        self.assertEqual(len(self.model._previews), 1)

    def test_can_delete_workspace(self):
        self.model._previews.append(PreviewModel(PreviewType.PLOT1D, "ws_0"))
        self.model._previews.append(PreviewModel(PreviewType.PLOT1D, "ws_1"))

        self.assertTrue(self.model.can_delete_workspace("ws"))
        self.assertFalse(self.model.can_delete_workspace("ws_1"))

    def test_choose_preview(self):
        # TODO a lot of different cases to cover
        pass

    def test_determine_acquisition_mode(self):
        ws = CreateWorkspace(DataX='1,1,1,1', DataY='1,4,2,7', NSpec=4)

        acq = self.model.determine_acquisition_mode(ws)
        self.assertEqual(acq, AcquisitionType.MONO)

        ws = CreateWorkspace(DataX='1,1,1,1', DataY='1,4,2,7', NSpec=2, UnitX="Empty")

        acq = self.model.determine_acquisition_mode(ws)
        self.assertEqual(acq, AcquisitionType.SCAN)

        ws = CreateWorkspace(DataX='1,1,1,1', DataY='1,4,2,7', NSpec=2, UnitX="TOF")

        acq = self.model.determine_acquisition_mode(ws)
        self.assertEqual(acq, AcquisitionType.TOF)

    @mock.patch("mantidqt.widgets.rawdataexplorer.model.error_reporting")
    def test_determine_acquisition_mode_failure_to_find(self, error_reporting):
        ws = CreateWorkspace(DataX='1,1,1,1', DataY='1,4,2,7', NSpec=2, UnitX="AtomicDistance")

        acq = self.model.determine_acquisition_mode(ws)
        self.assertEqual(acq, AcquisitionType.TOF)
        error_reporting.assert_called_once()

    def test_accumulate(self):
        ws_1 = 'ws1'
        ws_2 = 'ws2'

        CreateWorkspace(OutputWorkspace=ws_1, DataX='1,1,1', DataY='1,4,2', NSpec=3)
        CreateWorkspace(OutputWorkspace=ws_2, DataX='1,1,1', DataY='3,7,11', NSpec=3)

        acc = self.model.accumulate(ws_1, ws_2)
        self.assertEqual(acc, ws_1 + '_' + ws_2)
        self.assertTrue(mtd.doesExist(acc))
        self.assertEqual(mtd[acc].dataY(0), 4)
        self.assertEqual(mtd[acc].dataY(1), 11)

    @mock.patch("mantidqt.widgets.rawdataexplorer.model.error_reporting")
    def test_accumulate_failure(self, error_reporting):
        ws_1 = 'ws1'
        CreateWorkspace(OutputWorkspace=ws_1, DataX='1,1,1', DataY='1,4,2', NSpec=3)

        # cannot check the error case is handled well: the popup blocks the test
        acc = self.model.accumulate(ws_1, "ws_that_does_not_exist")
        self.assertIsNone(acc)
        error_reporting.assert_called_once()

    def test_accumulate_name(self):
        new = "new"

        single_file_acc = "000001"
        acc_name = self.model.accumulate_name(single_file_acc, new)
        self.assertEqual(acc_name, single_file_acc + "_" + new)

        two_files_acc = "000001_000002"
        acc_name = self.model.accumulate_name(two_files_acc, new)
        self.assertEqual(acc_name, "000001_..._new")

        multi_files_acc = "000001_..._000009"
        acc_name = self.model.accumulate_name(multi_files_acc, new)
        self.assertEqual(acc_name, "000001_..._new")


if __name__ == "__main__":
    unittest.main()

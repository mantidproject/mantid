# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys
import os

from qtpy.QtTest import QTest
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication, QAbstractItemView

from mantid.simpleapi import config

from mantidqt.widgets.rawdataexplorer.presenter import PreviewPresenter, RawDataExplorerPresenter
from mantidqt.widgets.rawdataexplorer.view import RawDataExplorerView

app = QApplication(sys.argv)


class PreviewPresenterTest(unittest.TestCase):
    def setUp(self) -> None:
        self.mocked_presenter = mock.MagicMock()
        self.mocked_main_model = mock.MagicMock()
        self.mocked_model = mock.MagicMock()
        self.mocked_main_view = mock.MagicMock()
        self.mocked_view = mock.MagicMock()
        self.preview_presenter = PreviewPresenter(self.mocked_main_view, self.mocked_view, self.mocked_main_model, self.mocked_model)

    def test_close_preview_not_requested(self):
        self.preview_presenter.close_preview(requested_by_user=False)

        self.preview_presenter._main_view.del_preview.assert_called_once()
        self.preview_presenter._main_view.del_preview.has_calls(mock.call([self.mocked_view]))

        self.preview_presenter._main_model.del_preview.assert_called_once()
        self.preview_presenter._main_model.del_preview.has_calls(mock.call([self.mocked_model]))

        self.preview_presenter._main_view.select_last_clicked.assert_called_once()
        self.preview_presenter._main_view.clear_selection.assert_not_called()

    def test_close_preview_user_requested(self):
        self.preview_presenter.close_preview(requested_by_user=True)

        self.preview_presenter._main_view.del_preview.assert_called_once()
        self.preview_presenter._main_view.del_preview.has_calls(mock.call([self.mocked_view]))

        self.preview_presenter._main_model.del_preview.assert_called_once()
        self.preview_presenter._main_model.del_preview.has_calls(mock.call([self.mocked_model]))

        self.preview_presenter._main_view.select_last_clicked.assert_not_called()
        self.preview_presenter._main_view.clear_selection.assert_called_once()

    def test_on_workspace_changed(self):
        tmp_ws_name = "ws"
        self.mocked_model.get_workspace_name = lambda: tmp_ws_name
        self.preview_presenter.on_workspace_changed()

        self.preview_presenter._view.change_workspace.has_calls(mock.call([tmp_ws_name]))


class RawDataExplorerPresenterTest(unittest.TestCase):
    def setUp(self) -> None:
        # we need to mock the model because it holds the memory manager
        self.presenter = RawDataExplorerPresenter(model=mock.MagicMock())

    def test_constructor(self):
        self.assertFalse(self.presenter._is_accumulating)
        self.assertIsInstance(self.presenter.view, RawDataExplorerView)

    def test_setup_connections(self):
        # removing already set connections so the real functions are not called by emit
        self.presenter.view.file_tree_path_changed.disconnect()
        self.presenter.view.repositoryPath.editingFinished.disconnect()
        self.presenter.view.fileTree.sig_accumulate_changed.disconnect()
        self.presenter.model.sig_new_preview.disconnect()

        self.presenter.on_file_dialog_choice = mock.Mock()
        self.presenter.on_qlineedit = mock.Mock()
        self.presenter.on_accumulate_changed = mock.Mock()
        self.presenter.on_new_preview = mock.Mock()

        self.presenter.setup_connections()

        path = os.path.abspath("/new/path/")
        self.presenter.view.file_tree_path_changed.emit(path)
        self.presenter.on_file_dialog_choice.assert_called_with(path)

        QTest.keyPress(self.presenter.view.repositoryPath, Qt.Key_Enter)
        QTest.keyRelease(self.presenter.view.repositoryPath, Qt.Key_Enter)
        self.presenter.on_qlineedit.assert_called_once()

        self.presenter.view.fileTree.sig_accumulate_changed.emit(True)
        self.presenter.on_accumulate_changed.assert_called_with(True)

        self.presenter.model.sig_new_preview.emit()
        self.presenter.on_new_preview.asset_called_once()

    def test_set_initial_directory(self):
        data_search_dirs = config.getDataSearchDirs()

        config.setDataSearchDirs([])

        self.presenter.set_working_directory = mock.Mock()
        self.presenter._set_initial_directory()
        self.presenter.set_working_directory.assert_called_with(os.path.abspath(os.sep))

        config.setDataSearchDirs(["invalid_path"])
        self.presenter._set_initial_directory()
        self.presenter.set_working_directory.assert_called_with(os.path.abspath(os.sep))

        config.setDataSearchDirs(data_search_dirs)

    def test_set_working_directory(self):
        self.presenter.view.repositoryPath = mock.Mock()
        self.presenter.view.fileTree = mock.Mock()

        path = os.path.abspath("/")
        self.presenter.set_working_directory(path)
        self.presenter.view.repositoryPath.setText.assert_called_once_with(path)
        self.presenter.view.fileTree.model().setRootPath.assert_called_with(path)

    def test_on_qlineedit(self):
        self.presenter.view.repositoryPath.setText(os.path.abspath(os.sep))
        self.presenter.set_working_directory = mock.Mock()

        self.presenter.on_qlineedit()

        self.presenter.set_working_directory.assert_called_with(os.path.abspath(os.sep))

    @mock.patch("mantidqt.widgets.rawdataexplorer.presenter.logger")
    def test_on_qlineedit_invalid(self, logger):
        path = "/invalid/path"
        self.presenter.view.repositoryPath.setText(path)
        self.presenter.set_working_directory = mock.Mock()

        self.presenter.on_qlineedit()

        self.presenter.set_working_directory.assert_not_called()
        logger.warning.assert_called_once()

    def test_on_accumulate_changed_to_true(self):
        self.presenter.view.fileTree.selectionModel = mock.Mock()
        self.presenter.set_selection_mode = mock.Mock()

        self.presenter.on_accumulate_changed(True)
        self.presenter.set_selection_mode.assert_called_once()
        self.presenter.view.fileTree.selectionModel.assert_not_called()

    def test_on_accumulate_changed_to_false(self):
        self.presenter.view.fileTree.selectionModel = mock.Mock()
        self.presenter.set_selection_mode = mock.Mock()

        self.presenter.on_accumulate_changed(False)

        self.presenter.view.fileTree.selectionModel.assert_has_calls([mock.call().clearSelection()])
        self.presenter.set_selection_mode.assert_called_once()

    @mock.patch("mantidqt.widgets.rawdataexplorer.presenter.os")
    def test_on_selection_changed(self, os_mod):
        last_ind = mock.Mock()
        self.presenter.view.get_last_clicked = mock.Mock(return_value=last_ind)
        os_mod.path.isfile.return_value = True
        self.presenter.set_selection_mode = mock.Mock()

        self.presenter.on_selection_changed()

        self.presenter.model.modify_preview.assert_called_with(last_ind)
        self.presenter.set_selection_mode.assert_has_calls([mock.call(False), mock.call(True)])

    @mock.patch("mantidqt.widgets.rawdataexplorer.presenter.PreviewPresenter")
    def test_on_new_preview(self, prev):
        mock_view = mock.Mock()
        mock_model = mock.Mock()
        self.presenter.view.add_preview = mock.Mock()
        self.presenter.view.add_preview.return_value = mock_view

        self.presenter.on_new_preview(mock_model)

        self.presenter.view.add_preview.assert_called_once()
        prev.assert_called_once_with(self.presenter.view, mock_view, self.presenter.model, mock_model)

    def test_set_selection_mode_cannot_select(self):
        self.presenter.set_selection_mode(False)
        self.assertEqual(self.presenter.view.fileTree.selectionMode(), QAbstractItemView.NoSelection)

    def test_set_selection_mode_no_acc(self):
        self.presenter._is_accumulating = False
        self.presenter.set_selection_mode(True)
        self.assertEqual(self.presenter.view.fileTree.selectionMode(), QAbstractItemView.SingleSelection)

    def test_set_selection_mode_acc(self):
        self.presenter._is_accumulating = True
        self.presenter.set_selection_mode(True)
        self.assertEqual(self.presenter.view.fileTree.selectionMode(), QAbstractItemView.MultiSelection)


if __name__ == "__main__":
    unittest.main()

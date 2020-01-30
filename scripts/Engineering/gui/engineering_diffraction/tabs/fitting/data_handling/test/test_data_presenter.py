# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.py3compat import mock
from mantid.py3compat.mock import patch

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling import data_model, data_presenter, data_view

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.data_handling"


class FittingDataPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(data_model.FittingDataModel)
        self.view = mock.create_autospec(data_view.FittingDataView)
        self.presenter = data_presenter.FittingDataPresenter(self.model, self.view)

    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_started_correctly(self, mock_worker):
        self.view.is_searching.return_value = False
        self.view.get_filenames_to_load.return_value = "/a/file/to/load.txt, /another/one.nxs"
        self.model.load_files = "mocked model method"

        self.presenter.on_load_clicked()

        mock_worker.assert_called_with("mocked model method",
                                       "/a/file/to/load.txt, /another/one.nxs",
                                       error_cb=self.presenter._on_worker_error,
                                       finished_cb=self.presenter._emit_enable_button_signal)

    @patch(dir_path + ".data_presenter.create_error_message")
    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_not_started_while_searching(self, mock_worker, mock_error):
        self.view.is_searching.return_value = True
        self.view.get_filenames_valid.return_value = True

        self.presenter.on_load_clicked()

        self.assertEqual(0, mock_worker.call_count)
        self.assertEqual(0, self.view.get_filenames_to_load.call_count)
        mock_error.assert_called_with(self.view, "Mantid is searching for files. Please wait.")

    @patch(dir_path + ".data_presenter.create_error_message")
    @patch(dir_path + ".data_presenter.AsyncTask")
    def test_worker_not_started_while_files_invalid(self, mock_worker, mock_error):
        self.view.is_searching.return_value = False
        self.view.get_filenames_valid.return_value = False

        self.presenter.on_load_clicked()

        self.assertEqual(0, mock_worker.call_count)
        self.assertEqual(0, self.view.get_filenames_to_load.call_count)
        mock_error.assert_called_with(self.view, "Entered files are not valid.")

    @patch(dir_path + ".data_presenter.logger")
    def test_worker_error(self, logger):
        self.view.sig_enable_load_button = mock.MagicMock()

        self.presenter._on_worker_error("info")

        logger.error.assert_called_with("Error occurred when loading files.")
        self.assertEqual(1, self.view.sig_enable_load_button.emit.call_count)
        self.view.sig_enable_load_button.emit.called_with(True)



if __name__ == '__main__':
    unittest.main()

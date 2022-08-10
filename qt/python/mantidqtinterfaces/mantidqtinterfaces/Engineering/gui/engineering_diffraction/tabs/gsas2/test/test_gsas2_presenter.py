# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

# from unittest import mock
# from unittest.mock import patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model import \
    GSAS2Model
# from testhelpers import assertRaisesNothing

data_model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model"
# TODO: Make this about the presenter!!!


class TestGSAS2Model(unittest.TestCase):
    def setUp(self):
        self.model = GSAS2Model()
        # setup a mock workspace
        # self.mock_inst = mock.MagicMock()

    # def test_run_model(self):
    #     success_state = self.model.run_model()
    #     assert(success_state, "run_model failed!")

    # @patch(data_model_path + ".FittingDataModel.update_log_workspace_group")
    # @patch(data_model_path + ".Load")
    # def test_loading_single_file_stores_workspace(self, mock_load, mock_update_logws_group):
    #     mock_load.return_value = self.mock_ws
    #
    #     self.model.load_files("/ar/a_filename.whatever")
    #
    #     self.assertEqual(1, len(self.model._data_workspaces))
    #     self.assertEqual(self.mock_ws, self.model._data_workspaces["a_filename"].loaded_ws)
    #     mock_load.assert_called_with("/ar/a_filename.whatever", OutputWorkspace="a_filename")
    #     mock_update_logws_group.assert_called_once()
    #
    # @patch(data_model_path + ".FittingDataModel.update_log_workspace_group")
    # @patch(data_model_path + '.ADS')
    # @patch(data_model_path + ".Load")
    # def test_loading_single_file_already_loaded_untracked(self, mock_load, mock_ads, mock_update_logws_group):
    #     mock_ads.doesExist.return_value = True
    #     mock_ads.retrieve.return_value = self.mock_ws
    #
    #     self.model.load_files("/ar/a_filename.whatever")
    #
    #     self.assertEqual(1, len(self.model._data_workspaces))
    #     mock_load.assert_not_called()
    #     mock_update_logws_group.assert_called_once()


if __name__ == '__main__':
    unittest.main()

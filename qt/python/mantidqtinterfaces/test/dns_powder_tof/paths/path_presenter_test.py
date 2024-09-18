# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.paths.path_model import DNSPathModel
from mantidqtinterfaces.dns_powder_tof.paths.path_presenter import DNSPathPresenter
from mantidqtinterfaces.dns_powder_tof.paths.path_view import DNSPathView


class DNSPathPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    @classmethod
    def setUpClass(cls):
        # cls.parent = mock.patch(DNSReductionGUI_presenter)
        cls.view = mock.create_autospec(DNSPathView)
        cls.model = mock.create_autospec(DNSPathModel)
        # view signals

        cls.view.sig_data_path_is_set = mock.Mock(return_value="dummypath")
        cls.view.sig_data_dir_editing_finished = mock.Mock()
        cls.view.sig_clear_cache = mock.Mock()
        cls.view.sig_file_dialog_requested = mock.Mock(return_value="data")
        cls.view.sig_data_path_changed = mock.Mock(return_value="C:/data")

        cls.view.within_mantid = False
        # view functions
        cls.view.get_path.return_value = ""
        cls.view.open_file_dialog.return_value = "C:/dummy/test.py"
        # model functions
        cls.model.get_start_path_for_dialog.return_value = "C:/dummy"
        cls.model.get_user_and_proposal_number.return_value = ["Thomas", "p123456", []]
        # create presenter
        cls.presenter = DNSPathPresenter(view=cls.view, model=cls.model)

    def setUp(self):
        self.model.get_user_and_proposal_number.reset_mock()
        self.view.set_data_path.reset_mock()
        self.view.set_prop_number.reset_mock()
        self.view.set_user.reset_mock()
        self.view.get_path.reset_mock()
        self.view.set_path.reset_mock()

    def test__init__(self):
        self.assertIsInstance(self.presenter, DNSObserver)
        self.view.set_data_path.assert_not_called()
        self.model.get_current_directory.assert_called()

    def test_data_path_editing_finished(self):
        self.view.get_state.return_value = {"auto_set_other_dir": True}
        self.presenter._set_data_path(dir_name="C:/test")
        self.assertEqual(self.view.set_path.call_count, 4)

    def test_data_path_set(self):
        self.view.get_state.return_value = {"auto_set_other_dir": True}
        self.presenter._set_data_path(dir_name="C:/test")
        self.assertEqual(self.view.set_path.call_count, 4)
        self.view.set_path.assert_called_with("export_dir", "C:/test/export")
        self.view.set_path.reset_mock()
        self.view.get_state.return_value = {"auto_set_other_dir": False}
        self.presenter._set_data_path(dir_name="C:/test")
        self.view.set_path.assert_not_called()

    def test_set_user_prop_from_datafile(self):
        self.presenter._set_user_prop_from_datafile(dir_name="C:/test")
        self.model.get_user_and_proposal_number.assert_called_once()
        self.view.set_prop_number.assert_called_once()
        self.view.set_user.assert_called_once()
        self.view.show_status_message.assert_not_called()

    def test_clear_cache(self):
        self.presenter._clear_cache()
        self.model.clear_cache.assert_not_called()
        self.presenter.own_dict = {"data_dir": "123"}
        self.presenter._clear_cache()
        self.model.clear_cache.assert_called_once()

    def test_filedialog_requested(self):
        self.presenter._file_dialog_requested(sender="data")
        self.view.get_path.assert_called_once_with("data_dir")
        self.model.get_start_path_for_dialog.assert_called_once()
        self.view.open_file_dialog.assert_called_once()
        self.view.set_data_path.assert_called_once()
        self.view.set_path.assert_not_called()


if __name__ == "__main__":
    unittest.main()

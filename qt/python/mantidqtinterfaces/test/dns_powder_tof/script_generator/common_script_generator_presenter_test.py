# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_model import DNSScriptGeneratorModel
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_presenter import DNSScriptGeneratorPresenter
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_view import DNSScriptGeneratorView
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_fake_empty_param_dict, get_fake_param_dict, get_filepath


class DNSScriptGeneratorPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    model = None
    view = None
    filepath = None
    presenter = None
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.filepath = get_filepath()
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSScriptGeneratorView, instance=True)
        cls.model = mock.create_autospec(DNSScriptGeneratorModel, instance=True)
        cls.model.script_maker.return_value = (["test1", "test2"], "")
        cls.model.run_script.return_value = ""
        cls.view._raise_error = mock.Mock()
        cls.view.pB_copy_to_clipboard = mock.Mock()

        cls.view.sig_progress_canceled.connect = mock.Mock()
        cls.view.sig_generate_script.connect = mock.Mock()
        cls.view.get_state.return_value = {"script_filename": "script.txt", "automatic_filename": False}

        cls.model.save_script.return_value = ["script.txt", cls.filepath]
        cls.model.get_filename.return_value = "script.txt"

        cls.presenter = DNSScriptGeneratorPresenter(view=cls.view, model=cls.model, name="common_script_generator", parent=cls.parent)
        cls.presenter.param_dict = get_fake_param_dict()
        cls.presenter.param_dict["common_options"] = {}
        cls.presenter.param_dict["paths"] = {"script_dir": cls.filepath}
        cls.presenter.request_from_abo = lambda *args: None

    def setUp(self):
        self.model.reset_mock()
        self.view.reset_mock()

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSScriptGeneratorPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertEqual(self.presenter._script_text, "")
        self.assertEqual(self.presenter._script_number, 0)

    def test_generate_script(self):
        self.presenter._script_number = 0
        self.presenter._generate_script()
        self.model.script_maker.assert_called_once()
        self.view.set_script_output.assert_called_once_with("test1\ntest2")
        self.assertEqual(self.view.process_events.call_count, 2)
        self.view.open_progress_dialog.assert_called_once_with(1)
        self.model.run_script.assert_called_once_with(["test1", "test2"])
        self.assertEqual(self.presenter._script_number, 1)
        self.view.show_status_message.assert_called_once()  # from saving
        self.model.run_script.return_value = "Error"
        self.presenter._generate_script()
        self.view.show_status_message.assert_called_with("Error", 30, clear=True)
        self.model.script_maker.return_value = ["test1", "test2"]
        self.presenter._generate_script()
        self.assertEqual(self.presenter._script_number, 1)  # not run

    def test_get_sampledata(self):
        testv = self.presenter._get_sample_data()
        self.assertEqual(testv[0]["file_number"], "787463")
        self.presenter.param_dict = get_fake_empty_param_dict()
        self.assertFalse(self.presenter._get_sample_data())
        self.view.raise_error.assert_called_once()

    def test_get_option_dict(self):
        testv = self.presenter.get_option_dict()
        self.assertEqual(len(testv), 5)

    def test_update_progress(self):
        self.presenter.update_progress(10)
        self.view.set_progress.assert_called_once_with(10)

    def test_progress_canceled(self):
        self.presenter._progress_canceled()
        self.model.cancel_progress.assert_called_once()

    def test_set_script_filename(self):
        self.presenter._set_script_filename()
        self.view.set_filename.assert_not_called()
        self.view.get_state.return_value = {"script_filename": "script.txt", "automatic_filename": True}
        self.presenter._set_script_filename()
        self.view.set_filename.assert_called_once_with("script.py")

    def test_save_script(self):
        self.presenter.param_dict["paths"] = {"script_dir": "123"}
        self.presenter._save_script("")
        self.model.save_script.assert_called_once_with("", "script.txt", "123")
        self.view.show_status_message.assert_called_once()
        self.model.save_script.reset_mock()
        self.presenter.param_dict["paths"] = {"script_dir": ""}
        self.presenter._save_script("")
        self.model.save_script.assert_not_called()
        self.view.raise_error.assert_called_once()


if __name__ == "__main__":
    unittest.main()

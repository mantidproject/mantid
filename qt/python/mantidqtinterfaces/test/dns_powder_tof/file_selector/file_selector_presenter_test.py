# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model import DNSFileSelectorModel
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter import DNSFileSelectorPresenter
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_view import DNSFileSelectorView
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_watcher import DNSFileSelectorWatcher
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_file_selector_param_dict


class DNSFileSelectorPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    view = None
    model = None
    watcher = None
    presenter = None

    @classmethod
    def setUpClass(cls):
        cls.view = mock.create_autospec(DNSFileSelectorView, instance=True)
        cls.model = mock.create_autospec(DNSFileSelectorModel, instance=True)
        cls.watcher = mock.create_autospec(DNSFileSelectorWatcher, instance=True)

        # view signals
        cls.view.sig_expanded = mock.Mock()
        cls.view.sig_filters_clicked = mock.Mock()
        cls.view.sig_standard_filters_clicked = mock.Mock()
        cls.view.sig_check_all = mock.Mock()
        cls.view.sig_uncheck_all = mock.Mock()
        cls.view.sig_check_last = mock.Mock()
        cls.view.sig_check_selected = mock.Mock()
        cls.view.sig_right_click = mock.Mock()
        cls.view.sig_progress_canceled = mock.Mock()
        cls.view.sig_autoload_new_clicked = mock.Mock()
        cls.view.sig_auto_select_standard_clicked = mock.Mock()
        cls.view.sig_standard_data_clicked = mock.Mock()
        cls.view.sig_sample_data_clicked = mock.Mock()

        # watcher signal
        cls.watcher.sig_files_changed = mock.Mock()

        cls.presenter = DNSFileSelectorPresenter(view=cls.view, model=cls.model, name="file_selector", watcher=cls.watcher)

    def setUp(self):
        self.view.reset_mock()
        self.model.reset_mock()

    def test___init__(self):
        self.presenter = DNSFileSelectorPresenter(view=self.view, model=self.model, name="file_selector", watcher=self.watcher)
        self.assertIsInstance(self.presenter, DNSFileSelectorPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)

    def test_read_all(self):
        self.presenter.param_dict = get_file_selector_param_dict()
        self.model.set_datafiles_to_load.return_value = 1, ["b"], ["a"], [3, 4]
        self.model.get_number_of_scans.return_value = 1

        self.presenter._read_all(True, False, 0, 100)
        self.model.set_datafiles_to_load.assert_called_once_with("C:/data", [0, 100], True, False)
        self.view.open_progress_dialog.assert_called_once_with(1)
        self.model.read_all.assert_called_once_with(["a"], "C:/data", ["b"], [], False)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._filter_standard")
    def test_read_standard(self, mock_filter_standard):
        self.model.read_standard.return_value = True
        self.presenter._read_standard()
        self.model.read_standard.assert_called_once()
        mock_filter_standard.assert_called_once()
        self.model.try_unzip.assert_not_called()
        self.model.read_standard.return_value = False
        self.presenter._read_standard(True)
        self.model.try_unzip.assert_not_called()
        self.model.reset_mock()
        self.presenter._read_standard()
        self.model.try_unzip.assert_called_once_with("C:/data", "C:/stand")
        self.assertEqual(self.model.read_standard.call_count, 2)

    def test_cancel_loading(self):
        self.presenter._cancel_loading()
        self.model.set_loading_canceled.assert_called_once_with(True)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._format_view")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._read_all")
    def test_autoload_new(self, mock_read_all, mock_format_view):
        self.presenter.param_dict = get_file_selector_param_dict()
        self.presenter._old_data_set = [1]
        self.presenter._autoload_new(1)
        self.watcher.stop_watcher.assert_called_once()
        self.watcher.start_watcher.assert_not_called()
        mock_read_all.assert_not_called()
        mock_format_view.assert_called_once()
        self.watcher.reset_mock()
        self.presenter._autoload_new(2)
        self.watcher.start_watcher.assert_called_once()
        self.watcher.stop_watcher.assert_not_called()
        mock_read_all.assert_not_called()
        self.presenter._old_data_set = []
        self.presenter._autoload_new(2)
        mock_read_all.assert_called_once()
        self.presenter.param_dict["paths"]["data_dir"] = ""
        self.presenter._autoload_new(2)
        self.watcher.stop_watcher.assert_called_once()
        self.assertEqual(mock_format_view.call_count, 4)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._get_non_hidden_rows")
    def test_check_all_visible_scans(self, mock_hidden_row):
        mock_hidden_row.return_value = [1, 2]
        self.presenter._check_all_visible_scans()
        self.model.check_scans_by_rows.assert_called_once_with([1, 2])

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._get_non_hidden_rows")
    def test_check_last_scans(self, mock_hidden_row):
        mock_hidden_row.return_value = [1, 2]
        self.view.get_number_scans_to_check.return_value = 0
        self.presenter._check_last_scans("test")
        self.view.get_number_scans_to_check.assert_called_once()
        self.model.check_last_scans.assert_called_once_with(0, False, [1, 2])
        mock_hidden_row.assert_called_once()
        self.model.reset_mock()
        self.presenter._check_last_scans("test_complete")
        self.model.check_last_scans.assert_called_once_with(0, True, [1, 2])

    def test_check_selected_scans(self):
        self.view.get_selected_indexes.return_value = 1
        self.presenter._check_selected_scans()
        self.view.get_selected_indexes.assert_called_once()
        self.model.check_scans_by_indexes.assert_called_once_with(1)

    def test_uncheck_all_scans(self):
        self.presenter._uncheck_all_scans()
        self.model.uncheck_all_scans.assert_called_once()

    def test_show_all_scans(self):
        self.model.get_scan_range.return_value = [1, 2]
        self.presenter._show_all_scans()
        self.model.get_scan_range.assert_called_once()
        self.assertEqual(self.view.show_scan.call_count, 2)

    def test_get_non_hidden_rows(self):
        self.model.get_scan_range.return_value = [1, 2]
        self.view.is_scan_hidden.return_value = False
        test_v = self.presenter._get_non_hidden_rows()
        self.model.get_scan_range.assert_called_once()
        self.assertEqual(self.view.is_scan_hidden.call_count, 2)
        self.assertEqual(test_v, [1, 2])

    def test_filter_scans(self):
        self.model.filter_scans_for_boxes.return_value = []
        self.presenter.modus = "elastic"
        self.view.get_filters.return_value = {"a": 1}
        self.presenter._filter_scans()
        self.view.get_filters.assert_called_once()
        self.model.filter_scans_for_boxes.assert_called_once_with(self.view.get_filters.return_value.items(), False)
        self.view.hide_scan.assert_not_called()
        self.model.filter_scans_for_boxes.return_value = [1]
        self.presenter._filter_scans()
        self.view.hide_scan.assert_called_once_with(1)

    def test_filter_standard(self):
        self.presenter.modus = "123"
        self.view.get_standard_filters.return_value = {"vanadium": True}
        self.presenter._filter_standard()
        self.view.get_standard_filters.assert_called_once()
        self.model.filter_standard_types.assert_called_once_with({"vanadium": True}, True, False)
        self.view.hide_scan.assert_not_called()

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._filter_standard")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._read_standard")
    def test_changed_to_standard(self, mock_read, mock_filter):
        self.model.get_number_of_scans.return_value = 1
        self.presenter._changed_to_standard()
        self.model.get_number_of_scans.assert_called_once()
        mock_read.assert_not_called()
        mock_filter.assert_called_once()
        self.model.get_number_of_scans.return_value = 0
        self.presenter._changed_to_standard()
        mock_read.assert_called_once()

    def test_is_modus_tof(self):
        self.presenter.modus = "123"
        self.assertFalse(self.presenter._is_modus_tof())
        self.presenter.modus = "123_tof"
        self.assertTrue(self.presenter._is_modus_tof())

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._filter_scans")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._filter_standard")
    def test_modus_changed(self, mock_filter_standard, mock_filter_scans):
        self.presenter.modus = "123_tof"
        self.presenter._modus_changed()
        mock_filter_standard.assert_called_once()
        mock_filter_scans.assert_called_once()
        self.view.hide_tof.assert_called_once_with(True)

    def test_update_progress(self):
        self.presenter.update_progress(0, 0)
        self.view.set_progress.assert_called_once_with(0, 0)
        self.view.set_progress.reset_mock()
        self.presenter.update_progress(2, 0)
        self.view.set_progress.assert_called_once_with(2, 0)
        self.view.set_progress.reset_mock()

    def test_right_click(self):
        self.presenter.param_dict = get_file_selector_param_dict()
        self.presenter._right_click(0)
        self.model.open_datafile.assert_called_once_with(0, "C:/data", "C:/stand")

    def test_get_option_dict(self):
        self.presenter.get_option_dict()
        self.view.get_state.assert_called_once()
        self.model.get_data.assert_any_call()
        self.model.get_data.assert_any_call(standard=True)
        self.model.get_data.assert_any_call(full_info=False)
        self.assertEqual(self.model.get_data.call_count, 3)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._auto_select_standard")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter.get_option_dict")
    def test_process_request(self, mock_get_option, mock_auto_select_standard):
        mock_get_option.return_value = {}
        with self.assertRaises(KeyError):
            self.presenter.process_request()
        mock_auto_select_standard.reset_mock()
        mock_get_option.return_value = {"auto_select_standard": True, "standard_data_tree_model": []}
        self.presenter.process_request()
        mock_auto_select_standard.assert_called_once_with(state=2)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.print")
    def test_set_view_from_param(self, mock_print):
        self.presenter.param_dict = get_file_selector_param_dict()
        self.model.check_by_file_numbers.return_value = []
        self.presenter.set_view_from_param()
        self.view.set_state.assert_called_once_with({})
        self.model.check_by_file_numbers.assert_called_once_with([796640, 796640])
        self.model.check_by_file_numbers.return_value = [1, 2]
        self.presenter.set_view_from_param()
        mock_print.assert_called_once_with("Of 0 loaded checked file numbers [1, 2] were not found in list of datafiles")

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._clear_data_trees")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._sample_data_clicked")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._standard_data_clicked")
    def test_tab_got_focus(self, mock_clear_data_trees, mock_standard_data_clicked, mock_sample_data_clicked):
        self.presenter.modus = "123_tof"
        self._standard_data_counter = 0
        self.presenter.tab_got_focus()
        mock_clear_data_trees.assert_called_once()
        mock_standard_data_clicked.assert_called_once()
        mock_sample_data_clicked.assert_called_once()
        self.view.hide_tof.assert_called_once_with(hidden=False)
        self.presenter.modus = "123"
        self._standard_data_counter = 1
        self.view.reset_mock()
        mock_standard_data_clicked.reset_mock()
        mock_sample_data_clicked.reset_mock()
        self.presenter.tab_got_focus()
        mock_standard_data_clicked.assert_not_called()
        mock_sample_data_clicked.assert_not_called()
        self.view.hide_tof.assert_called_once_with(hidden=True)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._read_all")
    def test_process_commandline_request(self, mock_read):
        command_dict = {"files": [{"start": 0, "end": 10}]}
        self.presenter.process_commandline_request(command_dict)
        mock_read.assert_called_once_with(filtered=True, start=0, end=10)
        self.model.check_file_number_range.assert_called_once_with(0, 10)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._format_view")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._filter_scans")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._read_all")
    def test_sample_data_clicked(self, mock_read_all, mock_filter_scans, mock_format_view):
        self.model.get_sample_data_model.return_value = True
        self.model.get_number_of_scans.return_value = []
        self.presenter._sample_data_clicked()
        self.view.set_sample_data_tree_model.assert_called_once_with(True)
        self.model.set_active_model.assert_called_once_with(standard=False)
        mock_read_all.assert_called_once()
        mock_filter_scans.assert_called_once()
        mock_format_view.assert_called_once()

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._format_view")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._check_all_visible_scans")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter.get_option_dict")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_presenter.DNSFileSelectorPresenter._changed_to_standard")
    def test_standard_data_clicked(self, mock_change_std, mock_get_opts, mock_check_scans, mock_format_view):
        self.model.get_standard_data_model.return_value = True
        self.model.get_number_of_scans.return_value = []
        mock_get_opts.return_value = {"auto_select_standard": True}
        self.presenter._standard_data_clicked()
        self.view.set_standard_data_tree_model.assert_called_once_with(True)
        self.model.set_active_model.assert_called_once_with(standard=True)
        mock_change_std.assert_called_once()
        mock_get_opts.assert_called_once()
        mock_check_scans.assert_called_once()
        mock_format_view.assert_called_once()

    def test_format_view(self):
        self.model.get_active_model_column_count.return_value = 5
        self.model.get_scan_range.return_value = range(2)
        self.model.get_number_of_files_in_treeview.return_value = 10
        self.presenter._format_view()
        self.view.set_first_column_spanned.assert_called_once_with(range(2))
        self.view.expand_all.assert_called_once()
        self.view.adjust_treeview_columns_width.assert_called_once_with(5)


if __name__ == "__main__":
    unittest.main()

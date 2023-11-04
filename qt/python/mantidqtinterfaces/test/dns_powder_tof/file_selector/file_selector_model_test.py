# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Presenter for dns path panel.
"""

import unittest
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treemodel import DNSTreeModel
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model import DNSFileSelectorModel
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import dns_file, get_filepath


def get3files():
    # functions, so we do get a new object for every test
    return ["service_774714.d_dat", "service_787463.d_dat", "service_788058.d_dat"]


def get2files():
    return ["service_787463.d_dat", "service_788058.d_dat"]


class DNSFileSelectorModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.parent.update_progress = mock.Mock()
        cls.model = DNSFileSelectorModel(cls.parent)
        cls.filepath = get_filepath()

    def setUp(self):
        self.parent.update_progress.reset_mock()

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFileSelectorModel._save_filelist")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFile", new=dns_file)
    def read3files(self, mock_save):
        # this avoids reading files by patching dnsfile with a corresponding
        # dictionary, there are three different files supported
        self.model.datafiles = get3files()
        self.model.all_datafiles = get3files()
        self.model.datapath = self.filepath
        self.model.read_all(self.model.datafiles, self.model.datapath, {}, [])
        mock_save.assert_called_once()

    def test___init__(self):
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertIsInstance(self.model.sample_data_tree_model, DNSTreeModel)
        self.assertIsInstance(self.model.standard_data_tree_model, DNSTreeModel)
        self.assertTrue(hasattr(self.model, "all_datafiles"))
        self.assertTrue(hasattr(self.model, "old_data_set"))
        self.assertTrue(hasattr(self.model, "active_model"))
        self.assertTrue(hasattr(self.model, "loading_canceled"))

    def test_filter_out_already_loaded(self):
        self.model.old_data_set = [1]
        test_v = self.model._filter_out_already_loaded([1, 2, 3], True)
        self.assertEqual(test_v, [2, 3])
        self.model.old_data_set = [1]
        test_v = self.model._filter_out_already_loaded([1, 2, 3], False)
        self.assertEqual(test_v, [1, 2, 3])

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFileSelectorModel._get_list_of_loaded_files")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.return_filelist")
    def test_set_datafiles_to_load(self, mock_return_filelist, mock_load):
        mock_load.return_value = [1, 2]
        mock_return_filelist.return_value = get3files()
        self.model.old_data_set = get3files()[0]
        test_v = self.model.set_datafiles_to_load(data_path="a", file_number_range=[0, 1000000], filtered=False, watcher=True)
        self.assertEqual(test_v, (2, [1, 2], ["service_787463.d_dat", "service_788058.d_dat"], [0, 1000000]))

    def test_get_start_end_file_numbers(self):
        self.model.all_datafiles = get2files()
        test_v = self.model._get_start_end_file_numbers()
        self.assertEqual(test_v, ["service_787463.d_dat", "service_788058.d_dat"])

    def test_filter_range(self):
        self.model.all_datafiles = get3files()
        test_v = self.model._filter_range(get2files(), [0, 1], filtered=False)
        self.assertEqual(test_v, (get2files(), [0, 1]))

        test_v = self.model._filter_range(get2files(), [None, 1], filtered=True)
        self.assertEqual(test_v, ([], ["service_774714.d_dat", "service_788058.d_dat"]))
        test_v = self.model._filter_range(get3files(), filtered=True, file_number_range=[787463, 787463])
        self.assertEqual(test_v, ([get2files()[0]], [787463, 787463]))

    def test_read_all(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        self.assertFalse(self.model.loading_canceled)
        self.assertEqual(self.parent.update_progress.call_count, 4)
        self.assertEqual(len(self.model.old_data_set), 3)
        self.assertEqual(self.model.sample_data_tree_model.rowCount(), 3)  # three scans

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.return_filelist")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFile", new=dns_file)
    def test_read_standard(self, mock_return_filelist):
        mock_return_filelist.return_value = ["service_774714.d_dat"]
        standard_path = self.filepath
        test_v = self.model.read_standard(standard_path, [])
        self.parent.update_progress.assert_not_called()
        self.assertEqual(test_v, 1)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.unzip_latest_standard")
    def test_try_unzip(self, mock_unzip):
        test_v = self.model.try_unzip("a", "b")
        mock_unzip.assert_called_once_with("a", "b")
        test_v = self.model.try_unzip("a", "")
        self.assertFalse(test_v)

    def test_clear_scans_if_not_sequential(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        self.model._clear_scans_if_not_sequential(True)
        self.assertEqual(self.model.sample_data_tree_model.rowCount(), 3)
        self.model._clear_scans_if_not_sequential(False)
        self.assertEqual(self.model.sample_data_tree_model.rowCount(), 0)

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFileSelectorModel._load_saved_filelist")
    def test_get_list_of_loaded_files(self, mock_load):
        mock_load.return_value = "x"
        test_v = self.model._get_list_of_loaded_files("a", True)
        self.assertEqual(test_v, {})
        test_v = self.model._get_list_of_loaded_files("a", False)
        mock_load.assert_called_once_with("a")
        self.assertEqual(test_v, "x")

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.load_txt")
    def test_load_saved_filelist(self, mock_load):
        mock_load.return_value = [" ; 0" * 14]
        test_v = self.model._load_saved_filelist("123")
        mock_load.assert_called_once_with("last_filelist.txt", "123")
        self.assertIsInstance(test_v["0"], ObjectDict)
        for name in [
            "file_number",
            "det_rot",
            "sample_rot",
            "field",
            "temp_sample",
            "sample",
            "end_time",
            "tof_channels",
            "channel_width",
            "filename",
            "wavelength",
            "selector_speed",
            "scan_number",
            "scan_command",
            "scan_points",
            "new_format",
        ]:
            self.assertTrue(hasattr(test_v["0"], name))
        self.assertEqual(len(test_v), 1)
        mock_load.side_effect = IOError
        test_v = self.model._load_saved_filelist("123")
        self.assertEqual(test_v, {})

    def test_check_last_scans(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        self.model.check_last_scans(1, False, [0, 1, 2])
        self.assertEqual(self.model.sample_data_tree_model.get_checked(False), [788058])
        self.model.check_last_scans(1, True, [0, 1, 2])

    def test_check_by_file_numbers(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        test_v = self.model.check_by_file_numbers([4])
        self.assertEqual(test_v, 1)
        test_v = self.model.check_by_file_numbers([788058])
        self.assertEqual(test_v, 0)

    def test_set_loading_canceled(self):
        self.model.set_loading_canceled()
        self.assertTrue(self.model.loading_canceled)
        self.model.set_loading_canceled(False)
        self.assertFalse(self.model.loading_canceled)

    def test_get_standard_data_model(self):
        tree_model = self.model.get_standard_data_model()
        self.assertIsInstance(tree_model, DNSTreeModel)
        self.assertEqual(self.model.standard_data_tree_model, tree_model)

    def test_get_sample_data_model(self):
        tree_model = self.model.get_sample_data_model()
        self.assertIsInstance(tree_model, DNSTreeModel)
        self.assertEqual(self.model.sample_data_tree_model, tree_model)

    def test_model_is_standard(self):
        self.assertFalse(self.model.model_is_standard())
        self.model.active_model = self.model.standard_data_tree_model
        self.assertTrue(self.model.model_is_standard())

    def test_set_model(self):
        self.model.active_model = ""
        self.assertFalse(self.model.model_is_standard())
        self.model.set_active_model(standard=True)
        self.assertTrue(self.model.model_is_standard())
        self.assertFalse(self.model.active_model == self.model.sample_data_tree_model)
        self.model.set_active_model()
        self.assertTrue(self.model.active_model == self.model.sample_data_tree_model)

    def test_filter_scans_for_boxes(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        filters = [("dummy", True)]
        test_v = self.model.filter_scans_for_boxes(filters, is_tof=True)
        self.assertEqual(test_v, {0, 1, 2})  # hidden scan rows
        filters = [("scan", True)]
        test_v = self.model.filter_scans_for_boxes(filters, is_tof=True)
        self.assertEqual(test_v, {0, 1})

    def test__filter_tof_scans(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        test_v = self.model._filter_tof_scans(is_tof=True)
        self.assertEqual(test_v, {0, 1})
        test_v = self.model._filter_tof_scans(is_tof=False)
        self.assertEqual(test_v, {2})

    def test_filter_standard_types(self):
        self.read3files()  # pylint: disable=no-value-for-parameter
        filters = {"vanadium": True, "nicr": False, "empty": False}
        test_v = self.model.filter_standard_types(filters, True, False)
        self.assertEqual(test_v, {1, 2})  # hidden scan rows
        filters = {"vanadium": False, "nicr": True, "empty": False}
        test_v = self.model.filter_standard_types(filters, True, False)
        self.assertEqual(test_v, {0, 1, 2})  # hidden scan rows

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.open_editor")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSTreeModel.get_filename_from_index")
    def test_open_datafile(self, mock_index, mock_open):
        self.model.active_model = self.model.standard_data_tree_model
        mock_index.return_value = "a"
        self.model.open_datafile(1, "b", "c")
        mock_open.assert_called_once_with("a", "c")
        mock_open.reset_mock()
        self.model.active_model = self.model.sample_data_tree_model
        self.model.open_datafile(1, "b", "c")
        mock_open.assert_called_once_with("a", "b")

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSFile")
    def test_load_file_from_cache_or_new(self, mock_dnsfile):
        filename = get2files()[0]
        # data_path = self.filepath
        mydict = {filename: dns_file("a", filename, [])}
        test_v = self.model._load_file_from_cache_or_new(mydict, filename, "a", [])
        mock_dnsfile.assert_not_called()
        self.assertIsInstance(test_v, ObjectDict)
        test_v = self.model._load_file_from_cache_or_new({}, filename, "a", [])
        mock_dnsfile.assert_called_once_with("a", filename, [])

    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.DNSTreeModel.get_txt")
    @patch("mantidqtinterfaces.dns_powder_tof.file_selector.file_selector_model.save_txt")
    def test_save_filelist(self, mock_save, mock_get_txt):
        mock_get_txt.return_value = "c"
        mock_save.side_effect = PermissionError  # handled exception
        self.model._save_filelist("a")
        mock_get_txt.assert_called_once()
        mock_save.assert_called_once_with("c", "last_filelist.txt", "a")


if __name__ == "__main__":
    unittest.main()

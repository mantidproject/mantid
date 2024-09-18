# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import os

from mantidqtinterfaces.dns_powder_tof.data_structures import dns_tof_powder_dataset
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_tof_powder_dataset import DNSTofDataset
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_fake_tof_data_dic, get_file_selector_full_data


class DNSTofDatasetTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    @classmethod
    def setUpClass(cls):
        # has more data than the one below to test loops
        cls.full_data = get_file_selector_full_data()["full_data"]
        cls.standard_data = get_file_selector_full_data()["standard_data_tree_model"]
        cls.ds = DNSTofDataset(data=cls.full_data, path="C:/123", is_sample=True)

    def set_standard(self):
        self.ds = DNSTofDataset(data=self.standard_data, path="C:/123", is_sample=False)

    def set_sample(self):
        self.ds = DNSTofDataset(data=self.full_data, path="C:/123", is_sample=True)

    def test___init__(self):
        self.assertIsInstance(self.ds, DNSTofDataset)
        self.assertIsInstance(self.ds, ObjectDict)
        self.assertTrue(self.ds.is_sample)
        self.assertEqual(self.ds.banks, [-9.0])
        self.assertIsInstance(self.ds.data_dic, dict)

    def test__get_field_string(self):
        test_v = dns_tof_powder_dataset._get_field_string({2.6: 1, 3.5: 2}, 3)
        self.assertEqual(test_v, "   2.60: 1,\n   3.50: 2")

    def test__get_path_string(self):
        test_v = dns_tof_powder_dataset._get_path_string({"a": {"path": "123"}}, "a")
        self.assertEqual(test_v, "'path': '123',\n")

    def test__get_sample_string(self):
        test_v = dns_tof_powder_dataset._get_sample_string("abc")
        self.assertEqual(test_v, "'abc': {")

    def test_format_dataset(self):
        test_v = self.ds.format_dataset()
        self.assertEqual(
            test_v,
            "{\n    '4p1K_map': {'path': '" + os.path.join("C:/123", "service_******.d_dat") + "',\n                 -9.00: [788058]},\n}",
        )

    def test__get_nb_banks(self):
        test_v = self.ds._get_nb_banks()
        self.assertEqual(test_v, 0)
        test_v = self.ds._get_nb_banks("4p1K_map")
        self.assertEqual(test_v, 1)

    def test__get_vana_filename(self):
        test_v = self.ds.get_vana_scan_name()
        self.assertEqual(test_v, "")
        self.set_standard()
        test_v = self.ds.get_vana_scan_name()
        self.assertEqual(test_v, "_vana")
        self.ds.data_dic = {"_vana": 1, "_vana2": 2}
        test_v = self.ds.get_vana_scan_name()
        self.assertEqual(test_v, "")
        self.set_sample()

    def test__get_empty_filename(self):
        test_v = self.ds.get_empty_scan_name()
        self.assertEqual(test_v, "")
        self.set_standard()
        test_v = self.ds.get_empty_scan_name()
        self.assertEqual(test_v, "_empty")
        self.ds.data_dic = {"_leer": 1, "_empty2": 2}
        test_v = self.ds.get_empty_scan_name()
        self.assertEqual(test_v, "")
        self.set_sample()

    def test_get_sample_filename(self):
        test_v = self.ds.get_sample_filename()
        self.assertEqual(test_v, "4p1K_map")
        self.ds.data_dic = {"4p1K_": 1, "4p1K_maa": 2}
        self.assertIsInstance(test_v, str)
        self.set_sample()

    def test__get_data_path(self):
        entry = self.full_data[0]
        path = "123"
        star_pattern = "*" * len(str(entry["file_number"]))
        test_v = dns_tof_powder_dataset._get_data_path(entry, path)
        self.assertEqual(test_v, os.path.join("123", f"service_{star_pattern}.d_dat"))

    def test__convert_list_to_range(self):
        test_v = dns_tof_powder_dataset._convert_list_to_range(get_fake_tof_data_dic())
        self.assertIsInstance(test_v, dict)
        self.assertEqual(test_v, {"knso": {"path": "C:/data", -6.0: "range(0, 9, 1)", -5.0: "[2, 3, 4]"}})

    def test__add_or_create_filelist(self):
        entry = self.full_data[0]
        dataset = get_fake_tof_data_dic()
        det_rot = 0.0
        datatype = "knso"
        dns_tof_powder_dataset._add_or_create_filelist(dataset, datatype, det_rot, entry)
        self.assertTrue(0.0 in dataset["knso"])
        self.assertEqual(dataset["knso"][0], [788058])
        det_rot = -5.005
        dns_tof_powder_dataset._add_or_create_filelist(dataset, datatype, det_rot, entry)
        self.assertEqual(dataset["knso"][-5], [2, 3, 4, 788058])

    def test_create_dataset(self):
        data = self.full_data
        path = "a"
        star_pattern = "*" * len(str(data[0]["file_number"]))
        test_v = self.ds.create_dataset(data, path)
        self.assertIsInstance(test_v, dict)
        self.assertEqual(test_v, {"4p1K_map": {-9.0: "[788058]", "path": os.path.join(path, f"service_{star_pattern}.d_dat")}})

    def test__create_new_datatype(self):
        entry = self.full_data[0]
        dataset = get_fake_tof_data_dic()
        datatype = "xv"
        det_rot = -5
        path = "C:/123"
        dns_tof_powder_dataset._create_new_datatype(dataset, datatype, det_rot, entry, path)
        self.assertTrue("xv" in dataset)
        self.assertEqual(dataset["xv"][-5], [788058])
        self.assertEqual(dataset["xv"]["path"], os.path.join("C:/123", "service_" + "*" * len("788058") + ".d_dat"))

    def test__get_closest_bank_key(self):
        test_v = dns_tof_powder_dataset._get_closest_bank_key({0: 1, 2: 1}, 0.1)
        self.assertEqual(test_v, 0.1)
        test_v = dns_tof_powder_dataset._get_closest_bank_key({0: 1, 2: 1}, 2.01)
        self.assertEqual(test_v, 2)


if __name__ == "__main__":
    unittest.main()

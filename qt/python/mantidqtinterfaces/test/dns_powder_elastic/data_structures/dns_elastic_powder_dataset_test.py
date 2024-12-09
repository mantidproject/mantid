# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import os
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset import DNSElasticDataset
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_file_selector_full_data

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset import (
    round_step,
    get_omega_step,
    list_to_set,
    automatic_omega_binning,
    get_proposal_from_filename,
    get_sample_fields,
    create_dataset,
    get_datatype_from_sample_name,
    remove_unnecessary_standard_fields,
    get_bank_positions,
)


class DNSDatasetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.full_data = get_file_selector_full_data()["full_data"]
        cls.standard_data = get_file_selector_full_data()["standard_data_tree_model"]
        cls.ds = DNSElasticDataset(data=cls.full_data, path="C:/123", is_sample=True)
        cls.data_dic = {"4p1K_map": {"path": "C:/123\\service", "z_nsf": [788058]}}

    def setUp(self):
        self.ds.data_dic = {"4p1K_map": {"path": "C:/123\\service", "z_nsf": [788058]}}

    def test___init__(self):
        self.assertIsInstance(self.ds, DNSElasticDataset)
        self.assertIsInstance(self.ds, ObjectDict)
        self.assertTrue(self.ds.is_sample)
        self.assertTrue(hasattr(self.ds, "banks"))
        self.assertTrue(hasattr(self.ds, "is_sample"))
        self.assertTrue(hasattr(self.ds, "fields"))
        self.assertTrue(hasattr(self.ds, "omega"))
        self.assertTrue(hasattr(self.ds, "data_dic"))
        self.assertIsInstance(self.ds.data_dic, dict)

    #
    def test_format_dataset(self):
        test_v = self.ds.format_dataset()
        self.assertEqual(test_v, "{\n    '4p1K_map': {'path': 'C:/123\\serv" "ice',\n                 'z_nsf': [788058]}" ",\n}")

    def test_create_subtract(self):
        test_v = self.ds.create_subtract()
        self.assertEqual(test_v, ["4p1K_map_z_nsf"])

    def test_get_number_banks(self):
        test_v = self.ds.get_number_banks(sample_type="4p1K_map")
        self.assertEqual(test_v, 1)
        test_v = self.ds.get_number_banks()
        self.assertEqual(test_v, 0)

    def test_round_step(self):
        test_v = round_step(1.99, rounding_limit=0.05)
        self.assertEqual(test_v, 2)
        test_v = round_step(0.38, rounding_limit=0.05)
        self.assertEqual(test_v, 0.4)
        test_v = round_step(0.47, rounding_limit=0.05)
        self.assertEqual(test_v, 1 / 2)
        test_v = round_step(0.1, rounding_limit=0.05)
        self.assertEqual(test_v, 0.1)

    def test_get_omega_step(self):
        test_v = get_omega_step([0, 1, 2])
        self.assertEqual(test_v, 1)
        test_v = get_omega_step([0.1, 0.2, 0.3])
        self.assertAlmostEqual(test_v, 0.1)
        test_v = get_omega_step([0.1, 0.2, 2.3])
        self.assertAlmostEqual(test_v, 0.1)
        test_v = get_omega_step([0.1])
        self.assertEqual(test_v, 1)

    def test_list_to_set(self):
        test_v = list_to_set([-4, -5, -3])
        self.assertEqual(test_v, [-5, -4, -3])
        test_v = list_to_set([-4, -5, -4.97])
        self.assertEqual(test_v, [-5, -4])
        test_v = list_to_set([-4.1])
        self.assertEqual(test_v, [-4.1])
        test_v = list_to_set([])
        self.assertEqual(test_v, [])

    @patch("mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset.DNSBinning")
    def test_automatic_omega_binning(self, mock_binning):
        test_v = automatic_omega_binning(self.full_data)
        self.assertEqual(test_v, mock_binning.return_value)
        mock_binning.assert_called_once_with(304.0, 304.0, 0)

    def test_get_proposal_from_filename(self):
        test_v = get_proposal_from_filename("p678_000123.d_dat", 123)
        self.assertEqual(test_v, "p678")

    def test_get_sample_fields(self):
        test_v = get_sample_fields(self.full_data)
        self.assertEqual(test_v, {"z_nsf"})

    def test_create_dataset(self):
        test_v = create_dataset(self.full_data, "C:/123")
        star_pattern = "*" * len(str(self.full_data[0]["file_number"]))
        self.assertEqual(test_v, {"4p1K_map": {"z_nsf": [788058], "path": os.path.join("C:/123", f"service_{star_pattern}.d_dat")}})
        test_v = create_dataset(self.standard_data, "C:/123")
        self.assertEqual(
            test_v,
            {
                "empty": {"x_nsf": [788058], "path": os.path.join("C:/123", "test_empty.d_dat")},
                "vana": {"z_nsf": [788058, 788059], "path": os.path.join("C:/123", "test_vana.d_dat")},
            },
        )

    def test_get_datatype_from_sample_name(self):
        test_v = get_datatype_from_sample_name("_12 3")
        self.assertEqual(test_v, "123")
        test_v = get_datatype_from_sample_name("leer")
        self.assertEqual(test_v, "empty")

    def test_remove_non_measured_fields(self):
        test_v = remove_unnecessary_standard_fields(self.standard_data, ["x_sf"], False)
        self.assertEqual(test_v, [])
        test_v = remove_unnecessary_standard_fields(self.standard_data, ["z_nsf"], False)
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 2)
        self.assertEqual(test_v[0]["file_number"], 788058)

    def test_get_bank_positions(self):
        test_v = get_bank_positions(self.full_data)
        self.assertEqual(test_v, [-9])
        test_v = get_bank_positions(self.standard_data)
        self.assertEqual(test_v, [-10, -9.04, -9])


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import os

from mantidqtinterfaces.dns_powder_tof.data_structures import \
    dns_tof_powder_dataset
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_tof_powder_dataset\
    import DNSTofDataset
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_fake_tof_datadic, get_file_selector_fulldat)


class DNSTofDatasetTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    @classmethod
    def setUpClass(cls):
        # has more data than the one below to test loops
        cls.fulldata = get_file_selector_fulldat()['full_data']
        cls.standarddata = get_file_selector_fulldat()['standard_data']
        cls.ds = DNSTofDataset(data=cls.fulldata, path='C:/123',
                               is_sample=True)

    def set_standard(self):
        self.ds = DNSTofDataset(data=self.standarddata,
                                path='C:/123',
                                is_sample=False)

    def set_sample(self):
        self.ds = DNSTofDataset(data=self.fulldata,
                                path='C:/123',
                                is_sample=True)

    def test___init__(self):
        self.assertIsInstance(self.ds, DNSTofDataset)
        self.assertIsInstance(self.ds, ObjectDict)
        self.assertTrue(self.ds.is_sample)
        self.assertEqual(self.ds.banks, [-9.0])
        self.assertIsInstance(self.ds.data_dic, dict)

    def test__get_max_key_length(self):
        testv = dns_tof_powder_dataset._get_max_key_length(self.ds.data_dic)
        self.assertEqual(testv, 18)

    def test__get_field_string(self):
        testv = dns_tof_powder_dataset._get_field_string({2.6: 1, 3.5: 2}, 3)
        self.assertEqual(testv, "    2.60: 1,\n    3.50: 2")

    def test__get_path_string(self):
        testv = dns_tof_powder_dataset._get_path_string({'a': {
            'path': '123'
        }}, 'a')
        self.assertEqual(testv, "'path': '123',\n")

    def test__get_sample_string(self):
        testv = dns_tof_powder_dataset._get_sample_string('abc', 2)
        self.assertEqual(testv, "'abc': {")

    def test_format_dataset(self):
        testv = self.ds.format_dataset()
        self.assertEqual(
            testv,
            "{\n     '4p1K_map': {'path': '" + os.path.join('C:/123', 'service') + "',\n               "
            "    -9.00: [788058]},\n}")

    def test__get_nb_banks(self):
        testv = self.ds._get_nb_banks()
        self.assertEqual(testv, 0)
        testv = self.ds._get_nb_banks('4p1K_map')
        self.assertEqual(testv, 1)

    def test__get_vana_filename(self):
        testv = self.ds.get_vana_filename()
        self.assertEqual(testv, '')
        self.set_standard()
        testv = self.ds.get_vana_filename()
        self.assertEqual(testv, '_vana')
        self.ds.data_dic = {'_vana': 1, '_vana2': 2}
        testv = self.ds.get_vana_filename()
        self.assertEqual(testv, '')
        self.set_sample()

    def test__get_empty_filename(self):
        testv = self.ds.get_empty_filename()
        self.assertEqual(testv, '')
        self.set_standard()
        testv = self.ds.get_empty_filename()
        self.assertEqual(testv, '_empty')
        self.ds.data_dic = {'_leer': 1, '_empty2': 2}
        testv = self.ds.get_empty_filename()
        self.assertEqual(testv, '')
        self.set_sample()

    def test_get_sample_filename(self):
        testv = self.ds.get_sample_filename()
        self.assertEqual(testv, '4p1K_map')
        self.ds.data_dic = {'4p1K_': 1, '4p1K_maa': 2}
        self.assertIsInstance(testv, str)
        self.set_sample()

    def test__get_datapath(self):
        entry = self.fulldata[0]
        path = '123'
        testv = dns_tof_powder_dataset._get_datapath(entry, path)
        self.assertEqual(testv, os.path.join('123', 'service'))

    def test__convert_list_to_range(self):
        testv = dns_tof_powder_dataset._convert_list_to_range(
            get_fake_tof_datadic())
        self.assertIsInstance(testv, dict)
        self.assertEqual(testv, {
            'knso': {
                'path': 'C:/data',
                -6.0: 'range(0, 9, 1)',
                -5.0: '[2, 3, 4]'
            }
        })

    def test__add_or_create_filelist(self):
        entry = self.fulldata[0]
        dataset = get_fake_tof_datadic()
        det_rot = 0.0
        datatype = 'knso'
        dns_tof_powder_dataset._add_or_create_filelist(dataset, datatype,
                                                       det_rot, entry)
        self.assertTrue(0.0 in dataset['knso'])
        self.assertEqual(dataset['knso'][0], [788058])
        det_rot = -5.005
        dns_tof_powder_dataset._add_or_create_filelist(dataset, datatype,
                                                       det_rot, entry)
        self.assertEqual(dataset['knso'][-5], [2, 3, 4, 788058])

    def test_create_dataset(self):
        data = self.fulldata
        path = 'a'
        testv = self.ds.create_dataset(data, path)
        self.assertIsInstance(testv, dict)
        self.assertEqual(
            testv, {'4p1K_map': {
                -9.0: '[788058]',
                'path': os.path.join('a', 'service')
            }})

    def test__create_new_datatype(self):
        entry = self.fulldata[0]
        dataset = get_fake_tof_datadic()
        datatype = 'xv'
        det_rot = -5
        path = 'C:/123'
        dns_tof_powder_dataset._create_new_datatype(dataset, datatype, det_rot,
                                                    entry, path)
        self.assertTrue('xv' in dataset)
        self.assertEqual(dataset['xv'][-5], [788058])
        self.assertEqual(dataset['xv']['path'], os.path.join('C:/123', 'service'))

    def test__get_closest_bank_key(self):
        testv = dns_tof_powder_dataset._get_closest_bank_key({0: 1, 2: 1}, 0.1)
        self.assertEqual(testv, 0.1)
        testv = dns_tof_powder_dataset._get_closest_bank_key({
            0: 1,
            2: 1
        }, 2.01)
        self.assertEqual(testv, 2)


if __name__ == '__main__':
    unittest.main()

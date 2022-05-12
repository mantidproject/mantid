# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which loads and stores a single DNS datafile in a dictionary
"""

import unittest
from unittest.mock import patch
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_dataset import \
    DNSDataset
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import \
    get_file_selector_fulldat

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_dataset import (
    create_script_name, automatic_ttheta_binning, get_ttheta_step, round_step,
    get_omega_step, list_to_set, automatic_omega_binning,
    get_proposal_from_filname, get_sample_fields, create_dataset,
    get_datatype_from_samplename, remove_non_measured_fields,
    get_bank_positions)


class DNSDatasetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fulldata = get_file_selector_fulldat()['full_data']
        cls.standarddata = get_file_selector_fulldat()['standard_data']
        cls.ds = DNSDataset(data=cls.fulldata, path='C:/123', issample=True)
        cls.datadic = {'4p1K_map': {'path': 'C:/123\\service',
                                    'z_nsf': [788058]}}

    def setUp(self):
        self.ds.datadic = {'4p1K_map': {'path': 'C:/123\\service',
                                        'z_nsf': [788058]}}

    def test___init__(self):
        self.assertIsInstance(self.ds, DNSDataset)
        self.assertIsInstance(self.ds, ObjectDict)
        self.assertTrue(self.ds.issample)
        self.assertTrue(hasattr(self.ds, 'banks'))
        self.assertTrue(hasattr(self.ds, 'issample'))
        self.assertTrue(hasattr(self.ds, 'scriptname'))
        self.assertTrue(hasattr(self.ds, 'fields'))
        self.assertTrue(hasattr(self.ds, 'ttheta'))
        self.assertTrue(hasattr(self.ds, 'omega'))
        self.assertTrue(hasattr(self.ds, 'datadic'))
        self.assertIsInstance(self.ds.datadic, dict)

    #
    def test_format_dataset(self):
        testv = self.ds.format_dataset()
        self.assertEqual(testv, "{\n    '4p1K_map' : { 'path' : 'C:/123\\serv"
                                "ice',\n                  'z_nsf' : [788058]}"
                                ",\n}")

    def test_create_plotlist(self):
        testv = self.ds.create_plotlist()
        self.assertEqual(testv, ['4p1K_map_z_nsf'])

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.dns_dataset.'
           'interpolate_standard')
    def test_interpolate_standard(self, mock_ip):
        mock_ip.return_value = {}, False
        mock_parent = mock.Mock()
        self.ds.interpolate_standard([-9], '123', mock_parent)
        self.assertEqual(self.ds.datadic, {})

        mock_ip.assert_called_once_with(self.datadic, [-9], '123')
        mock_parent.raise_error.assert_not_called()
        mock_ip.return_value = {}, True
        self.ds.interpolate_standard([-9], '123', mock_parent)
        mock_parent.raise_error.assert_called_once()

    def test_get_nb_banks(self):
        testv = self.ds.get_nb_banks(sampletype='4p1K_map')
        self.assertEqual(testv, 1)
        testv = self.ds.get_nb_banks()
        self.assertEqual(testv, 0)

    # helper function
    def test_create_script_name(self):
        testv = create_script_name(self.fulldata)
        self.assertEqual(testv, '4p1K_map_788058_to_788058.py')

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_dataset.DNSBinning')
    def test_automatic_ttheta_binning(self, mock_binning):
        testv = automatic_ttheta_binning(self.fulldata)
        self.assertEqual(testv, mock_binning.return_value)
        mock_binning.assert_called_once_with(9.0, 124.0, 5.0)

    def test_get_ttheta_step(self):
        testv = get_ttheta_step([0, 1, 2])
        self.assertEqual(testv, 1)
        testv = get_ttheta_step([0, 0.01, 2])
        self.assertEqual(testv, 2)
        testv = get_ttheta_step([0, 0.33, 0.66, 1.33, 1.66])
        self.assertEqual(testv, 1 / 3)
        testv = get_ttheta_step([0, 0.49, 1.01])
        self.assertEqual(testv, 1 / 2)

    def test_round_step(self):
        testv = round_step(1.99, rounding_limit=0.05)
        self.assertEqual(testv, 2)
        testv = round_step(0.38, rounding_limit=0.05)
        self.assertEqual(testv, 1 / 3)
        testv = round_step(0.47, rounding_limit=0.05)
        self.assertEqual(testv, 1 / 2)
        testv = round_step(0.1, rounding_limit=0.05)
        self.assertEqual(testv, 0.1)

    def test_get_omega_step(self):
        testv = get_omega_step([0, 1, 2])
        self.assertEqual(testv, 1)
        testv = get_omega_step([0.1, 0.2, 0.3])
        self.assertAlmostEqual(testv, 0.1)
        testv = get_omega_step([0.1, 0.2, 2.3])
        self.assertAlmostEqual(testv, 0.1)
        testv = get_omega_step([0.1])
        self.assertEqual(testv, 1)

    def test_list_to_set(self):
        testv = list_to_set([-4, -5, -3])
        self.assertEqual(testv, [-5, -4, -3])
        testv = list_to_set([-4, -5, -4.97])
        self.assertEqual(testv, [-5, -4])
        testv = list_to_set([-4.1])
        self.assertEqual(testv, [-4.1])
        testv = list_to_set([])
        self.assertEqual(testv, [])

    @patch('mantidqtinterfaces.dns_powder_elastic.data_structures.'
           'dns_dataset.DNSBinning')
    def test_automatic_omega_binning(self, mock_binning):
        testv = automatic_omega_binning(self.fulldata)
        self.assertEqual(testv, mock_binning.return_value)
        mock_binning.assert_called_once_with(304.0, 304.0, 1)

    def test_get_proposal_from_filname(self):
        testv = get_proposal_from_filname('p678_000123.d_dat', 123)
        self.assertEqual(testv, 'p678')

    def test_get_sample_fields(self):
        testv = get_sample_fields(self.fulldata)
        self.assertEqual(testv, {'z7_nsf'})

    def test_create_dataset(self):
        testv = create_dataset(self.fulldata, 'C:/123')
        self.assertEqual(testv, {
            '4p1K_map': {'z_nsf': [788058], 'path': 'C:/123\\service'}})
        testv = create_dataset(self.standarddata, 'C:/123')
        self.assertEqual(testv, {
            'empty': {'x_nsf': [788058], 'path': 'C:/123\\test_empty.d_dat'},
            'vana': {'z_nsf': [788058, 788059],
                     'path': 'C:/123\\test_vana.d_dat'}})

    def test_get_datatype_from_samplename(self):
        testv = get_datatype_from_samplename('_12 3')
        self.assertEqual(testv, '123')
        testv = get_datatype_from_samplename('leer')
        self.assertEqual(testv, 'empty')

    def test_remove_non_measured_fields(self):
        testv = remove_non_measured_fields(self.standarddata, ['x_sf'])
        self.assertEqual(testv, [])
        testv = remove_non_measured_fields(self.standarddata, ['z7_nsf'])
        self.assertIsInstance(testv, list)
        self.assertEqual(len(testv), 2)
        self.assertEqual(testv[0]['file_number'], 788058)

    def test_get_bank_positions(self):
        testv = get_bank_positions(self.fulldata)
        self.assertEqual(testv, [-9])
        testv = get_bank_positions(self.standarddata)
        self.assertEqual(testv, [-9.04, -10])


if __name__ == '__main__':
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
presenter for dns path panel
"""

import unittest
from os.path import expanduser
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns_powder_tof.paths.path_model import DNSPathModel


class DNSPathModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.model = DNSPathModel(cls)

    def test__init__(self):
        self.assertIsInstance(self.model, DNSObsModel)

    @patch('mantidqtinterfaces.dns_powder_tof.paths.path_model.DNSFile')
    @patch('mantidqtinterfaces.dns_powder_tof.paths.path_model.glob.iglob')
    def test_get_user_and_propnumber(self, mock_glob, mock_dnsfile):
        mock_dnsfile.return_value = {'users': 'Thomas', 'proposal': 'p123',
                                     'new_format': True}
        mock_glob.return_value = iter((1, 2, 3))
        testv = self.model.get_user_and_propnumber('')
        self.assertEqual(testv, ['Thomas', 'p123'])
        mock_glob.side_effect = StopIteration()
        testv = self.model.get_user_and_propnumber('')
        self.assertEqual(testv, ['', ''])

    @patch('mantidqtinterfaces.dns_powder_tof.paths.path_model.os.remove')
    @patch('mantidqtinterfaces.dns_powder_tof.paths.path_model.os.path.isfile')
    def test_clear_cache(self, mock_isfile, mock_remove):
        mock_isfile.return_value = True
        self.model.clear_cache(path='123')
        mock_remove.assert_called_once_with('123/last_filelist.txt')

    def test_get_start_path_for_dialog(self):
        testv = self.model.get_start_path_for_dialog('')
        self.assertEqual(testv, expanduser("~"))
        testv = self.model.get_start_path_for_dialog('1')
        self.assertEqual(testv, '1')


if __name__ == '__main__':
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which loads and stores a single DNS datafile in a dictionary
"""

from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import unittest
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import load_txt
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_dataset, get_filepath)


class DNSFileTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.filepath = get_filepath()
        cls.data = get_dataset()
        cls.file = DNSFile('', "dnstof.d_dat")
        cls.txt = "".join(load_txt("dnstof.d_dat"))

    def test___init__(self):
        self.assertIsInstance(self.file, ObjectDict)
        self.assertIsInstance(self.file, DNSFile)

    @patch('mantidqtinterfaces.dns_powder_tof.data_structures.dns_file.'
           'save_txt')
    def test_write(self, mock_save):
        self.file.write('', '123.d_dat')
        mock_save.assert_called_with(self.txt, '123.d_dat', '')

    def test_read(self):
        # already read in init
        self.assertAlmostEqual(self.file['det_rot'], -7.5)
        self.assertEqual(self.file.counts[3, 0], 3)


if __name__ == '__main__':
    unittest.main()

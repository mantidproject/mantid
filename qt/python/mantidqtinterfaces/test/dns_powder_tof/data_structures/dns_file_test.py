# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.api import FileFinder

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import load_txt
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_dataset, get_filepath


class DNSFileTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.filepath = get_filepath()
        cls.data = get_dataset()
        cls.full_filename = FileFinder.Instance().getFullPath("dnstof.d_dat")
        cls.file = DNSFile("", cls.full_filename, [])
        cls.txt = "".join(load_txt(cls.full_filename))

    def test___init__(self):
        self.assertIsInstance(self.file, ObjectDict)
        self.assertIsInstance(self.file, DNSFile)

    def test_read(self):
        # already read in init
        self.assertAlmostEqual(self.file["det_rot"], -7.5)
        self.assertEqual(self.file.counts[3, 0], 3)


if __name__ == "__main__":
    unittest.main()

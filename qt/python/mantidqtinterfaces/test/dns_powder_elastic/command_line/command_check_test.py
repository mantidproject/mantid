# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_elastic.command_line.command_check import CommandLineReader
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict


class CommandLineReaderTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    @classmethod
    def setUpClass(cls):
        cls.colire = CommandLineReader()

    def test___init__(self):
        self.assertIsInstance(self.colire, CommandLineReader)
        self.assertIsInstance(self.colire, ObjectDict)
        self.assertEqual(self.colire.files, [])

    @patch("mantidqtinterfaces.dns_powder_elastic.command_line.command_check.CommandLineReader._parse_file_command")
    def test_read(self, mock_parse):
        # sc elastic
        mock_parse.return_value = ["786359", "788058", "100"]
        cla = (
            "-files p164260000 100 2 786359 788058"
            " 4p1K_map.d_dat -dx 3.54 -dy 6.13 -nx"
            " 1,1,0 -ny 1,-1,0 -cz standards_rc47_v4.zip"
            " -v -fr 0.0"
        ).split(" ")
        self.colire.read(cla)
        self.assertEqual(self.colire.omega_offset, "100")
        self.assertEqual(self.colire.hkl1, "1,1,0")
        self.assertEqual(self.colire.hkl2, "1,-1,0")
        self.assertEqual(self.colire.det_efficiency, True)
        self.assertEqual(self.colire.flipping_ratio, True)
        self.assertEqual(self.colire.separation_xyz, False)
        self.assertEqual(self.colire.separation_coh_inc, False)
        self.assertIsInstance(self.colire.files[0]["path"], str)
        self.assertTrue(self.colire.files[0]["path"])

        self.assertEqual(self.colire.files[0]["ffnmb"], "786359")
        self.assertEqual(self.colire.cz, "standards_rc47_v4.zip")
        self.assertEqual(self.colire.files[0]["lfnmb"], "788058")
        # new format with less arguments
        mock_parse.return_value = ["786357", "788059", "101"]

        cla = ("-new 786357 788059 101 4p1K_map.d_dat -dx 3.54 -dy 6.13 -nx 1,1,0 -ny 1,-1,0 -cz standards_rc47_v4.zip -v -fr 0.0").split(
            " "
        )
        self.colire.read(cla)
        self.assertEqual(self.colire.omega_offset, "101")
        self.assertTrue(self.colire.files[0]["path"])
        self.assertEqual(self.colire.files[0]["ffnmb"], "786357")
        self.assertEqual(self.colire.files[0]["lfnmb"], "788059")
        # test rest, not a valid command, but does not matter here
        cla = ("-powder -files p164260000 0 2 786359 788058 4p1K_map.d_dat -xyz -sep-nonmag -tof").split(" ")
        self.colire.read(cla)
        self.assertTrue(self.colire.powder)
        self.assertTrue(self.colire.separation_xyz)
        self.assertTrue(self.colire.separation_coh_inc)
        self.assertTrue(self.colire.tof)

    @patch("mantidqtinterfaces.dns_powder_elastic.command_line.command_check.DNSFile")
    def test__parse_old_filenumbers(self, mock_dnsfile):
        mock_dnsfile.return_value = {"file_number": "1230"}
        test_v = self.colire._parse_old_filenumbers("23", "p151", "0_1", "a")
        self.assertEqual(test_v, ["1", "0"])

    @patch("mantidqtinterfaces.dns_powder_elastic.command_line.command_check.CommandLineReader._parse_old_filenumbers")
    @patch("mantidqtinterfaces.dns_powder_elastic.command_line.command_check.os.path.isfile")
    def test__get_fixpart_fnb(self, mock_isfile, mock_parse):
        # function can fail if the file number is used in the
        # prefix or postfix and the old format is used
        # there is no way to test for this, has to be avoided by user
        mock_isfile.return_value = False
        mock_parse.return_value = ["1", "2"]
        test_v = self.colire._get_fix_part_fnb("54858", "p123_2", "3.d_dat", "b")
        self.assertEqual(test_v, ["2", "3"])
        mock_isfile.return_value = True
        test_v = self.colire._get_fix_part_fnb("54858", "p123_2", "3.d_dat", "b")
        self.assertEqual(test_v, ["1", "2"])

    @patch("mantidqtinterfaces.dns_powder_elastic.command_line.command_check.CommandLineReader._get_fix_part_fnb")
    def test__parse_file_command(self, mock_get_fix_fnb):
        mock_get_fix_fnb.return_value = ["00002", "1"]
        cla = "-files p164260002 100 2 786359 788058 14p1K_map.d_dat".split(" ")
        test_v = self.colire._parse_file_command(["abcd", "123"], "")
        self.assertEqual(test_v, [None, None, 0, ""])
        test_v = self.colire._parse_file_command(["-files"], "")
        self.assertEqual(test_v, [None, None, 0, ""])
        test_v = self.colire._parse_file_command(cla, "")
        self.assertEqual(test_v, ["000027863591", "000027880581", "100"])


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
from shutil import rmtree

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from unittest.mock import patch

dir_path = "Engineering.gui.engineering_diffraction.tabs.common"


class VanadiumCorrectionsTest(unittest.TestCase):
    def setUp(self):
        self.directory_name = tempfile.mkdtemp()

    def tearDown(self):
        rmtree(self.directory_name)

    @patch(dir_path + ".vanadium_corrections.create_vanadium_corrections")
    @patch(dir_path + ".vanadium_corrections.check_workspaces_exist")
    def test_fetch_correction_workspaces_when_not_cached(self, wspexist, van_correction):
        van_correction.return_value = ("integ", "processed")
        wspexist.return_value = False
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(1, van_correction.call_count)

    @patch(dir_path + ".vanadium_corrections.create_vanadium_corrections")
    @patch(dir_path + ".vanadium_corrections.check_workspaces_exist")
    @patch(dir_path + ".vanadium_corrections.Ads")
    def test_fetch_correction_workspaces_when_cached(self, ads, wspexist, van_correction):
        van_correction.return_value = ("integ", "processed")
        wspexist.return_value = True
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(0, van_correction.call_count)
        self.assertEqual(2, ads.retrieve.call_count)

    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_processed_instrument")
    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_integral")
    @patch(dir_path + ".vanadium_corrections.Load")
    def test_create_vanadium_corrections(self, load, calc_integ, calc_processed):
        van_path = "mocked/path"
        calc_integ.return_value = "mock_integral"
        calc_processed.return_value = "mock_processed"
        integral, processed = vanadium_corrections.create_vanadium_corrections(van_path, "ENGINX")
        self.assertEqual("mock_integral", integral)
        self.assertEqual("mock_processed", processed)
        self.assertEqual(2, load.call_count)


if __name__ == '__main__':
    unittest.main()

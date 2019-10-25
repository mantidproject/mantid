# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest
import tempfile
from shutil import rmtree
from os import path

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from mantid.py3compat.mock import patch

dir_path = "Engineering.gui.engineering_diffraction.tabs.common"


class VanadiumCorrectionsTest(unittest.TestCase):
    def setUp(self):
        self.directory_name = tempfile.mkdtemp()

    def tearDown(self):
        rmtree(self.directory_name)

    @patch(dir_path + ".vanadium_corrections._save_correction_files")
    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_correction")
    @patch(dir_path + ".vanadium_corrections.Load")
    @patch(dir_path + ".vanadium_corrections._generate_saved_workspace_file_paths")
    def test_fetch_correction_workspaces_when_not_cached(self, gen_paths, load_alg, van_correction,
                                                         save):
        gen_paths.return_value = (path.join(self.directory_name,
                                            "123" + vanadium_corrections.SAVED_FILE_INTEG_SUFFIX),
                                  path.join(self.directory_name,
                                            "123" + vanadium_corrections.SAVED_FILE_CURVE_SUFFIX))
        van_correction.return_value = ("integ", "curves")
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(0, load_alg.call_count)
        self.assertEqual(1, van_correction.call_count)
        self.assertEqual(1, save.call_count)

    @patch(dir_path + ".vanadium_corrections._save_correction_files")
    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_correction")
    @patch(dir_path + ".vanadium_corrections.Load")
    @patch(dir_path + ".vanadium_corrections._generate_saved_workspace_file_paths")
    def test_fetch_correction_workspaces_when_cached(self, gen_paths, load_alg, van_correction,
                                                     save):
        temp_integ = tempfile.NamedTemporaryFile(dir=self.directory_name)
        temp_curve = tempfile.NamedTemporaryFile(dir=self.directory_name)
        gen_paths.return_value = (path.join(self.directory_name, temp_integ.name),
                                  path.join(self.directory_name, temp_curve.name))
        van_correction.return_value = ("integ", "curves")
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(2, load_alg.call_count)
        self.assertEqual(0, van_correction.call_count)
        self.assertEqual(0, save.call_count)

    @patch(dir_path + ".vanadium_corrections.makedirs")
    def test_file_path_generation(self, makedirs):
        vanadium_run_number = "1234"
        engineering_path = path.join(path.expanduser("~"), "Engineering_Mantid")
        if path.exists(engineering_path):
            rmtree(engineering_path)
        output = vanadium_corrections._generate_saved_workspace_file_paths(vanadium_run_number)
        self.assertEqual(output,
                         (path.join(path.expanduser("~"), "Engineering_Mantid", "Vanadium_Runs",
                                    "1234_precalculated_vanadium_run_integration.nxs"),
                          path.join(path.expanduser("~"), "Engineering_Mantid", "Vanadium_Runs",
                                    "1234_precalculated_vanadium_run_bank_curves.nxs")))
        self.assertEqual(1, makedirs.call_count)


if __name__ == '__main__':
    unittest.main()

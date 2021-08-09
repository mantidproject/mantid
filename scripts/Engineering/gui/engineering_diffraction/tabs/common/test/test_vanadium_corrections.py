# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
from shutil import rmtree
from os import path

from Engineering.gui.engineering_diffraction.tabs.common import vanadium_corrections
from unittest.mock import patch

dir_path = "Engineering.gui.engineering_diffraction.tabs.common"


class VanadiumCorrectionsTest(unittest.TestCase):
    def setUp(self):
        self.directory_name = tempfile.mkdtemp()

    def tearDown(self):
        rmtree(self.directory_name)

    @patch(dir_path + ".vanadium_corrections.save_van_workspace")
    @patch(dir_path + ".vanadium_corrections.create_vanadium_corrections")
    @patch(dir_path + ".vanadium_corrections.Load")
    @patch(dir_path + ".vanadium_corrections.generate_van_ws_file_paths")
    def test_fetch_correction_workspaces_when_not_cached(self, gen_paths, load_alg, van_correction,
                                                         save):
        gen_paths.return_value = (path.join(self.directory_name, "123" + vanadium_corrections.SAVED_FILE_INTEG_SUFFIX),
                                  path.join(self.directory_name, "123"
                                            + vanadium_corrections.SAVED_FILE_PROCESSED_SUFFIX))
        van_correction.return_value = ("integ", "processed")
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(0, load_alg.call_count)
        self.assertEqual(1, van_correction.call_count)
        self.assertEqual(2, save.call_count)

    @patch(dir_path + ".vanadium_corrections.save_van_workspace")
    @patch(dir_path + ".vanadium_corrections.create_vanadium_corrections")
    @patch(dir_path + ".vanadium_corrections.Load")
    @patch(dir_path + ".vanadium_corrections.generate_van_ws_file_paths")
    def test_fetch_correction_workspaces_when_cached(self, gen_paths, load_alg, van_correction,
                                                     save):
        temp_integ = tempfile.NamedTemporaryFile(dir=self.directory_name)
        temp_processed = tempfile.NamedTemporaryFile(dir=self.directory_name)
        gen_paths.return_value = (path.join(self.directory_name, temp_integ.name),
                                  path.join(self.directory_name, temp_processed.name))
        van_correction.return_value = ("integ", "processed")
        vanadium_corrections.fetch_correction_workspaces("something/somewhere/ENGINX123.nxs",
                                                         "ENGINX")
        self.assertEqual(2, load_alg.call_count)
        self.assertEqual(0, van_correction.call_count)
        self.assertEqual(0, save.call_count)
        temp_integ.close()
        temp_processed.close()

    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_processed_instrument")
    @patch(dir_path + ".vanadium_corrections._calculate_vanadium_integral")
    @patch(dir_path + ".vanadium_corrections.Load")
    def test_create_vanadium_corrections(self, load, calc_integ, calc_processed):
        van_path = "mocked/path"
        calc_integ.return_value = "mock_integral"
        calc_processed.return_value = "mock_processed"
        integral, processed = vanadium_corrections.create_vanadium_corrections(van_path)
        self.assertEqual("mock_integral", integral)
        self.assertEqual("mock_processed", processed)
        self.assertEqual(2, load.call_count)

    @patch(dir_path + ".vanadium_corrections.SaveNexus")
    def test_save_van_workspace(self, save):
        vanadium_corrections.save_van_workspace("ws", "out/path")
        self.assertEqual(1, save.call_count)

    @patch(dir_path + ".vanadium_corrections.output_settings.get_output_path")
    @patch(dir_path + ".vanadium_corrections.makedirs")
    def test_generate_van_ws_file_paths_no_rb(self, makedirs, out_path):
        usr_path = path.expanduser("~")
        out_path.return_value = path.join(usr_path, "Test_Directory")
        vanadium_run_number = "1234"
        engineering_path = path.join(usr_path, "Test_Directory")
        if path.exists(engineering_path):
            rmtree(engineering_path)
        expected_integral = (path.join(usr_path, "Test_Directory", "Vanadium_Runs",
                             "1234_precalculated_vanadium_run_integration.nxs"))
        expected_processed = (path.join(usr_path, "Test_Directory", "Vanadium_Runs",
                              "1234_precalculated_vanadium_run_processed_instrument.nxs"))
        output = vanadium_corrections.generate_van_ws_file_paths(vanadium_run_number)
        self.assertEqual((expected_integral, expected_processed), output)
        self.assertEqual(1, makedirs.call_count)

    @patch(dir_path + ".vanadium_corrections.output_settings.get_output_path")
    @patch(dir_path + ".vanadium_corrections.makedirs")
    def test_generate_van_ws_file_paths_with_rb(self, makedirs, out_path):
        usr_path = path.expanduser("~")
        out_path.return_value = path.join(usr_path, "Test_Directory")
        vanadium_run_number = "1234"
        rb_no = "2345"
        engineering_path = path.join(usr_path, "Test_Directory")
        if path.exists(engineering_path):
            rmtree(engineering_path)
        expected_integral = (path.join(usr_path, "Test_Directory", "User", "2345", "Vanadium_Runs",
                             "1234_precalculated_vanadium_run_integration.nxs"))
        expected_processed = (path.join(usr_path, "Test_Directory", "User", "2345", "Vanadium_Runs",
                              "1234_precalculated_vanadium_run_processed_instrument.nxs"))
        output = vanadium_corrections.generate_van_ws_file_paths(vanadium_run_number, rb_num=rb_no)
        self.assertEqual((expected_integral, expected_processed), output)
        self.assertEqual(1, makedirs.call_count)


if __name__ == '__main__':
    unittest.main()

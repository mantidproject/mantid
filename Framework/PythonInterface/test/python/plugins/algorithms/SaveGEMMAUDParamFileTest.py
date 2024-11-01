# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from mantid.kernel import config
import mantid.simpleapi as mantid
from testhelpers import run_algorithm


class SaveGEMMAUDParamFileTest(unittest.TestCase):

    ALG_NAME = "SaveGEMMAUDParamFile"
    GSAS_PARAM_FILE = "GEM_PF1_PROFILE.IPF"
    INPUT_FILE_NAME = "GEM61785_texture_banks_1_to_4.nxs"
    INPUT_WS_NAME = ALG_NAME + "_input_ws"
    OUTPUT_FILE_NAME = os.path.join(config["defaultsave.directory"], "SaveGEMMAUDParamFileTest_outputFile.maud")

    file_contents = None

    def setUp(self):
        mantid.Load(Filename=self.INPUT_FILE_NAME, OutputWorkspace=self.INPUT_WS_NAME)
        run_algorithm(
            self.ALG_NAME,
            InputWorkspace=self.INPUT_WS_NAME,
            GSASParamFile=self.GSAS_PARAM_FILE,
            OutputFilename=self.OUTPUT_FILE_NAME,
            GroupingScheme=[1, 1, 2, 3],
        )
        with open(self.OUTPUT_FILE_NAME) as output_file:
            self.file_contents = output_file.read().split("\n")

        mantid.mtd.remove(self.INPUT_WS_NAME)
        os.remove(self.OUTPUT_FILE_NAME)

    def _test_file_segment_matches(self, segment_header, expected_values, val_type=float):
        line_index = self.file_contents.index(segment_header) + 1
        expected_vals_index = 0
        while self.file_contents[line_index]:
            self.assertAlmostEqual(val_type(self.file_contents[line_index]), expected_values[expected_vals_index])
            line_index += 1
            expected_vals_index += 1

    def _test_all_values_in_segment_equal(self, segment_header, val_to_match):
        self._test_file_segment_matches(segment_header, [val_to_match] * 4)

    def _test_all_zeros(self, segment_header):
        self._test_all_values_in_segment_equal(segment_header, 0)

    def test_values_saved_correctly(self):
        # Bank IDs, generated from the number of spectra
        self._test_file_segment_matches("_instrument_counter_bank_ID", ["Bank{}".format(i + 1) for i in range(5)], val_type=str)

        # Conversion factors, read from GSAS param file
        self._test_file_segment_matches("_instrument_bank_difc", [746.96, 746.96, 1468.19, 2788.34])
        self._test_file_segment_matches("_instrument_bank_difa", [-0.24, -0.24, 4.82, 10.26])
        self._test_file_segment_matches("_instrument_bank_zero", [-9.78, -9.78, 8.95, 16.12])

        # Scattering angles, read from detector 0 if each spectrum
        self._test_file_segment_matches("_instrument_bank_tof_theta", [9.1216, 8.15584, 8.03516799206, 9.06114184264])
        self._test_file_segment_matches("_instrument_bank_eta", [0, 30, 150, 180])

        # Distance, read from GSAS param file
        self._test_file_segment_matches("_pd_instr_dist_spec/detc", [2.3696, 2.3696, 1.7714, 1.445])

        # Function type, always 1
        self._test_all_values_in_segment_equal("_riet_par_TOF_function_type", 1)

        # Profile coefficients for function 1, read from GSAS param file
        self._test_all_zeros("_riet_par_TOF_func1_alpha0")
        self._test_all_values_in_segment_equal("_riet_par_TOF_func1_alpha1", 0.16359)
        self._test_all_values_in_segment_equal("_riet_par_TOF_func1_beta0", 0.0265)
        self._test_all_values_in_segment_equal("_riet_par_TOF_func1_beta1", 0.02108)

        self._test_all_zeros("_riet_par_TOF_func1_sigma0")
        self._test_file_segment_matches("_riet_par_TOF_func1_sigma1", [90.0816, 90.0816, 151.242, 278.117])
        self._test_file_segment_matches("_riet_par_TOF_func1_sigma2", [0, 0, 10.32, 13.63])

        # Profile coefficients for function 2, read from GSAS param file (but always 0)
        prof_coeffs_func_2 = {"alpha0", "alpha1", "beta", "switch", "sigma0", "sigma1", "sigma2", "gamma0", "gamma1", "gamma2"}
        for param in prof_coeffs_func_2:
            self._test_all_zeros("_riet_par_TOF_func2_" + param)


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins.input
import abins.test_helpers


class AbinsLoadJSONTest(unittest.TestCase, abins.input.Tester):
    def setUp(self):
        # A small cutoff is used to limit number of data files
        self.default_cutoff = abins.parameters.sampling["force_constants"]["qpt_cutoff"]
        abins.parameters.sampling["force_constants"]["qpt_cutoff"] = 4.0

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["_LoadJSON"])
        abins.parameters.sampling["force_constants"]["qpt_cutoff"] = self.default_cutoff

    def test_json_1(self):
        """Load from Euphonic QpointPhononModes dump"""
        self.check(name="NH3_euphonic_modes_LoadJSON", loader=abins.input.JSONLoader)

    def test_json_2(self):
        """Load from Euphonic ForceConstants dump"""
        self.check(name="NH3_euphonic_fc_LoadJSON", loader=abins.input.JSONLoader, max_displacement_kpt=-1)

    def test_json_3(self):
        """Load from AbinsData dump"""
        self.check(name="NH3_abinsdata_LoadJSON", loader=abins.input.JSONLoader)


if __name__ == "__main__":
    unittest.main()

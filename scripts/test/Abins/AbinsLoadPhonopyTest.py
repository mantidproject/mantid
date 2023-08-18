# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins.input
import abins.test_helpers
from abins.input import EuphonicLoader


class AbinsLoadPhonopyTest(unittest.TestCase, abins.input.Tester):
    def setUp(self):
        # A small cutoff is used to limit number of data files
        self.default_cutoff = abins.parameters.sampling["force_constants"]["qpt_cutoff"]
        abins.parameters.sampling["force_constants"]["qpt_cutoff"] = 4.0

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["_LoadPhonopy"])
        abins.parameters.sampling["force_constants"]["qpt_cutoff"] = self.default_cutoff

    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_phonopy_reader = EuphonicLoader(input_ab_initio_filename="NonExistingFile.yaml")
            bad_phonopy_reader.read_vibrational_or_phonon_data()

        with self.assertRaises(TypeError):
            _ = EuphonicLoader(input_ab_initio_filename=1)


if __name__ == "__main__":
    unittest.main()

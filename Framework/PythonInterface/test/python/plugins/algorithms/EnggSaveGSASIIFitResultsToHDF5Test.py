# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import h5py
import os
import random
import tempfile
import unittest

import mantid.simpleapi as mantid
from testhelpers import run_algorithm


class EnggSaveGSASIIFitResultsToHDF5Test(unittest.TestCase):
    ALG_NAME = "EnggSaveGSASIIFitResultsToHDF5"
    FIT_RESULTS_TABLE_NAME = "FitResults"
    LATTICE_PARAMS_TABLE_NAME = "LatticeParams"
    LATTICE_PARAMS = ["a", "b", "c", "alpha", "beta", "gamma", "volume"]
    TEMP_FILE_NAME = os.path.join(tempfile.gettempdir(), "EnggSaveGSASIIFitResultsToHDF5Test.hdf5")

    def setUp(self):
        self.defaultAlgParams = {
            "LatticeParamWorkspaces": [self._create_lattice_params_table()],
            "BankIDs": [1],
            "Filename": self.TEMP_FILE_NAME,
            "Rwp": [75],
        }

    def tearDown(self):
        try:
            os.remove(self.TEMP_FILE_NAME)
        except OSError:
            pass

    def test_saveLatticeParams(self):
        lattice_params = self._create_lattice_params_table()
        test_alg = run_algorithm(
            self.ALG_NAME, LatticeParamWorkspaces=[lattice_params], BankIDs=[1], Filename=self.TEMP_FILE_NAME, Rwp=[12]
        )
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            bank_group = output_file["Bank 1"]
            self.assertTrue("GSAS-II Fitting" in bank_group)
            fit_results_group = bank_group["GSAS-II Fitting"]

            self.assertTrue("Lattice Parameters" in fit_results_group)
            lattice_params_dataset = fit_results_group["Lattice Parameters"]
            lattice_params_row = lattice_params.row(0)

            for param in self.LATTICE_PARAMS:
                self.assertAlmostEqual(lattice_params_row[param], lattice_params_dataset[param][()])

    def test_saveRefinementParameters(self):
        refinement_method = b"Rietveld refinement"
        x_min = 10000
        x_max = 40000
        lattice_params = self._create_lattice_params_table()
        test_alg = run_algorithm(
            self.ALG_NAME,
            LatticeParamWorkspaces=[lattice_params],
            BankIDs=[2],
            Filename=self.TEMP_FILE_NAME,
            Rwp=[12],
            RefinementMethod=refinement_method,
            RefineSigma=True,
            Sigma=[1],
            RefineGamma=False,
            XMin=x_min,
            XMax=x_max,
        )
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 2"]["GSAS-II Fitting"]
            self.assertTrue("Refinement Parameters" in fit_results_group)
            refinement_params = fit_results_group["Refinement Parameters"]

            self.assertEqual(len(refinement_params), 5)

            # refinement_params is a tuple, so test that parameters are at the correct index
            self.assertEqual(refinement_params["RefinementMethod"][()], refinement_method)
            self.assertTrue(refinement_params["RefineSigma"][()])
            self.assertFalse(refinement_params["RefineGamma"][()])
            self.assertEqual(refinement_params["XMin"][()], x_min)
            self.assertEqual(refinement_params["XMax"][()], x_max)

    def test_saveProfileCoefficients(self):
        sigma = 13
        gamma = 14

        test_alg = run_algorithm(self.ALG_NAME, RefineSigma=True, Sigma=sigma, RefineGamma=True, Gamma=gamma, **self.defaultAlgParams)
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertTrue("Profile Coefficients" in fit_results_group)
            profile_coeffs = fit_results_group["Profile Coefficients"]

            self.assertEqual(len(profile_coeffs), 2)
            self.assertEqual(profile_coeffs["Sigma"][()], sigma)
            self.assertEqual(profile_coeffs["Gamma"][()], gamma)

    def test_profileCoeffsNotSavedWhenNotRefined(self):
        run_algorithm(self.ALG_NAME, **self.defaultAlgParams)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertFalse("Profile Coefficients" in fit_results_group)

    def test_pawleyRefinementSavesPawleyParams(self):
        d_min = 1.0
        negative_weight = 1.5

        run_algorithm(self.ALG_NAME, PawleyDMin=d_min, PawleyNegativeWeight=negative_weight, **self.defaultAlgParams)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            refinement_params = fit_results_group["Refinement Parameters"]
            self.assertEqual(len(refinement_params), 7)
            self.assertEqual(refinement_params["PawleyDMin"][()], d_min)
            self.assertEqual(refinement_params["PawleyNegativeWeight"][()], negative_weight)

    def test_saveRwp(self):
        rwp = 75

        run_algorithm(self.ALG_NAME, **self.defaultAlgParams)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertTrue("Rwp" in fit_results_group)
            rwp_from_file = fit_results_group["Rwp"]
            self.assertEqual(rwp_from_file[()], rwp)

    def test_saveToExistingFileDoesNotOverwrite(self):
        run_algorithm(
            self.ALG_NAME,
            LatticeParamWorkspaces=[self._create_lattice_params_table("tbl1")],
            BankIDs=[1],
            Filename=self.TEMP_FILE_NAME,
            Rwp=[1],
        )
        run_algorithm(
            self.ALG_NAME,
            LatticeParamWorkspaces=[self._create_lattice_params_table("tbl2")],
            BankIDs=[2],
            Filename=self.TEMP_FILE_NAME,
            Rwp=[2],
        )

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            self.assertTrue("Bank 2" in output_file)

    def test_saveMultipleWorkspacesIsIndexedCorrectly(self):
        alg = run_algorithm(
            self.ALG_NAME,
            LatticeParamWorkspaces=[self._create_lattice_params_table("tbl1"), self._create_lattice_params_table("tbl2")],
            RunNumbers=[123, 456],
            BankIDs=[1, 2],
            Filename=self.TEMP_FILE_NAME,
            Rwp=[1, 2],
        )
        self.assertTrue(alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Run 123" in output_file)
            run_123_group = output_file["Run 123"]
            self.assertTrue("Bank 1" in run_123_group)
            bank_1_group = run_123_group["Bank 1"]
            self.assertTrue("GSAS-II Fitting" in bank_1_group)

            self.assertTrue("Run 456" in output_file)
            run_456_group = output_file["Run 456"]
            self.assertTrue("Bank 2" in run_456_group)
            bank_2_group = run_456_group["Bank 2"]
            self.assertTrue("GSAS-II Fitting" in bank_2_group)

    def _create_lattice_params_table(self, output_name=LATTICE_PARAMS_TABLE_NAME):
        lattice_params = mantid.CreateEmptyTableWorkspace(OutputWorkspace=output_name)
        [lattice_params.addColumn("double", param) for param in self.LATTICE_PARAMS]
        lattice_params.addRow([random.random() for _ in self.LATTICE_PARAMS])
        return lattice_params


if __name__ == "__main__":
    unittest.main()

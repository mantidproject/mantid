from __future__ import (absolute_import, division, print_function)

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
    TEMP_FILE_NAME = os.path.join(tempfile.gettempdir(),
                                  "EnggSaveGSASIIFitResultsToHDF5Test.hdf5")

    def tearDown(self):
        try:
            os.remove(self.TEMP_FILE_NAME)

        except OSError:
            pass

        self._remove_ws_if_exists(self.FIT_RESULTS_TABLE_NAME)
        self._remove_ws_if_exists(self.LATTICE_PARAMS_TABLE_NAME)

    def test_saveLatticeParams(self):
        lattice_params = self._create_lattice_params_table()

        test_alg = run_algorithm(self.ALG_NAME,
                                 LatticeParams=lattice_params,
                                 BankID=1,
                                 Filename=self.TEMP_FILE_NAME)
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            bank_group = output_file["Bank 1"]
            self.assertTrue("GSAS-II Fitting" in bank_group)
            fit_results_group = bank_group["GSAS-II Fitting"]

            self.assertTrue("Lattice Parameters" in fit_results_group)
            lattice_params_dataset = fit_results_group["Lattice Parameters"]
            lattice_params_row = [lattice_params.row(0)[param] for param in self.LATTICE_PARAMS]

            for val_from_file, val_from_input in zip(lattice_params_row, lattice_params_dataset[0]):
                self.assertAlmostEqual(val_from_file, val_from_input)

    def test_saveRefinementParameters(self):
        lattice_params = self._create_lattice_params_table()

        refinement_method = "Rietveld refinement"
        refine_sigma = True
        refine_gamma = False
        x_min = 10000
        x_max = 40000

        test_alg = run_algorithm(self.ALG_NAME,
                                 LatticeParams=lattice_params,
                                 RefinementMethod=refinement_method,
                                 XMin=x_min,
                                 XMax=x_max,
                                 RefineSigma=refine_sigma,
                                 RefineGamma=refine_gamma,
                                 Filename=self.TEMP_FILE_NAME)
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertTrue("Refinement Parameters" in fit_results_group)
            refinement_params = fit_results_group["Refinement Parameters"][0]

            self.assertEquals(len(refinement_params), 5)

            # refinement_params is a tuple, so test that parameters are at the correct index
            self.assertEquals(refinement_params[0], refinement_method)
            self.assertTrue(refinement_params[1])  # RefineSigma
            self.assertFalse(refinement_params[2])  # RefineGamma
            self.assertEquals(refinement_params[3], x_min)
            self.assertEquals(refinement_params[4], x_max)

    def test_saveProfileCoefficients(self):
        lattice_params = self._create_lattice_params_table()
        sigma = 13
        gamma = 14

        test_alg = run_algorithm(self.ALG_NAME,
                                 LatticeParams=lattice_params,
                                 RefineSigma=True,
                                 RefineGamma=True,
                                 Sigma=sigma,
                                 Gamma=gamma,
                                 Filename=self.TEMP_FILE_NAME)
        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertTrue("Profile Coefficients" in fit_results_group)
            profile_coeffs = fit_results_group["Profile Coefficients"][0]

            self.assertEquals(len(profile_coeffs), 2)
            self.assertEquals(profile_coeffs[0], sigma)
            self.assertEquals(profile_coeffs[1], gamma)

    def test_profileCoeffsNotSavedWhenNotRefined(self):
        lattice_params = self._create_lattice_params_table()
        run_algorithm(self.ALG_NAME,
                      LatticeParams=lattice_params,
                      RefineSigma=False,
                      RefineGamma=False,
                      Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertFalse("Profile Coefficients" in fit_results_group)

    def test_pawleyRefinementSavesPawleyParams(self):
        lattice_params = self._create_lattice_params_table()
        d_min = 1.0
        negative_weight = 1.5

        run_algorithm(self.ALG_NAME,
                      BankID=2,
                      LatticeParams=lattice_params,
                      RefinementMethod="Pawley refinement",
                      PawleyDMin=d_min,
                      PawleyNegativeWeight=negative_weight,
                      Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 2"]["GSAS-II Fitting"]
            refinement_params = fit_results_group["Refinement Parameters"][0]
            self.assertEquals(len(refinement_params), 7)
            self.assertEquals(refinement_params[5], d_min)
            self.assertEquals(refinement_params[6], negative_weight)

    def test_saveRwp(self):
        lattice_params = self._create_lattice_params_table()
        rwp = 75

        run_algorithm(self.ALG_NAME,
                      LatticeParams=lattice_params,
                      Rwp=rwp,
                      Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            fit_results_group = output_file["Bank 1"]["GSAS-II Fitting"]
            self.assertTrue("Rwp" in fit_results_group)
            rwp_from_file = fit_results_group["Rwp"]
            self.assertEquals(rwp_from_file.value, rwp)

    def test_saveToExistingFileDoesNotOverwrite(self):
        lattice_params = self._create_lattice_params_table()
        run_algorithm(self.ALG_NAME,
                      LatticeParams=lattice_params,
                      BankID=1,
                      Filename=self.TEMP_FILE_NAME)
        run_algorithm(self.ALG_NAME,
                      LatticeParams=lattice_params,
                      BankID=2,
                      Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            self.assertTrue("Bank 2" in output_file)

    def _create_lattice_params_table(self):
        lattice_params = mantid.CreateEmptyTableWorkspace(OutputWorkspace=self.LATTICE_PARAMS_TABLE_NAME)
        [lattice_params.addColumn("double", param) for param in self.LATTICE_PARAMS]
        lattice_params.addRow([random.random() for _ in self.LATTICE_PARAMS])
        return lattice_params

    def _remove_ws_if_exists(self, name):
        if mantid.mtd.doesExist(name):
            mantid.mtd.remove(name)

if __name__ == "__main__":
    unittest.main()
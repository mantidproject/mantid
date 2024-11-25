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


class EnggSaveSinglePeakFitResultsToHDF5Test(unittest.TestCase):
    ALG_NAME = "EnggSaveSinglePeakFitResultsToHDF5"
    FIT_PARAMS = ["dSpacing", "A0", "A0_Err", "A1", "A1_Err", "X0", "X0_Err", "A", "A_Err", "B", "B_Err", "S", "S_Err", "I", "I_Err", "Chi"]
    FIT_RESULTS_TABLE_NAME = "FitResults"
    TEMP_FILE_NAME = os.path.join(tempfile.gettempdir(), "EnggSaveSinglePeakFitResultsToHDF5Test.hdf5")

    def tearDown(self):
        try:
            os.remove(self.TEMP_FILE_NAME)

        except OSError:
            pass

        if mantid.mtd.doesExist(self.FIT_RESULTS_TABLE_NAME):
            mantid.mtd.remove(self.FIT_RESULTS_TABLE_NAME)

    def test_savePeaks(self):
        peaks = []
        for i in range(2):
            peaks.append(self._create_random_peak_row())

        input_ws = self._create_fit_results_table(peaks)
        test_alg = run_algorithm(self.ALG_NAME, InputWorkspaces=[input_ws], BankIDs=[1], Filename=self.TEMP_FILE_NAME)

        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            bank_group = output_file["Bank 1"]
            self.assertTrue("Single Peak Fitting" in bank_group)
            peaks_dataset = bank_group["Single Peak Fitting"]

            for i, row in enumerate(peaks_dataset):
                for j, val in enumerate(row):
                    self.assertAlmostEqual(peaks[i][j], val)

    def test_saveToExistingFileDoesNotOverwrite(self):
        peaks1 = self._create_random_peak_row()
        table_ws1 = self._create_fit_results_table([peaks1])
        run_algorithm(self.ALG_NAME, InputWorkspaces=[table_ws1], BankIDs=[1], Filename=self.TEMP_FILE_NAME)

        peaks2 = self._create_random_peak_row()
        table_ws2 = self._create_fit_results_table([peaks2])
        run_algorithm(self.ALG_NAME, InputWorkspaces=[table_ws2], BankIDs=[2], Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Bank 1" in output_file)
            self.assertTrue("Bank 2" in output_file)

    def test_saveMultipleWorkspacesIsIndexedCorrectly(self):
        table_ws1 = self._create_fit_results_table([self._create_random_peak_row()])
        table_ws1 = mantid.RenameWorkspace(InputWorkspace=table_ws1, OutputWorkspace="ws1")
        table_ws2 = self._create_fit_results_table([self._create_random_peak_row()])
        table_ws2 = mantid.RenameWorkspace(InputWorkspace=table_ws2, OutputWorkspace="ws2")
        run_algorithm(
            self.ALG_NAME, InputWorkspaces=[table_ws1, table_ws2], BankIDs=[1, 2], RunNumbers=[123, 456], Filename=self.TEMP_FILE_NAME
        )

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Run 123" in output_file)
            run_123_group = output_file["Run 123"]
            self.assertTrue("Bank 1" in run_123_group)
            bank_1_group = run_123_group["Bank 1"]
            self.assertTrue("Single Peak Fitting" in bank_1_group)

            self.assertTrue("Run 456" in output_file)
            run_456_group = output_file["Run 456"]
            self.assertTrue("Bank 2" in run_456_group)
            bank_2_group = run_456_group["Bank 2"]
            self.assertTrue("Single Peak Fitting" in bank_2_group)

    def _create_fit_results_table(self, rows):
        table = mantid.CreateEmptyTableWorkspace(OutputWorkspace=self.FIT_RESULTS_TABLE_NAME)
        for col in self.FIT_PARAMS:
            table.addColumn("double", col)

        for row in rows:
            table.addRow(row)

        return table

    def _create_random_peak_row(self):
        return [random.random() for _ in self.FIT_PARAMS]


if __name__ == "__main__":
    unittest.main()

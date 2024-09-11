# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import *
from mantid.kernel import *
import h5py


class EnggSaveSinglePeakFitResultsToHDF5(PythonAlgorithm):
    PROP_BANKIDS = "BankIDs"
    PROP_FILENAME = "Filename"
    PROP_INPUT_WS = "InputWorkspaces"
    PROP_RUN_NUMBERS = "RunNumbers"

    BANK_GROUP_NAME = "Bank {}".format
    RUN_GROUP_NAME = "Run {}".format
    PEAKS_DATASET_NAME = "Single Peak Fitting"
    FIT_PARAMS = ["dSpacing", "A0", "A0_Err", "A1", "A1_Err", "X0", "X0_Err", "A", "A_Err", "B", "B_Err", "S", "S_Err", "I", "I_Err", "Chi"]

    def category(self):
        return "DataHandling"

    def name(self):
        return "EnggSaveSinglePeakFitResultsToHDF5"

    def summary(self):
        return "Save a table workspace containing fit parameters from EnggFitPeaks to an HDF5 file, indexed by bank ID"

    def validateInputs(self):
        issues = {}

        input_ws_names = self.getProperty(self.PROP_INPUT_WS).value
        for ws_name in input_ws_names:
            if ws_name not in mtd:
                issues[self.PROP_INPUT_WS] = "The workspace {} was not found in the ADS".format(ws_name)

        bankIDs = self.getProperty(self.PROP_BANKIDS).value
        if len(bankIDs) != len(input_ws_names):
            issues[self.PROP_BANKIDS] = "One bank ID must be supplied for every input workspace"

        runNumbers = self.getProperty(self.PROP_RUN_NUMBERS).value
        if len(input_ws_names) > 1 and len(runNumbers) != len(input_ws_names):
            issues[self.PROP_RUN_NUMBERS] = (
                "When saving multiple fit results at once, one run number must be " "supplied for every input workspace"
            )

        return issues

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name=self.PROP_INPUT_WS), doc="Table workspaces containing fit parameters to save")

        self.declareProperty(IntArrayProperty(name=self.PROP_BANKIDS), doc="The bank ID of each input workspace, in order")

        self.declareProperty(IntArrayProperty(name=self.PROP_RUN_NUMBERS), doc="The run number of each input workspace, in order")

        self.declareProperty(
            FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save, extensions=[".hdf5", ".h5", ".hdf"]),
            doc="HDF5 file to save to",
        )

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value
        input_ws_names = self.getProperty(self.PROP_INPUT_WS).value
        bankIDs = self.getProperty(self.PROP_BANKIDS).value
        run_numbers = self.getProperty(self.PROP_RUN_NUMBERS).value

        with h5py.File(output_file_name, "a") as output_file:
            for i, ws_name in enumerate(input_ws_names):
                input_ws = mtd[ws_name]

                if len(input_ws_names) > 1:
                    top_level_group = output_file.require_group(self.RUN_GROUP_NAME(run_numbers[i]))
                else:
                    top_level_group = output_file

                bank_group = top_level_group.require_group(self.BANK_GROUP_NAME(bankIDs[i]))

                if self.PEAKS_DATASET_NAME in bank_group:
                    del bank_group[self.PEAKS_DATASET_NAME]

                peaks_dataset = bank_group.create_dataset(
                    name=self.PEAKS_DATASET_NAME,
                    shape=(input_ws.rowCount(),),
                    dtype=[(column_name, "f") for column_name in self.FIT_PARAMS],
                )

                for i, row in enumerate(input_ws):
                    peaks_dataset[i] = tuple(row[column_name] for column_name in self.FIT_PARAMS)


AlgorithmFactory.subscribe(EnggSaveSinglePeakFitResultsToHDF5)

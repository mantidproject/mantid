from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
import h5py


class EnggSaveSinglePeakFitResultsToHDF5(PythonAlgorithm):

    PROP_BANKID = "BankID"
    PROP_FILENAME = "Filename"
    PROP_INPUT_WS = "InputWorkspace"

    BANK_GROUP_NAME = "Bank {}".format
    PEAKS_DATASET_NAME = "Single Peak Fitting"
    FIT_PARAMS = ["dSpacing", "A0", "A0_Err", "A1", "A1_Err", "X0", "X0_Err",
                  "A", "A_Err", "B", "B_Err", "S", "S_Err", "I", "I_Err", "Chi"]

    def category(self):
        return "DataHandling"

    def name(self):
        return "EnggSaveSinglePeakFitResultsToHDF5"

    def summary(self):
        return "Save a table workspace containing fit parameters from EnggFitPeaks to an HDF5 file, indexed by bank ID"

    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty(name=self.PROP_INPUT_WS, defaultValue="",
                                                     direction=Direction.Input),
                             doc="Table workspace containing fit parameters to save")

        self.declareProperty(name=self.PROP_BANKID, validator=IntMandatoryValidator(), direction=Direction.Input,
                             doc="ID of the bank associated with this data (1 for North, 2 for South)", defaultValue=1)

        self.declareProperty(FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save,
                                          extensions=[".hdf5", ".h5", ".hdf"]), doc="HDF5 file to save to")

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value

        with h5py.File(output_file_name, "a") as output_file:
            bankID = self.getProperty(self.PROP_BANKID).value

            bank_group = output_file.require_group(self.BANK_GROUP_NAME(bankID))
            if self.PEAKS_DATASET_NAME in bank_group:
                del bank_group[self.PEAKS_DATASET_NAME]

            input_ws = self.getProperty(self.PROP_INPUT_WS).value
            peaks_dataset = bank_group.create_dataset(name=self.PEAKS_DATASET_NAME, shape=(input_ws.rowCount(),),
                                                      dtype=[(column_name, "f") for column_name in self.FIT_PARAMS])

            for i, row in enumerate(input_ws):
                peaks_dataset[i] = tuple(row[column_name] for column_name in self.FIT_PARAMS)


AlgorithmFactory.subscribe(EnggSaveSinglePeakFitResultsToHDF5)

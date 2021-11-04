# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.simpleapi import LoadNexusProcessed, ReflectometryISISSumBanks
from mantid.api import AnalysisDataService


class ReflectometryISISSumBanksTest(systemtesting.MantidSystemTest):
    TEST_FILENAME = "INTER45455.nxs"
    REF_FILENAME = "INTER45455_SummedX.nxs"
    OUT_WS_NAME = "ISISSumBanks"

    def cleanUp(self):
        AnalysisDataService.Instance().remove(self.OUT_WS_NAME)
        AnalysisDataService.Instance().remove(self.REF_FILENAME)

    def requiredFiles(self):
        return [self.TEST_FILENAME]

    def runTest(self):
        raw_ws = LoadNexusProcessed(self.TEST_FILENAME)

        def get_first_detid_in_bank(bank_pos):
            starting_pixel_id = 2001
            bank_width = 60
            return starting_pixel_id + (bank_width * bank_pos)

        # Banks 1 + 2
        roi_str = f"{get_first_detid_in_bank(1)}-{get_first_detid_in_bank(3) - 1}"
        ReflectometryISISSumBanks(InputWorkspace=raw_ws, ROIDetectorIDs=roi_str,
                                  OutputWorkspace=self.OUT_WS_NAME)

    def validate(self):
        return self.OUT_WS_NAME, self.REF_FILENAME


class ReflectometryISISSumAllBanksTest(systemtesting.MantidSystemTest):
    TEST_FILENAME = "INTER45455.nxs"
    REF_FILENAME = "INTER45455_SummedX_All.nxs"
    OUT_WS_NAME = "ISISSumBanks"

    def cleanUp(self):
        AnalysisDataService.Instance().remove(self.OUT_WS_NAME)
        AnalysisDataService.Instance().remove(self.REF_FILENAME)

    def requiredFiles(self):
        return [self.TEST_FILENAME]

    def runTest(self):
        raw_ws = LoadNexusProcessed(self.TEST_FILENAME)
        ReflectometryISISSumBanks(InputWorkspace=raw_ws, OutputWorkspace=self.OUT_WS_NAME)

    def validate(self):
        return self.OUT_WS_NAME, self.REF_FILENAME

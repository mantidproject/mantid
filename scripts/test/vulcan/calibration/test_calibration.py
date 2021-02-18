# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np
from numpy.testing import assert_allclose
# from os import path
#
from mantid import AnalysisDataService
from mantid.simpleapi import LoadDiffCal
# from mantid import config
# from mantid.kernel import V3D
# from mantid.simpleapi import (CreateEmptyTableWorkspace, DeleteWorkspaces, GroupWorkspaces, LoadEmptyInstrument,
#                               LoadNexusProcessed, mtd)
#
# from corelli.calibration.utils import (apply_calibration, preprocess_banks, bank_numbers, calculate_peak_y_table,
#                                        calibrate_tube, load_banks, trim_calibration_table, wire_positions)
from mantid.simpleapi import DeleteWorkspaces


class TestUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the tests cases for calibrate_bank, consisting of data for only one bank
        CORELLI_124023_bank10, tube 13 has shadows at pixel numbers quite different from the rest
        """
        cls.workspaces_temporary = list()
        print(f'setUpClass')

    @classmethod
    def tearDownClass(cls) -> None:
        r"""Delete temporary workspaces"""
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def setUp(self) -> None:
        # load the testing data
        test_file = "VULCAN_192226.nxs.h5"

    def tearDown(self) -> None:
        to_delete = [w for w in ['a', 'b'] if AnalysisDataService.doesExist(w)]
        if len(to_delete) > 0:
            DeleteWorkspaces(to_delete)


if __name__ == "__main__":
    unittest.main()

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
from mantid.simpleapi import Load, mtd
# from mantid import config
# from mantid.kernel import V3D
# from mantid.simpleapi import (CreateEmptyTableWorkspace, DeleteWorkspaces, GroupWorkspaces, LoadEmptyInstrument,
#                               LoadNexusProcessed, mtd)
#
# from corelli.calibration.utils import (apply_calibration, preprocess_banks, bank_numbers, calculate_peak_y_table,
#                                        calibrate_tube, load_banks, trim_calibration_table, wire_positions)
from mantid.simpleapi import DeleteWorkspaces


class TestUtils(unittest.TestCase):

    workspaces_temporary = list()

    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the testing file(s) for Vulcan calibration
        
        NOTE: called once before the test begin
        """
        # load the testing data
        Load(
            Filename="VULCAN_192226.nxs.h5",
            OutputWorkspace="VULCAN_192226",
        )
        cls.workspaces_temporary.append("VULCAN_192226")

    @classmethod
    def tearDownClass(cls) -> None:
        r"""
        Delete temporary workspaces

        NOTE: called once before exiting test
        """
        if len(cls.workspaces_temporary) > 0:
            DeleteWorkspaces(cls.workspaces_temporary)

    def setUp(self) -> None:
        r"""
        Per test based setup
        """
        pass

    def tearDown(self) -> None:
        r"""
        Per test case cleanup
        """
        pass

    # ------------------ #
    # -- System tests -- #
    # ------------------ #
    def test_calibration(self):
        assert ("VULCAN_192226" in mtd)


if __name__ == "__main__":
    unittest.main()

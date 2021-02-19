# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import os
import tempfile
from mantid.simpleapi import mtd
from vulcan.calibration.calibrate_vulcan_x import align_vulcan_data, cross_correlate_calibrate, peak_position_calibrate


class TestUtils(unittest.TestCase):

    workspaces_temporary = list()

    @classmethod
    def setUpClass(cls) -> None:
        r"""
        Load the testing file(s) for Vulcan calibration
        
        NOTE: called once before the test begin
        """
        # load the testing data (diamond run)
        # NOTE: the calibration functions takes filenames
        #       as input, therefore we do not have to
        #       pre-load any workspace into memory
        pass

    @classmethod
    def tearDownClass(cls) -> None:
        r"""
        Delete temporary workspaces

        NOTE: called once before exiting test
        """
        # remove all workspace from memory before exiting
        mtd.clear()

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
        diamond_run = ["VULCAN_192226.nxs.h5"]
        vulcan_x_idf = ""  # TODO: update IDF for vulcanx
        calibration_rst = os.path.join(tempfile.gettempdir(), "VULCAN_Calibration_Hybrid.h5")

        # perform calibration
        cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_run, vulcan_x_idf)

        # verify calibration file is created
        # NOTE: this is step 1, we will add more reliable asseration once
        #       we have a reference calibration results


if __name__ == "__main__":
    unittest.main()

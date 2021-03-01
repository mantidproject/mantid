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
        vulcan_x_idf = "VULCAN_Definition.xml"  # Should be in Mantid instrument folder
        calibration_rst = os.path.join(tempfile.gettempdir(), "VULCAN_Calibration_Hybrid.h5")
        tube_grouping_plan = None
        # FIXME - output directory shall be set by tempfile
        test_output_dir = '/tmp'

        # perform calibration
        # step_1:
        cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_run, cross_correlate_param_dict=None,
                                                                   output_dir=test_output_dir)

        # step_2:
        cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(
            diamond_runs=diamond_run,
            diff_cal_file_name=cc_calib_file,
            output_dir=test_output_dir,
            tube_grouping_plan=tube_grouping_plan)

        # step_3:
        tube_grouping_plan = [(0, None, 81920), (81920, None, 81920 * 2), (81920 * 2, None, 200704)]
        peak_position_calibrate(cc_focus_ws_name, tube_grouping_plan, cc_calib_file, calibration_rst,
                                test_output_dir)

        # verify calibration file is created
        # NOTE: this is step 1, we will add more reliable asseration once
        #       we have a reference calibration results
        assert os.path.exists(calibration_rst)


if __name__ == "__main__":
    unittest.main()

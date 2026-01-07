# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import StitchByBackground, CreateSampleWorkspace, GroupWorkspaces, AnalysisDataService


class StitchByBackgroundTest(unittest.TestCase):
    ws_list = []
    stitch_points = []

    @classmethod
    def setUpClass(cls):
        for i in range(5):
            CreateSampleWorkspace(
                OutputWorkspace=f"ws_{i + 1}",
                NumBanks=1,
                BankPixelWidth=1,
                Function="Multiple Peaks",
                XMin=2e4 * i - 1e3,
                XMax=2e4 * (i + 1),
            )
            cls.ws_list.append(f"ws_{i + 1}")
            cls.stitch_points.append(20000 * (i + 1))
        cls.stitch_points.pop(-1)  # Remove the last stitch point.
        GroupWorkspaces(InputWorkspaces=cls.ws_list, OutputWorkspace="ws_group")

    def teardown(self):
        AnalysisDataService.clear()

    def test_overlap_width_must_be_positive(self):
        with self.assertRaisesRegex(ValueError, "-1 is < the lower bound"):
            StitchByBackground(InputWorkspaces="ws_group", OverlapWidth=-1, CropUpperBound=1000)

    def test_stitch_and_ws_list_correctly_sized(self):
        with self.assertRaisesRegex(RuntimeError, r"There must be one less stitch point \(3\) than input workspaces \(5\)."):
            StitchByBackground(
                InputWorkspaces=self.ws_list, OverlapWidth=1, StitchPoints=[1.2, 2.3, 3.4], OutputWorkspace="out", CropUpperBound=1000
            )

    def test_crop_bounds_are_tested(self):
        with self.assertRaisesRegex(RuntimeError, r"Upper bound \(0.0\) must be greater than lower bound \(0.0\)"):
            StitchByBackground(InputWorkspaces=self.ws_list, OverlapWidth=1, StitchPoints=self.stitch_points, OutputWorkspace="out")

        with self.assertRaisesRegex(RuntimeError, r"Upper bound \(0.0\) must be greater than lower bound \(1000.0\)"):
            StitchByBackground(
                InputWorkspaces=self.ws_list, OverlapWidth=2e3, StitchPoints=self.stitch_points, OutputWorkspace="out", CropLowerBound=1000
            )

    def test_data_limits_are_checked_for_crop_bounds(self):
        with self.assertRaisesRegex(RuntimeError, r"-1001.0 is outside the lower limit of the data \(-1000.0\)"):
            StitchByBackground(
                InputWorkspaces=self.ws_list, OverlapWidth=2e3, StitchPoints=self.stitch_points, OutputWorkspace="out", CropLowerBound=-1001
            )

        with self.assertRaisesRegex(RuntimeError, r"100001.0 is outside the upper limit of the data \(100000.0\)."):
            StitchByBackground(
                InputWorkspaces=self.ws_list,
                OverlapWidth=2e3,
                StitchPoints=self.stitch_points,
                OutputWorkspace="out",
                CropUpperBound=100001,
            )

    def test_all_stitch_points_are_checked_to_be_within_overlap(self):
        with self.assertRaisesRegex(RuntimeError, r"Invalid points: 30000.0 is not between 39000.0 and 40000.0."):
            StitchByBackground(
                InputWorkspaces=self.ws_list,
                OverlapWidth=2000,
                StitchPoints=[2e4, 3e4, 6e4, 8e4],
                OutputWorkspace="out",
                CropUpperBound=100000,
            )

    def test_stitching_occurs_correctly(self):
        out_ws = StitchByBackground(
            InputWorkspaces=self.ws_list,
            StitchPoints=self.stitch_points,
            OutputWorkspace="out",
            OverlapWidth=2000,
            CropUpperBound=95000,
            CropLowerBound=0,
        )

        self.assertEqual(out_ws.getNumberHistograms(), 1)
        self.assertEqual(out_ws.blocksize(), 475)  # Cropping upper bound to 95000 drops the number of bins from 500 to 475.
        self.assertEqual(out_ws.dataY(0)[10], 0.3)
        self.assertEqual(out_ws.dataY(0)[25], 10.3)


if __name__ == "__main__":
    unittest.main()

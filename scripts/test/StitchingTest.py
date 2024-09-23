# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.kernel import PropertyManagerDataService
from mantid.simpleapi import CreateWorkspace
from LargeScaleStructures import data_stitching
import numpy as np


class StitchingTest(unittest.TestCase):
    def setUp(self):
        # Create two workspaces to stitch
        r1 = 9.8 + 0.4 * np.random.rand(250)
        r2 = 59.8 + 0.4 * np.random.rand(250)
        x1 = np.arange(250)
        x2 = x1 + 50.0

        self.ws1 = CreateWorkspace(DataX=x1, DataY=r1, OutputWorkspace="ws1")
        self.ws2 = CreateWorkspace(DataX=x2, DataY=r2, OutputWorkspace="ws2")

    def tearDown(self):
        for prop_man_name in PropertyManagerDataService.getObjectNames():
            PropertyManagerDataService.remove(prop_man_name)

    def test_stitch(self):
        """
        Test the stitching call
        """
        data_stitching.stitch(
            ["ws1", "ws2"],
            [
                70,
            ],
            [100],
            output_workspace="scaled_ws",
        )
        x_out = mtd["scaled_ws"].dataY(0)
        # Stitching will scale ws2 to ws1, so the output workspace should line up
        # with ws1, which should have an average value of about 10.
        # Since both workspaces have very different ranges of values, we use a wide
        # acceptance range to allow for random fluctuations.
        self.assertTrue(np.average(x_out) < 13 and np.average(x_out) > 7)


if __name__ == "__main__":
    unittest.main()

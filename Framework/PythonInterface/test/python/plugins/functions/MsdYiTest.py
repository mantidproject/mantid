# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, create_model, create_test_workspace, create_function_string
from mantid.simpleapi import Fit


class MsdYiTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("MsdYi")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = np.array([[1, 2], [3, 4]])
        expected = np.array([[1.00547492, 1.18215301], [1.97145491, 3.98690068]])
        tolerance = 0.000001
        status, output = check_output("MsdYi", input, expected, tolerance, Height=1.0, Msd=0.05, Sigma=1.0)

        if not status:
            self.fail(
                "Computed output " + str(output) + " from input " + str(input) + " is not equal to the expected output: " + str(expected)
            )

    def test_use_in_fit(self):
        workspace = create_test_workspace(create_model("MsdYi", Height=1.0, Msd=0.05, Sigma=1.0), 1000)
        function_string = create_function_string("MsdYi", Height=1.0, Msd=0.05, Sigma=1.0)
        Fit(Function=function_string, InputWorkspace=workspace, StartX=1.2, EndX=1200)


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, AlgorithmFactory, FunctionFactory, MatrixWorkspaceProperty, Progress, PythonAlgorithm, WorkspaceFactory
from mantid.kernel import Direction, FloatBoundedValidator
from mantid.simpleapi import Fit
import testhelpers


class _InternalMakeLinear(PythonAlgorithm):
    def PyInit(self):
        self.declareProperty("A0", -1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty("A1", -1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    def PyExec(self):
        import random

        nbins = 4000

        wspace = WorkspaceFactory.create("Workspace2D", NVectors=1, XLength=nbins, YLength=nbins)

        a0 = self.getProperty("A0").value
        a1 = self.getProperty("A1").value

        # Noise parameter
        amplitude = 100

        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=nbins)
        for i in range(0, nbins):
            x_value = 5.0 + 5.5 * i
            nominal_y = a0 + a1 * x_value
            # add some noise
            nominal_y += random.random() * amplitude

            wspace.dataX(0)[i] = x_value
            wspace.dataY(0)[i] = nominal_y
            wspace.dataE(0)[i] = 1
            prog_reporter.report("Setting %dth bin in workspace" % (i - 1))

        self.setProperty("OutputWorkspace", wspace)  # Stores the workspace as the given name


class Example1DFunctionTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        try:
            FunctionFactory.createFunction("ExamplePeakFunction")
        except RuntimeError as exc:
            self.fail("Could not create ExamplePeakFunction function: %s" % str(exc))

    def test_fit_succeeds_with_expected_answer(self):
        AlgorithmFactory.subscribe(_InternalMakeLinear)
        testhelpers.run_algorithm("_InternalMakeLinear", A0=1.0, A1=0.75, OutputWorkspace="_test_linear")

        func_string = "name=Example1DFunction,A0=0.0,A1=0.0"
        Fit(Function=func_string, InputWorkspace="_test_linear", StartX=1000, EndX=6000, CreateOutput=1, MaxIterations=2)

        mtd.remove("_test_linear")
        mtd.remove("_test_linear_NormalisedCovarianceMatrix")
        mtd.remove("_test_linear_Parameters")
        mtd.remove("_test_linear_Workspace")


if __name__ == "__main__":
    unittest.main()

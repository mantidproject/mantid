import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Fit
import testhelpers

class _InternalMakeGaussian(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty("Height", -1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty("Centre", -1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty("Sigma", -1.0, validator=FloatBoundedValidator(lower=0))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction = Direction.Output))

    def PyExec(self):
        import math
        import random
        nbins=1000
        wspace = WorkspaceFactory.create("Workspace2D",NVectors=1,XLength=nbins,YLength=nbins)
        height = self.getProperty("Height").value
        centre = self.getProperty("Centre").value
        sigma_sq = math.pow(self.getProperty("Sigma").value,2)
        # Noise parameter
        amplitude = 0.1*height

        prog_reporter = Progress(self,start=0.0,end=1.0,nreports=nbins)
        for i in range(1,nbins):
            x_value = 5.0 + 5.5*i;
            nominal_y = height * math.exp(-0.5*math.pow(x_value - centre, 2.)/sigma_sq)
            # add some noise
            nominal_y += random.random()*amplitude

            wspace.dataX(0)[i] = x_value
            wspace.dataY(0)[i] = nominal_y
            wspace.dataE(0)[i] = 1
            prog_reporter.report("Setting %dth bin in workspace" % (i-1))

        self.setProperty("OutputWorkspace", wspace) # Stores the workspace as the given name

class ExamplePeakFunctionTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        try:
            FunctionFactory.createFunction("ExamplePeakFunction")
        except RuntimeError, exc:
            self.fail("Could not create ExamplePeakFunction function: %s" % str(exc))

    def test_fit_succeeds_with_expected_answer(self):
        AlgorithmFactory.subscribe(_InternalMakeGaussian)
        alg = testhelpers.run_algorithm("_InternalMakeGaussian", Height=300,Centre=2100,Sigma=700,OutputWorkspace='_test_gauss')

        func_string="name=ExamplePeakFunction,NTerms=3,Height=309.92,PeakCentre=2105,Sigma=710.2"
        Fit(Function=func_string,InputWorkspace="_test_gauss",StartX=150,EndX=4310,CreateOutput=1,MaxIterations=2)

        mtd.remove('_test_gauss')
        mtd.remove('_test_gauss_NormalisedCovarianceMatrix')
        mtd.remove('_test_gauss_Parameters')
        mtd.remove('_test_gauss_Workspace')

if __name__ == '__main__':
    unittest.main()

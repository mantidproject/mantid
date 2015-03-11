#pylint: disable=invalid-name,no-init
import stresstesting
from mantid.simpleapi import *

import platform

#------------------------------------------------------------------------------------------------------------------
WS_PREFIX="fit"

def do_fit_no_background(k_is_free):
    """
    Run the Vesuvio fit without background. If k_is_free is False then it is fixed to f0.Width*sqrt(2)/12
    """
    function_str = \
        "composite=ComptonScatteringCountRate,NumDeriv=1,IntensityConstraints=\"Matrix(1|3)0|-1|3\";"\
        "name=GramCharlierComptonProfile,Mass=1.007940,HermiteCoeffs=1 0 1;"\
        "name=GaussianComptonProfile,Mass=27.000000;"\
        "name=GaussianComptonProfile,Mass=91.000000"
    # Run fit
    _do_fit(function_str, k_is_free)

def do_fit_with_quadratic_background():
    """
    Run the Vesuvio fit without background. If k_is_free is False then it is fixed to f0.Width*sqrt(2)/12
    """
    function_str = \
        "composite=ComptonScatteringCountRate,NumDeriv=1,IntensityConstraints=\"Matrix(1|3)0|-1|3\";"\
        "name=GramCharlierComptonProfile,Mass=1.007940,HermiteCoeffs=1 0 1;"\
        "name=GaussianComptonProfile,Mass=27.000000;"\
        "name=GaussianComptonProfile,Mass=91.000000;name=Polynomial,n=2,A0=0,A1=0,A2=0"
    # Run fit
    _do_fit(function_str, k_is_free=False)

def _do_fit(function_str, k_is_free):
    """
    Run the Vesuvio . If k_is_free is False then it is fixed to f0.Width*sqrt(2)/12

    """
    LoadVesuvio(Filename='14188-14190',OutputWorkspace='raw_ws',SpectrumList='135',Mode='SingleDifference',
                InstrumentParFile=r'IP0005.dat')
    CropWorkspace(InputWorkspace='raw_ws',OutputWorkspace='raw_ws',XMin=50,XMax=562)
    # Convert to seconds
    ScaleX(InputWorkspace='raw_ws',OutputWorkspace='raw_ws',Operation='Multiply',Factor=1e-06)

    if k_is_free:
        ties_str = "f1.Width=10.000000,f2.Width=25.000000"
    else:
        ties_str = "f1.Width=10.000000,f2.Width=25.000000,f0.FSECoeff=f0.Width*sqrt(2)/12"

    constraints_str = "2.000000 < f0.Width < 7.000000"

    Fit(InputWorkspace='raw_ws',Function=function_str,Ties=ties_str,Constraints=constraints_str,
        Output=WS_PREFIX, CreateOutput=True,OutputCompositeMembers=True,MaxIterations=5000,
        Minimizer="Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08")
    # Convert to microseconds
    ScaleX(InputWorkspace=WS_PREFIX + '_Workspace',OutputWorkspace=WS_PREFIX + '_Workspace',Operation='Multiply',Factor=1e06)

def tolerance():
    # Not too happy about this but the gsl seems to behave slightly differently on Windows/Mac but the reference result is from Linux
    # The results however are still acceptable
    system = platform.system()
    if system == "Windows":
        if platform.architecture()[0] == "64bit":
            return 1e-2 # Other fitting tests seem to require this level too.
        else:
            return 1e-1
    elif system == "Darwin":
        return 1e-1 # Other fitting tests seem to require this level too.
    else:
        return 1e-6

#------------------------------------------------------------------------------------------------------------------

class VesuvioFittingTest(stresstesting.MantidStressTest):

    def runTest(self):
        do_fit_no_background(k_is_free=False)

        self.assertTrue(WS_PREFIX + "_Workspace" in mtd, "Expected function workspace in ADS")
        self.assertTrue(WS_PREFIX + "_Parameters" in mtd, "Expected parameters workspace in ADS")
        self.assertTrue(WS_PREFIX + "_NormalisedCovarianceMatrix" in mtd, "Expected covariance workspace in ADS")

    def validate(self):
        self.tolerance = tolerance()
        return "fit_Workspace","VesuvioFittingTest.nxs"

#------------------------------------------------------------------------------------------------------------------

class VesuvioFittingWithKFreeTest(stresstesting.MantidStressTest):

    def runTest(self):
        do_fit_no_background(k_is_free=True)

        self.assertTrue(WS_PREFIX + "_Workspace" in mtd, "Expected function workspace in ADS")
        self.assertTrue(WS_PREFIX + "_Parameters" in mtd, "Expected parameters workspace in ADS")
        self.assertTrue(WS_PREFIX + "_NormalisedCovarianceMatrix" in mtd, "Expected covariance workspace in ADS")

    def validate(self):
        self.tolerance = tolerance()
        return "fit_Workspace","VesuvioFittingWithKFreeTest.nxs"

#------------------------------------------------------------------------------------------------------------------

class VesuvioFittingWithQuadraticBackgroundTest(stresstesting.MantidStressTest):

    def runTest(self):
        do_fit_with_quadratic_background()

        self.assertTrue(WS_PREFIX + "_Workspace" in mtd, "Expected function workspace in ADS")
        self.assertTrue(WS_PREFIX + "_Parameters" in mtd, "Expected parameters workspace in ADS")
        self.assertTrue(WS_PREFIX + "_NormalisedCovarianceMatrix" in mtd, "Expected covariance workspace in ADS")

    def validate(self):
        self.tolerance = tolerance()
        return "fit_Workspace","VesuvioFittingWithQuadraticBackgroundTest.nxs"

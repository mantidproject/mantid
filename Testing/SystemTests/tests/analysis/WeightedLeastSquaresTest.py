# pylint: disable=no-init,too-few-public-methods
#
# Here weighted least squares fitting tests, for which:
#
#   1. data must include errors (otherwise add systemtest to: UnweightedLeastSquaresTest.py)
#   2. data must not be processed before or after the fitting in these tests (we already have many systemtests where at
#      some point during an instrument specific data reduction/analysis workflow fitting is done)
#   3. where possible the input data is either specified in-line in the test or read from an ascii input data format
#
# The systemtests here
#
#   a. test Mantid fitting, on both examples where Mantid fitting appears to be doing the right thing and
#      cases for which we would like Mantid fitting to do better in the future
#   b. test rubostness and ability to rubustly reach the solution we would like a fitting minimizer to reach
#   c. are not performance tests (i.e. testing speed of a minimizer). For this please add performance tests
#   d. should be in a form such that these tests can also be used as fairly simple input into other
#      fitting packages (to give this option for testing other minimizers/algorithms, which may following such tests be
#      made available to Mantid users)
#

import stresstesting
from mantid.simpleapi import *


def run_fit(wks, function, minimizer):
    """
    Checks that a value 'val' does not differ from a reference value 'ref' by 'epsilon'
    or more. This method compares the relative error. An epsilon of 0.1 means a relative
    difference of 10 % = 100*0.1 %

    @param wks :: workspace with data
    @param function :: model + initial parameters to use to fit the data
    @param epsilon :: name of minimizer to use

    @returns the fitted parameter values and error estimates for these
    """
    Fit(function, wks, Output='ws',
        Minimizer=minimizer,
        CostFunction="Least squares")

    param_table = mtd['ws_Parameters']

    params = param_table.column(1)[:-1]
    errors = param_table.column(2)[:-1]

    return params, errors

def get_evs14188_90processed():
    """
    Load a processed ISIS Vesuvio data. This processed data contains a couple of peaks.

    @returns this data as a workspace
    """
    wks = Load(Filename='EVS14188-90_processed.txt')

    return wks

class WeigthedLSGaussPeaksEVSdataTest1(stresstesting.MantidStressTest):
    def runTest(self):
        # Use default Mantid Levenberg-Marquardt minimizer, which for this example it does not get to the expected
        # solution (minimum), starting from the below specified initial guess of the fitting parameters

        wks = get_evs14188_90processed()

        function = ("name=Gaussian,Height=0.01,PeakCentre=0.00037,Sigma=1e-05;name=LinearBackground,A0=0,A1=0;"
                    "name=Gaussian,Height=0.0979798,PeakCentre=0.000167,Sigma=1e-05")

        expected_params = [-0.02830944965319149, 0.0003966626475232753, 3.2690103473132937e-06, 0.04283560333422615,
                           -82.49982468272542, 0.13971885301153036, 0.00016591941312628293, 9.132514633819799e-06]
        expected_errors = [0.007480178355054269, 9.93278345856534e-07, 9.960514853350883e-07, 0.0017945463077016224,
                           4.9824412855830404, 0.004955791268590802, 3.695975249653185e-07, 3.8197105944596216e-07]

        # osx exception
        import sys
        if "darwin" == sys.platform:
            expected_errors[2] = 1.0077697381037357e-06

        params, errors = run_fit(wks, function, 'Levenberg-Marquardt')

        for err, expected in zip(errors, expected_errors):
            self.assertDelta(err / expected - 1.0, 0.0, 1e-2)

        for val, expected in zip(params, expected_params):
            self.assertDelta(val / expected - 1.0, 0.0, 2e-5)

class WeigthedLSGaussPeaksEVSdataTest2(stresstesting.MantidStressTest):
    def runTest(self):
        # Same as WeigthedLSGaussPeaksEVSdataTest1 but starting from a different initial guess of the fitting
        # parameters. Here the minmizer gets to the expected solution (minimum), i.e. do the right thing

        wks = get_evs14188_90processed()

        function = ("name=Gaussian,Height=0.0271028,PeakCentre=0.000371946,Sigma=1e-05;name=LinearBackground,A0=0,A1=0;"
                    "name=Gaussian,Height=0.0979798,PeakCentre=0.000167,Sigma=1.7267e-05")

        expected_params = [0.117909282702681, 0.0003733355959906781, 4.750983503334754e-06, 0.002725666504029797,
                           -4.494580010809393, 0.12986299166539694, 0.00016646632365980064, 2.616230019006275e-05]
        expected_errors = [0.00655660314595665, 3.021546058414827e-07, 3.0766264397350073e-07, 0.0023835827954566415,
                           5.996463420450547, 0.003059328883379551, 6.632752531256318e-07, 7.707070805005832e-07]

        # osx and windows exception
        import sys
        if "darwin" == sys.platform:
            # This is on osx; on win7-release, slightly different, see below
            expected_errors[2] = 3.027791099421756e-07
            expected_errors[7] = 7.797742226241165e-07
        if "win32" == sys.platform:
            expected_errors[2] = 3.0277910994217546e-07

        params, errors = run_fit(wks, function, 'Levenberg-Marquardt')

        for err, expected in zip(errors, expected_errors):
            self.assertDelta(err / expected - 1.0, 0.0, 1e-2)

        for val, expected in zip(params, expected_params):
            self.assertDelta(val / expected - 1.0, 0.0, 2e-5)

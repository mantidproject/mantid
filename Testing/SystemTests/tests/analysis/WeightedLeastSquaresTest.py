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
from abc import ABCMeta, abstractmethod

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


# This defines a base class that can be used then in individual system tests (if we want every test
# as an individual stresstesting.MantidStressTest). An alternative is to use Python unittest as for
# example in LoadVesuvioTest. I find the second alternative a bit neater if we add several more
# datasets and fit tests which tend to have a very repetitive structure.
class WeightedLSSysTest(stresstesting.MantidStressTest):
    """
    Base class for fail/success tests of fitting for different input files (datasets).
    """

    __metaclass__ = ABCMeta # We don't want a test from this class

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6
        self.minimizer = 'Levenberg-Marquardt'
        self.input_data_file = ''
        self.function_definition = ''
        self.fitted_params = []
        self.expected_params = []

    def requiredFiles(self):
        return set([self.input_data_file])

    def compare_relative_err(self, val, ref):
        """
        Checks that a value 'val' does not differ from a reference value 'ref' by a tolerance/threshold
        or more. This method compares the relative error. An epsilon of 0.01 means a relative
        difference of 1 % = 100*0.01 %. If the reference value is 0, checks that the difference
        from 0 is less than the tolerance.

        @param val :: value obtained from a calculation or algorithm (fitting for example)
        @param ref :: (expected) reference value

        @returns if val differs in relative terms from ref by less than epsilon
        """
        if 0 == ref:
            logger.information("Trying to calculate relative error with respect to 0. "
                               "Checking absolute difference from 0. Make sure this is the intended behavior!")
            check = (abs(val - ref) < self.tolerance)
        else:
            check = (abs(val/ref - 1) < self.tolerance)

        if not check:
            print("Value '{0}' differs from reference '{1}' by more than required tolerance '{2}'".
                  format(val, ref, self.tolerance))

        return check

    def validate(self):
        if not self.input_data_file:
            raise RuntimeError("The input data file cannot be empty.")

        if not self.fitted_params:
            raise RuntimeError("The fitted params have not been generated.")

        if not self.expected_params:
            raise RuntimeError("The expected parameters have not been set.")

        if len(self.fitted_params) != len(self.expected_params):
            raise RuntimeError("The number of fitted parameters does not match the number of "
                               "expected parameters")

        for idx, (val, expected) in enumerate(zip(self.fitted_params, self.expected_params)):
            self.assertTrue(self.compare_relative_err(val, expected),
                            "Relative error bigger than acceptable (tolerance: {0}) when "
                            "comparing parameter number {1} against expected value. Got: {2}. "
                            "Expected: {3}".format(self.tolerance, idx+1, val, expected))

        return True

    @abstractmethod
    def runTest(self):
        """This needs to be overriden in the concrete tests"""


class WeightedLSSineLikeMuonExperimentAsymmetry(WeightedLSSysTest):
    """
    A base class for tests of the fitting of a function that resembles the
    asymmetry from a muon experiment.

    Tests the fitting of a simplified (and synthetic) sine function that looks
    like the asymmetry functions that they usually fit when processing
    data from muon experiments.

    The motivation for this test is that the results seem to be very
    poor unless you have a very good initial guess of the
    frequency. Example: for a frequency 6, an initial guess of 5 or 7
    will produce rubbish results. This happens even with 0 noise.

    Initial values of w approximately <=5.25 or >=6.75 will make the minimizer fail.

    To keep it simple there is 0 noise, and the phase is 0.
    """

    __metaclass__ = ABCMeta # We don't want a test from this class

    def __init__(self):
        WeightedLSSysTest.__init__(self)
        self.input_data_file = 'sine_fitting_test_muon_asymmetry.txt'
        self.sine_user_def_function = 'name=UserFunction, Formula=sin(w*x)'

        self.workspace = LoadAscii(self.input_data_file, OutputWorkspace='fit_test_ws')

    @abstractmethod
    def runTest(self):
        """This needs to be overriden in the concrete test classes"""


class WeightedLSSineLikeMuonExperimentAsymmetry_Fails(WeightedLSSineLikeMuonExperimentAsymmetry):
    """
    This tests a fit failure. Initial guess of frequency not good enough (5.2 too far off 6)
    """

    def __init__(self):
        WeightedLSSineLikeMuonExperimentAsymmetry.__init__(self)
        self.function_definition = ("{0}, w=5.2".format(self.sine_user_def_function))
        self.expected_params = [4.753040119492522]

    def runTest(self):
        # Note: ignoring parameter errors
        self.fitted_params, _ = run_fit(self.workspace,
                                        self.function_definition, self.minimizer)

class WeightedLSSineLikeMuonExperimentAsymmetry_Good(WeightedLSSineLikeMuonExperimentAsymmetry):
    """
    This tests a fit that works. Initial guess of frequency close enough to real value.
    """

    def __init__(self):
        WeightedLSSineLikeMuonExperimentAsymmetry.__init__(self)
        self.function_definition = ("{0}, w=5.4".format(self.sine_user_def_function))
        self.expected_params = [6.000000000717283]

    def runTest(self):
        # Note: ignoring parameter errors
        self.fitted_params, _ = run_fit(self.workspace, self.function_definition, self.minimizer)


class WeightedLSVanadiumPatternFromENGINXSmoothing(WeightedLSSysTest):
    """
    A base class for tests of the fitting of data from a Vanadium run on the instrument ENGIN-X.
    This uses data collected for one bank.

    In the new ENGIN-X algorithms/scripts/interface this pattern is usually smoothed with
    a spline with 20-50 knots. This is used for calibration.
    """

    __metaclass__ = ABCMeta # We don't want a test from this class

    def __init__(self):
        WeightedLSSysTest.__init__(self)
        self.input_data_file = 'fitting_test_vanadium_pattern_enginx236516_bank1.txt'
        self.spline_user_def_function = 'name=BSpline,Uniform=true,Order=3, StartX=0, EndX=5.5'

        self.workspace = LoadAscii(self.input_data_file, OutputWorkspace='fit_test_ws')

    @abstractmethod
    def runTest(self):
        """This needs to be overriden in the concrete test classes"""

class WeightedLSVanadiumPatternFromENGINXSmoothing_50(WeightedLSVanadiumPatternFromENGINXSmoothing):

    """
    This tests a normal fit with a spline with 50 knots. This is currently producing results that look
    satisfactory to instrument scientists.

    Note that the status returned by Fit is 'cannot reach the specified tolerance in X'
    """

    def __init__(self):
        WeightedLSVanadiumPatternFromENGINXSmoothing.__init__(self)
        self.tolerance = 1e-4
        self.function_definition = ("{0}, NBreak=50".format(self.spline_user_def_function))
        self.expected_params = [0.0, 0.0, -36.8891264687, 18.2621705377, -2.26934877029,
                                74.1623483093, 489.805793426, 922.50307265, 1262.00262279, 1600.0337548,
                                1968.43336803, 2139.75598267, 2193.84158372, 2147.43156063, 2024.21157276,
                                1846.18150427, 1667.08750361, 1435.20910859, 1283.45295698, 1114.83687664,
                                948.124414481, 795.827508616, 674.396619086, 566.73758527, 470.013330458,
                                405.87940959, 343.150383227, 317.744533119, 287.247200073, 253.308328823,
                                224.945339773, 197.474782309, 175.017313505, 158.65592759, 134.930581111,
                                113.464625925, 103.075403043, 88.6933038825, 73.2454285003, 57.9475439827,
                                46.1502382308, 33.4958056688, 27.023943173, 19.6592967824, 14.8481655913,
                                9.65510062749, 5.73133751077, 1.94165179183, -0.908404460069, 8.86320436969,
                                0.0]

    def runTest(self):
        # Note: ignoring parameter errors
        self.fitted_params, _ = run_fit(self.workspace, self.function_definition, self.minimizer)

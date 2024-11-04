# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

import systemtesting
from mantid.api import mtd, MatrixWorkspace
from mantid.kernel import logger
from mantid.simpleapi import Fit, LoadAscii

import unittest


def run_fit(wks, function, minimizer="Levenberg-Marquardt", cost_function="Least squares"):
    """
    Fits the data in a workspace with a function, using the algorithm Fit.
    Importantly, the option IgnoreInvalidData is enabled. Check the documentation of Fit for the
    implications of this.

    @param wks :: MatrixWorkspace with data to fit, in the format expected by the algorithm Fit
    @param function :: function definition as used in the algorithm Fit
    @param minimizer :: minimizer to use in Fit
    @param cost_function :: cost function to use in Fit

    @returns the fitted parameter values and error estimates for these
    """
    Fit(function, wks, Output="ws", Minimizer=minimizer, CostFunction=cost_function, IgnoreInvalidData=True)

    param_table = mtd["ws_Parameters"]

    params = param_table.column(1)[:-1]
    errors = param_table.column(2)[:-1]

    return params, errors


def compare_relative_errors(values_fitted, reference_values, tolerance=1e-6):
    """
    Checks that all the values 'fitted' do not differ from reference values by
    a tolerance/threshold or more. This method compares the relative error. An
    epsilon of 0.01 means a relative difference of 1 % = 100*0.01 %. If the
    reference value is 0, checks that the difference from 0 is less than the
    tolerance.

    @param values_fitted :: parameter values obtained from the Fit algorithm
    (this function can be used to compare function parameters, error estimations, etc.)
    @param reference_values :: (expected) reference value

    @returns if val differs in relative terms from ref by less than epsilon. Otherwise RuntimeError is
    raised with description why comparison/test failed.
    """

    if not isinstance(values_fitted, list) or not isinstance(reference_values, list) or len(values_fitted) != len(reference_values):
        raise ValueError(
            "The values fitted and the reference values must be provided as lists (with the same "
            "number of elements). Got A: {0} and B: {1}".format(values_fitted, reference_values)
        )

    for idx, (val, expected) in enumerate(zip(values_fitted, reference_values)):
        if 0 == expected:
            logger.information(
                "Trying to calculate relative error with respect to 0. "
                "Checking absolute difference from 0. Make sure this is the intended behavior!"
            )
            check = abs(val - expected) < tolerance
        else:
            check = abs(val / expected - 1) < tolerance

        if not check:
            logger.error(
                "For the parameter with index {0}, the value found '{1}' differs from "
                "reference '{2}' by more than required tolerance '{3}'".format(idx, val, expected, tolerance)
            )
            logger.error("These were the values found:         {0}".format(values_fitted))
            logger.error(" and these are the reference values: {0}".format(reference_values))
            raise RuntimeError("Some results were not as accurate as expected. Please check the log " "messages for details")


def load_fitting_test_file_ascii(filename):
    """
    Loads an ascii file in with X,Y,E columns

    @returns this data as a workspace
    """
    wks = LoadAscii(Filename=filename)
    if not wks or not isinstance(wks, MatrixWorkspace):
        raise RuntimeError("Input workspace from file {0} not available as a MatrixWorkspace. " "Cannot continue.".format(filename))

    return wks


# pylint: disable=too-many-public-methods


class TwoGaussPeaksEVSData(unittest.TestCase):
    """
    Load a processed ISIS Vesuvio data.

    Representative of a processed Vesuvio dataset that contains a couple of peaks.
    """

    filename = "EVS14188-90_processed.txt"
    workspace = None
    function_template = "name=Gaussian, {0} ; name=LinearBackground,A0=0,A1=0;" "name=Gaussian, {1}"

    # Using this workaround as we still support Python 2.6 on rhel6, where setUpClass()
    # is not available
    def setUp(self):
        if not self.__class__.workspace:
            self.__class__.workspace = load_fitting_test_file_ascii(self.filename)

    def test_good_initial_guess(self):
        """
        Same as test_misses_expected_solution but starting from a different initial guess of the fitting
        parameters. Here the minmizer gets to the expected solution (minimum), i.e. do the right thing
        """
        function = self.function_template.format(
            "Height=0.0271028,PeakCentre=0.000371946,Sigma=1e-05", "Height=0.0979798,PeakCentre=0.000167,Sigma=1.7267e-05"
        )

        expected_params = [
            0.117909282702681,
            0.0003733355959906781,
            4.750983503334754e-06,
            0.002725666504029797,
            -4.494580010809393,
            0.12986299166539694,
            0.00016646632365980064,
            2.616230019006275e-05,
        ]
        expected_errors = [
            0.00655660314595665,
            3.021546058414827e-07,
            3.0766264397350073e-07,
            0.0023835827954566415,
            5.996463420450547,
            0.003059328883379551,
            6.632752531256318e-07,
            7.707070805005832e-07,
        ]

        params, errors = run_fit(self.workspace, function, "Levenberg-Marquardt")
        compare_relative_errors(params, expected_params, tolerance=3e-1)
        compare_relative_errors(errors, expected_errors, tolerance=3e-1)


class SineLikeMuonExperimentAsymmetry(unittest.TestCase):
    """
    Tests of the fitting of a function that resembles the asymmetry from a muon experiment.

    The test data is a simplified (and synthetic) sine function (sin(w*x)) that looks like the
    asymmetry functions that they usually fit when processing data from muon experiments.

    The motivation for this test is that the results seem to be very poor unless you have a
    very good initial guess of w. Example: for w=6, an initial guess of 5 or 7 will produce
    rubbish results. This happens even with 0 noise.

    Initial values of w approximately <=5.25 or >=6.75 will make the minimizer fail.
    This is very sensitive to initial conditions. The goodness of fit is highly non-convex on w.
    Any local minimizer should be very sensitive to the initial guess.
    """

    filename = "sine_fitting_test_muon_asymmetry.txt"
    workspace = None
    function_template = "name=UserFunction, Formula=sin(w*x), w={0}"

    def setUp(self):
        if not self.__class__.workspace:
            self.__class__.workspace = load_fitting_test_file_ascii(self.filename)

    def test_bad_initial_guess(self):
        """
        This tests a fit failure. Initial guess of w not good enough (5.2 too far off 6)
        """
        function_definition = self.function_template.format("5.2")
        expected_params = [4.753040119492522]

        # Note: ignoring parameter errors
        fitted_params, _ = run_fit(self.workspace, function_definition)
        compare_relative_errors(fitted_params, expected_params)

    def test_good_initial_guess(self):
        """
        This tests a fit that works. Initial guess of frequency close enough to real value.
        """
        function_definition = self.function_template.format("5.4")
        expected_params = [6.000000000717283]
        fitted_params, _ = run_fit(self.workspace, function_definition)
        compare_relative_errors(fitted_params, expected_params)


class VanadiumPatternFromENGINXSmoothing(unittest.TestCase):
    """
    Tests the fitting of data from a Vanadium run on the instrument ENGIN-X. This uses data
    collected for one bank (North bank, all specta focused).

    In the new ENGIN-X algorithms/scripts/interface this pattern is usually smoothed with
    a spline with 20-50 knots. This is used for calibration.
    """

    filename = "fitting_test_vanadium_pattern_enginx236516_bank1.txt"
    workspace = None
    spline_user_def_function = "name=BSpline, Uniform=true, Order=3, StartX=0, EndX=5.5, NBreak={0}"

    def setUp(self):
        if not self.__class__.workspace:
            self.__class__.workspace = load_fitting_test_file_ascii(self.filename)

    def test_50breaks(self):
        """
        This tests a normal fit with a spline with 50 knots. This is currently producing results that look
        satisfactory to instrument scientists.
        """
        function_definition = self.spline_user_def_function.format("50")
        expected_params = [
            0.0,
            0.0,
            -37.210995939539366,
            18.370350660372594,
            -2.323438604684101,
            74.22342247724607,
            489.75852793518493,
            922.5302436427901,
            1261.989106878403,
            1600.0406590395235,
            1968.4303057681236,
            2139.756948117313,
            2193.8413904726463,
            2147.4316461957706,
            2024.2113711188294,
            1846.1816530559352,
            1667.087333750607,
            1435.2097252716287,
            1283.45250983148,
            1114.837123909836,
            948.1243026646681,
            795.8275797795692,
            674.3966788220177,
            566.7374883589133,
            470.0133310752506,
            405.87945288846436,
            343.15039206081804,
            317.7445190490894,
            287.2471905069133,
            253.30824044242098,
            224.9453886333567,
            197.47473222574482,
            175.01736182667756,
            158.6559088656412,
            134.93057836157,
            113.46466051206023,
            103.07539466368209,
            88.69333062995749,
            73.2453746596794,
            57.94761712646885,
            46.150107399338026,
            33.49607446438909,
            27.023391825663943,
            19.660388795715143,
            14.846016985914035,
            9.65919973049868,
            5.724008517073549,
            1.9527932349469075,
            -0.9197805852038337,
            10.656047152998436,
            0.0,
        ]

        # Note: ignoring parameter errors. Note the higher tolerance so that it works on all platforms
        fitted_params, _ = run_fit(self.workspace, function_definition)
        compare_relative_errors(fitted_params, expected_params, tolerance=1e-4)

    def test_12breaks(self):
        """
        This uses 12 break points, which usually produces poorish results.
        """
        function_definition = self.spline_user_def_function.format("12")
        expected_params = [
            575.5043460508207,
            -362.0695583401004,
            722.7394915082397,
            2621.9749776340186,
            1572.450059153195,
            836.417481475315,
            361.6875979793134,
            240.00983642384153,
            132.46098325093416,
            63.95362315830608,
            17.41805806345004,
            0.8684078907341928,
            -5.204195324981802,
        ]

        # Note: ignoring parameter errors. Note the higher tolerance so that it works on all platforms
        fitted_params, _ = run_fit(self.workspace, function_definition)
        compare_relative_errors(fitted_params, expected_params, tolerance=1e-4)


class WeightedLeastSquaresTest(systemtesting.MantidSystemTest):
    _success = False

    def runTest(self):
        self._success = False
        # Custom code to create and run one or more test suites
        suite = unittest.TestSuite()
        # Add the tests for all the datasets
        suite.addTest(unittest.makeSuite(TwoGaussPeaksEVSData, "test"))
        suite.addTest(unittest.makeSuite(SineLikeMuonExperimentAsymmetry, "test"))
        suite.addTest(unittest.makeSuite(VanadiumPatternFromENGINXSmoothing, "test"))

        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success

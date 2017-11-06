# pylint: disable=no-init
"""These are more integration tests as they will require that the test data is available
and that mantid can be imported
"""
import stresstesting
import platform
import numpy as np

from mantid.api import (WorkspaceGroup, MatrixWorkspace)
from mantid.simpleapi import *
from vesuvio.commands import fit_tof


# =====================================Helper Function=================================

def _is_old_boost_version():
    # It appears that a difference in boost version is causing different
    # random number generation. As such an OS check is used.
    # Older boost (earlier than 56): Ubuntu 14.04, RHEL7
    dist = platform.linux_distribution()
    if any(dist):
        if 'Red Hat' in dist[0] and dist[1].startswith('7'):
            return True
        if dist[0] == 'Ubuntu' and dist[1] == '14.04':
            return True

    return False


def _create_test_flags(background, multivariate=False):
    flags = dict()
    flags['fit_mode'] = 'spectrum'
    flags['spectra'] = '135'
    if multivariate:
        mass1 = {'value': 1.0079, 'function': 'MultivariateGaussian', 'SigmaX': 5, 'SigmaY': 5, 'SigmaZ': 5}
    else:
        mass1 = {'value': 1.0079, 'function': 'GramCharlier', 'width': [2, 5, 7],
                 'hermite_coeffs': [1, 0, 0], 'k_free': 0, 'sears_flag': 1}
    mass2 = {'value': 16.0, 'function': 'Gaussian', 'width': 10}
    mass3 = {'value': 27.0, 'function': 'Gaussian', 'width': 13}
    mass4 = {'value': 133.0, 'function': 'Gaussian', 'width': 30}
    flags['masses'] = [mass1, mass2, mass3, mass4]
    flags['intensity_constraints'] = [0, 1, 0, -4]
    if background:
        flags['background'] = {'function': 'Polynomial', 'order': 3}
    else:
        flags['background'] = None
    flags['ip_file'] = 'Vesuvio_IP_file_test.par'
    flags['diff_mode'] = 'single'
    flags['gamma_correct'] = True
    flags['ms_flags'] = dict()
    flags['ms_flags']['SampleWidth'] = 10.0
    flags['ms_flags']['SampleHeight'] = 10.0
    flags['ms_flags']['SampleDepth'] = 0.5
    flags['ms_flags']['SampleDensity'] = 241
    flags['fit_minimizer'] = 'Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08'

    return flags


def _equal_within_tolerance(self, expected, actual, tolerance=0.05):
    """
    Checks the expected value is equal to the actual value with in a percentage of tolerance
    """
    tolerance_value = expected * tolerance
    abs_difference = abs(expected - actual)
    self.assertTrue(abs_difference <= abs(tolerance_value))


def _get_peak_height_and_index(workspace, ws_index):
    """
    returns the maximum height in y of a given spectrum of a workspace
    workspace is assumed to be a matrix workspace
    """
    y_data = workspace.readY(ws_index)
    peak_height = np.amax(y_data)
    peak_bin = np.argmax(y_data)

    return peak_height, peak_bin


# ====================================================================================


class FitSingleSpectrumNoBackgroundTest(stresstesting.MantidStressTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_wsg = self._fit_results[0]
        self.assertTrue(isinstance(fitted_wsg, WorkspaceGroup))
        self.assertEqual(1, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(7, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = -0.013011414483
        index_one_last = 0.00720741862173
        index_two_first = 1.12713408816e-05
        index_two_last = 6.90222280789e-05
        if _is_old_boost_version():
            index_one_first = 0.000631295911554

        _equal_within_tolerance(self, index_one_first, fitted_ws.readY(0)[0])
        _equal_within_tolerance(self, index_one_last, fitted_ws.readY(0)[-1])
        _equal_within_tolerance(self, index_two_first, fitted_ws.readY(1)[0])
        _equal_within_tolerance(self, index_two_last, fitted_ws.readY(1)[-1])

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(14, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class FitSingleSpectrumBivariateGaussianTiesTest(stresstesting.MantidStressTest):
    """
    Test ensures that internal ties for mass profiles work correctly
    This test ties SigmaX to SigmaY making the multivariate gaussian
    a Bivariate Gaussian
    """

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        flags = _create_test_flags(background=False, multivariate=True)
        flags['masses'][0]['ties'] = 'SigmaX=SigmaY'
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        # Get fit workspace
        fit_params = mtd['15039-15045_params_iteration_1']
        f0_sigma_x = fit_params.readY(2)[0]
        f0_sigma_y = fit_params.readY(3)[0]
        self.assertAlmostEqual(f0_sigma_x, f0_sigma_y)


# ====================================================================================


class SingleSpectrumBackground(stresstesting.MantidStressTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=True)
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_wsg = self._fit_results[0]
        self.assertTrue(isinstance(fitted_wsg, WorkspaceGroup))
        self.assertEqual(1, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(8, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = -0.00553133541138
        index_one_last = 0.00722053823154
        calc_data_height_expected = 0.13302098172
        calc_data_bin_expected = 635
        if _is_old_boost_version():
            index_one_first = 0.000605572768745

        _equal_within_tolerance(self, index_one_first, fitted_ws.readY(0)[0])
        _equal_within_tolerance(self, index_one_last, fitted_ws.readY(0)[-1])

        calc_data_height_actual, calc_data_bin_actual = _get_peak_height_and_index(fitted_ws, 1)
        _equal_within_tolerance(self, calc_data_height_expected, calc_data_height_actual)
        self.assertTrue(abs(calc_data_bin_expected - calc_data_bin_actual) <= 1)

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(18, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class BankByBankForwardSpectraNoBackground(stresstesting.MantidStressTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        flags['fit_mode'] = 'bank'
        flags['spectra'] = 'forward'
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEquals(4, len(self._fit_results))

        fitted_banks = self._fit_results[0]
        self.assertTrue(isinstance(fitted_banks, WorkspaceGroup))
        self.assertEqual(8, len(fitted_banks))

        bank1 = fitted_banks[0]
        self.assertTrue(isinstance(bank1, MatrixWorkspace))

        self.assertAlmostEqual(50.0, bank1.readX(0)[0])
        self.assertAlmostEqual(562.0, bank1.readX(0)[-1])

        _equal_within_tolerance(self, 8.23840378769e-05, bank1.readY(1)[0])
        _equal_within_tolerance(self, 0.000556695665501, bank1.readY(1)[-1])

        bank8 = fitted_banks[7]
        self.assertTrue(isinstance(bank8, MatrixWorkspace))

        self.assertAlmostEqual(50.0, bank8.readX(0)[0])
        self.assertAlmostEqual(562.0, bank8.readX(0)[-1])

        _equal_within_tolerance(self, 0.00025454613205, bank8.readY(1)[0])
        _equal_within_tolerance(self, 0.00050412575393, bank8.readY(1)[-1])

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(8, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class SpectraBySpectraForwardSpectraNoBackground(stresstesting.MantidStressTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        flags['fit_mode'] = 'spectra'
        flags['spectra'] = '143-144'
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEquals(4, len(self._fit_results))

        fitted_spec = self._fit_results[0]
        self.assertTrue(isinstance(fitted_spec, WorkspaceGroup))
        self.assertEqual(2, len(fitted_spec))

        spec143 = fitted_spec[0]
        self.assertTrue(isinstance(spec143, MatrixWorkspace))

        self.assertAlmostEqual(50.0, spec143.readX(0)[0])
        self.assertAlmostEqual(562.0, spec143.readX(0)[-1])

        _equal_within_tolerance(self, 2.27289862507e-06, spec143.readY(1)[0])
        _equal_within_tolerance(self, 3.49287467421e-05, spec143.readY(1)[-1])

        spec144 = fitted_spec[1]
        self.assertTrue(isinstance(spec144, MatrixWorkspace))

        self.assertAlmostEqual(50.0, spec144.readX(0)[0])
        self.assertAlmostEqual(562.0, spec144.readX(0)[-1])

        _equal_within_tolerance(self, 5.9811662524e-06, spec144.readY(1)[0])
        _equal_within_tolerance(self, 4.7479831769e-05, spec144.readY(1)[-1])

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(2, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

# ====================================================================================

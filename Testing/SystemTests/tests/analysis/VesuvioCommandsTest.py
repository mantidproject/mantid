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


#=====================================Helper Function=================================

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

def _create_test_flags(background):
    flags = dict()
    flags['fit_mode'] = 'spectrum'
    flags['spectra'] = '135'

    mass1 = {'value': 1.0079, 'function': 'GramCharlier', 'width': [2, 5, 7],
             'hermite_coeffs': [1, 0, 0], 'k_free': 0, 'sears_flag': 1}
    mass2 = {'value': 16.0, 'function': 'Gaussian', 'width': 10}
    mass3 = {'value': 27.0, 'function': 'Gaussian', 'width': 13}
    mass4 = {'value': 133.0, 'function': 'Gaussian', 'width': 30}
    flags['masses'] = [mass1, mass2, mass3, mass4]
    flags['intensity_constraints'] = [0, 1, 0, -4]
    if background:
        flags['background'] = {'function': 'Polynomial', 'order':3}
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

def _get_maximum_peak_height(workspace, ws_index):
    """
    returns the maximum height in y of a given spectrum of a workspace
    workspace is assumed to be a matrix workspace
    """
    y_data = workspace.readY(ws_index)
    peak_height = np.amax(y_data)
    return peak_height

#====================================================================================

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
        self.assertEqual(2, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(7, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = -0.016289703
        index_one_last = 0.0072029933
        index_two_first = 1.057476742e-05
        index_two_last = 7.023179770e-05
        if _is_old_boost_version():
            index_one_first = 7.798020e-04

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

#====================================================================================

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
        self.assertEqual(2, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(8, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = -0.0221362198069
        index_one_last = 0.00720728978699
        calc_data_height = 0.138704
        if _is_old_boost_version():
            index_one_first = 6.809169e-04
            index_one_last = 7.206634e-03

        _equal_within_tolerance(self, index_one_first, fitted_ws.readY(0)[0])
        _equal_within_tolerance(self, index_one_last, fitted_ws.readY(0)[-1])
        _equal_within_tolerance(self, calc_data_height, _get_maximum_peak_height(fitted_ws, 1))

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(18, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

#====================================================================================

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
        self.assertTrue(isinstance(fitted_banks, list))
        self.assertEqual(8, len(fitted_banks))

        bank1 = fitted_banks[0]
        self.assertTrue(isinstance(bank1, WorkspaceGroup))

        bank1_data = bank1[0]
        self.assertTrue(isinstance(bank1_data, MatrixWorkspace))

        self.assertAlmostEqual(50.0, bank1_data.readX(0)[0])
        self.assertAlmostEqual(562.0, bank1_data.readX(0)[-1])

        _equal_within_tolerance(self, 8.03245852426e-05, bank1_data.readY(1)[0])
        _equal_within_tolerance(self, 0.000559789299755, bank1_data.readY(1)[-1])

        bank8 = fitted_banks[-1]
        self.assertTrue(isinstance(bank8, WorkspaceGroup))

        bank8_data = bank8[0]
        self.assertTrue(isinstance(bank8_data, MatrixWorkspace))

        self.assertAlmostEqual(50.0, bank8_data.readX(0)[0])
        self.assertAlmostEqual(562.0, bank8_data.readX(0)[-1])

        _equal_within_tolerance(self, 0.000279169151321, bank8_data.readY(1)[0])
        _equal_within_tolerance(self, 0.000505355349359, bank8_data.readY(1)[-1])

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(8, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

#====================================================================================

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
        self.assertTrue(isinstance(fitted_spec, list))
        self.assertEqual(2, len(fitted_spec))

        spec143 = fitted_spec[0]
        self.assertTrue(isinstance(spec143, WorkspaceGroup))

        spec143_data = spec143[0]
        self.assertTrue(isinstance(spec143_data, MatrixWorkspace))

        self.assertAlmostEqual(50.0, spec143_data.readX(0)[0])
        self.assertAlmostEqual(562.0, spec143_data.readX(0)[-1])

        _equal_within_tolerance(self, 2.3090594752e-06, spec143_data.readY(1)[0])
        _equal_within_tolerance(self, 3.51960367895e-05, spec143_data.readY(1)[-1])

        spec144 = fitted_spec[-1]
        self.assertTrue(isinstance(spec144, WorkspaceGroup))

        spec144_data = spec144[0]
        self.assertTrue(isinstance(spec144_data, MatrixWorkspace))

        self.assertAlmostEqual(50.0, spec144_data.readX(0)[0])
        self.assertAlmostEqual(562.0, spec144_data.readX(0)[-1])

        _equal_within_tolerance(self, 7.79185212491e-06, spec144_data.readY(1)[0])
        _equal_within_tolerance(self, 4.79448882168e-05, spec144_data.readY(1)[-1])

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(2, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

#====================================================================================

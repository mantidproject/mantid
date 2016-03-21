# pylint: disable=no-init
"""These are more integration tests as they will require that the test data is available
and that mantid can be imported
"""
import stresstesting

from mantid.api import (WorkspaceGroup, MatrixWorkspace)
from mantid.simpleapi import *
from vesuvio.commands import fit_tof


#=====================================Helper Function=================================

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

#====================================================================================
'''
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

        self.assertAlmostEqual(-0.016289703, fitted_ws.readY(0)[0])
        self.assertAlmostEqual(0.0072029933, fitted_ws.readY(0)[-1])
        self.assertAlmostEqual(1.057476742e-05, fitted_ws.readY(1)[0])
        self.assertAlmostEqual(7.023179770e-05, fitted_ws.readY(1)[-1])

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(10, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

#====================================================================================
'''
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

        self.assertAlmostEqual(-0.0221362198069, fitted_ws.readY(0)[0])
        self.assertAlmostEqual(0.00720728978699, fitted_ws.readY(0)[-1])
        self.assertAlmostEqual(0.00571520523979, fitted_ws.readY(1)[0])
        self.assertAlmostEqual(-0.00211277263055, fitted_ws.readY(1)[-1])

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(14, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

#====================================================================================
'''
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

        # self.assertAlmostEqual(50.0, bank1_data.readX(0)[0])
        # self.assertAlmostEqual(562.0, bank1_data.readX(0)[-1])

        # self.assertAlmostEqual(0.000107272755986595, bank1_data.readY(1)[0])
        # self.assertAlmostEqual(0.000585633970072128, bank1_data.readY(1)[-1])

        bank8 = fitted_banks[-1]
        self.assertTrue(isinstance(bank8, WorkspaceGroup))

        bank8_data = bank8[0]
        self.assertTrue(isinstance(bank8_data, MatrixWorkspace))

        # self.assertAlmostEqual(50.0, bank8_data.readX(0)[0])
        # self.assertAlmostEqual(562.0, bank8_data.readX(0)[-1])

        # self.assertAlmostEqual(0.000596850729120898, bank8_data.readY(1)[0])
        # self.assertAlmostEqual(0.000529343513813141, bank8_data.readY(1)[-1])

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

        # self.assertAlmostEqual(50.0, spec143_data.readX(0)[0])
        # self.assertAlmostEqual(562.0, spec143_data.readX(0)[-1])

        # self.assertAlmostEqual(2.37897941103748e-06, spec143_data.readY(1)[0])
        # self.assertAlmostEqual(3.58226563303213e-05, spec143_data.readY(1)[-1])

        spec144 = fitted_spec[-1]
        self.assertTrue(isinstance(spec144, WorkspaceGroup))

        spec144_data = spec144[0]
        self.assertTrue(isinstance(spec144_data, MatrixWorkspace))

        # self.assertAlmostEqual(50.0, spec144_data.readX(0)[0])
        # self.assertAlmostEqual(562.0, spec144_data.readX(0)[-1])

        # self.assertAlmostEqual(5.57952304659615e-06, spec144_data.readY(1)[0])
        # self.assertAlmostEqual(6.00056973529846e-05, spec144_data.readY(1)[-1])

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, list))
        self.assertEqual(2, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))
'''
#====================================================================================

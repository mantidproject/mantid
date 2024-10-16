# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""These are more integration tests as they will require that the test data is available
and that mantid can be imported
"""

import systemtesting
import numpy as np

from mantid.api import mtd, WorkspaceGroup, MatrixWorkspace
from mantid.simpleapi import CropWorkspace, LoadVesuvio, Rebin
from vesuvio.commands import fit_tof
from vesuvio.instrument import VESUVIO


# =====================================Helper Function=================================


def _make_names_unique(names):
    name_count = dict()

    for name in names:
        if name in name_count:
            name_count[name] = name_count[name] + 1
            yield name + str(name_count[name])
        else:
            name_count[name] = 1
            yield name


def _test_caad_workspace(self, workspace_name, functions):
    try:
        caad_workspace = mtd[workspace_name]
    except RuntimeError:
        self.fail("CAAD Workspace with name " + workspace_name + " does not exist in the ADS.")

    self.assertTrue(isinstance(caad_workspace, WorkspaceGroup), "CAAD Workspace, '" + workspace_name + "', is not a WorkspaceGroup.")

    fit_caad_name = workspace_name + "_Calc"
    self.assertTrue(
        caad_workspace.contains(fit_caad_name), "Calculated fit workspace, '" + fit_caad_name + "', not found in CAAD WorkspaceGroup."
    )

    for suffix in _make_names_unique(functions):
        name = workspace_name + "_" + suffix
        self.assertTrue(caad_workspace.contains(name), "'" + name + "' not found in CAAD WorkspaceGroup.")


def _create_test_flags(background, multivariate=False):
    flags = dict()
    flags["fit_mode"] = "spectrum"
    if multivariate:
        mass1 = {"value": 1.0079, "function": "MultivariateGaussian", "SigmaX": 5, "SigmaY": 5, "SigmaZ": 5}
    else:
        mass1 = {"value": 1.0079, "function": "GramCharlier", "width": [2, 5, 7], "hermite_coeffs": [1, 0, 0], "k_free": 0, "sears_flag": 1}
    mass2 = {"value": 16.0, "function": "Gaussian", "width": 10}
    mass3 = {"value": 27.0, "function": "Gaussian", "width": 13}
    mass4 = {"value": 133.0, "function": "Gaussian", "width": 30}
    flags["masses"] = [mass1, mass2, mass3, mass4]
    flags["intensity_constraints"] = [0, 1, 0, -4]
    if background:
        flags["background"] = {"function": "Polynomial", "order": 3}
    else:
        flags["background"] = None
    flags["ip_file"] = "Vesuvio_IP_file_test.par"
    flags["diff_mode"] = "single"
    flags["gamma_correct"] = True
    flags["ms_flags"] = dict()
    flags["ms_flags"]["SampleWidth"] = 10.0
    flags["ms_flags"]["SampleHeight"] = 10.0
    flags["ms_flags"]["SampleDepth"] = 0.5
    flags["ms_flags"]["SampleDensity"] = 241
    flags["fit_minimizer"] = "Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08"

    return flags


def _equal_within_tolerance(self, expected, actual, tolerance=0.05):
    """
    Checks the expected value is equal to the actual value with in a percentage of tolerance
    """
    tolerance_value = expected * tolerance
    abs_difference = abs(expected - actual)
    self.assertLessEqual(abs_difference, abs(tolerance_value), msg="abs({:.6f} - {:.6f}) > {:.6f}".format(expected, actual, tolerance))


def _get_peak_height_and_index(workspace, ws_index):
    """
    returns the maximum height in y of a given spectrum of a workspace
    workspace is assumed to be a matrix workspace
    """
    y_data = workspace.readY(ws_index)
    peak_height = np.amax(y_data)
    peak_bin = np.argmax(y_data)

    return peak_height, peak_bin


def load_and_crop_data(runs, spectra, ip_file, diff_mode="single", fit_mode="spectra", rebin_params=None):
    """
    @param runs The string giving the runs to load
    @param spectra A list of spectra to load
    @param ip_file A string denoting the IP file
    @param diff_mode Either 'double' or 'single'
    @param fit_mode If bank then the loading is changed to summing each bank to a separate spectrum
    @param rebin_params Rebin parameter string to rebin data by (no rebin if None)
    """
    instrument = VESUVIO()
    load_banks = fit_mode == "bank"
    output_name = runs + "_" + spectra + "_tof"

    if load_banks:
        sum_spectra = True
        if spectra == "forward":
            bank_ranges = instrument.forward_banks
        elif spectra == "backward":
            bank_ranges = instrument.backward_banks
        else:
            raise ValueError("Fitting by bank requires selecting either 'forward' or 'backward' " "for the spectra to load")
        bank_ranges = ["{0}-{1}".format(x, y) for x, y in bank_ranges]
        spectra = ";".join(bank_ranges)
    else:
        sum_spectra = False
        if spectra == "forward":
            spectra = "{0}-{1}".format(*instrument.forward_spectra)
        elif spectra == "backward":
            spectra = "{0}-{1}".format(*instrument.backward_spectra)

    if diff_mode == "double":
        diff_mode = "DoubleDifference"
    else:
        diff_mode = "SingleDifference"

    kwargs = {
        "Filename": runs,
        "Mode": diff_mode,
        "InstrumentParFile": ip_file,
        "SpectrumList": spectra,
        "SumSpectra": sum_spectra,
        "OutputWorkspace": output_name,
        "StoreInADS": False,
    }
    full_range = LoadVesuvio(**kwargs)
    tof_data = CropWorkspace(
        InputWorkspace=full_range, XMin=instrument.tof_range[0], XMax=instrument.tof_range[1], OutputWorkspace=output_name, StoreInADS=False
    )

    if rebin_params is not None:
        tof_data = Rebin(InputWorkspace=tof_data, OutputWorkspace=output_name, Params=rebin_params, StoreInADS=False)

    return tof_data


# ====================================================================================


class FitSingleSpectrumNoBackgroundTest(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        runs = "15039-15045"
        flags["spectra"] = "135"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_wsg = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_wsg, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_wsg)) + "'."
        )
        self.assertEqual(1, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(7, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = 0.000794
        index_one_last = 0.007207
        index_two_first = 1.127134e-05
        index_two_last = 6.902223e-05

        _equal_within_tolerance(self, index_one_first, fitted_ws.readY(0)[0])
        _equal_within_tolerance(self, index_one_last, fitted_ws.readY(0)[-1])
        _equal_within_tolerance(self, index_two_first, fitted_ws.readY(1)[0])
        _equal_within_tolerance(self, index_two_last, fitted_ws.readY(1)[-1])

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(14, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class FitSingleSpectrumBivariateGaussianTiesTest(systemtesting.MantidSystemTest):
    """
    Test ensures that internal ties for mass profiles work correctly
    This test ties SigmaX to SigmaY making the multivariate gaussian
    a Bivariate Gaussian
    """

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        flags = _create_test_flags(background=False, multivariate=True)
        flags["spectra"] = "135"
        flags["masses"][0]["ties"] = "SigmaX=SigmaY"
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        # Get fit workspace
        fit_params = mtd["15039-15045_params_iteration_1"]
        f0_sigma_x = fit_params.readY(2)[0]
        f0_sigma_y = fit_params.readY(3)[0]
        self.assertAlmostEqual(f0_sigma_x, f0_sigma_y)


# ====================================================================================


class SingleSpectrumBackground(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=True)
        flags["spectra"] = "135"
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_wsg = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_wsg, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_wsg)) + "'."
        )
        self.assertEqual(1, len(fitted_wsg))

        fitted_ws = fitted_wsg[0]
        self.assertTrue(isinstance(fitted_ws, MatrixWorkspace))
        self.assertEqual(8, fitted_ws.getNumberHistograms())

        self.assertAlmostEqual(50.0, fitted_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, fitted_ws.readX(0)[-1])

        index_one_first = 0.000819
        index_one_last = 0.007221
        calc_data_height_expected = 0.133021
        calc_data_bin_expected = 635

        _equal_within_tolerance(self, index_one_first, fitted_ws.readY(0)[0])
        _equal_within_tolerance(self, index_one_last, fitted_ws.readY(0)[-1])

        calc_data_height_actual, calc_data_bin_actual = _get_peak_height_and_index(fitted_ws, 1)
        _equal_within_tolerance(self, calc_data_height_expected, calc_data_height_actual)
        self.assertTrue(abs(calc_data_bin_expected - calc_data_bin_actual) <= 1)

        fitted_params = self._fit_results[1]
        self.assertTrue(isinstance(fitted_params, MatrixWorkspace))
        self.assertEqual(18, fitted_params.getNumberHistograms())

        chisq_values = self._fit_results[2]
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(1, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class BankByBankForwardSpectraNoBackground(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        flags["fit_mode"] = "bank"
        flags["spectra"] = "forward"
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_banks = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_banks, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_banks)) + "'."
        )
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
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(8, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class SpectraBySpectraForwardSpectraNoBackground(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        flags["fit_mode"] = "spectra"
        flags["spectra"] = "143-144"
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_spec = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_spec, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_spec)) + "'."
        )
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
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(2, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class PassPreLoadedWorkspaceToFitTOF(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        runs_ws = load_and_crop_data("15039-15045", "143-144", flags["ip_file"])
        self._fit_results = fit_tof(runs_ws, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_spec = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_spec, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_spec)) + "'."
        )
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
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(2, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))


# ====================================================================================


class CalculateCumulativeAngleAveragedData(systemtesting.MantidSystemTest):
    _fit_results = None

    def runTest(self):
        flags = _create_test_flags(background=False)
        flags["fit_mode"] = "bank"
        flags["spectra"] = "forward"
        flags["calculate_caad"] = True
        flags["load_log_files"] = False
        runs = "15039-15045"
        self._fit_results = fit_tof(runs, flags)

    def validate(self):
        self.assertTrue(isinstance(self._fit_results, tuple))
        self.assertEqual(4, len(self._fit_results))

        fitted_banks = self._fit_results[0]
        self.assertTrue(
            isinstance(fitted_banks, WorkspaceGroup), "Expected fit result to be a WorkspaceGroup, is '" + str(type(fitted_banks)) + "'."
        )
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
        self.assertTrue(isinstance(chisq_values, np.ndarray), msg="Chi-sq values is not a numpy array. Found {}".format(type(chisq_values)))
        self.assertEqual(8, len(chisq_values))

        exit_iteration = self._fit_results[3]
        self.assertTrue(isinstance(exit_iteration, int))

        functions = ["GramCharlierComptonProfile", "GaussianComptonProfile", "GaussianComptonProfile", "GaussianComptonProfile"]
        _test_caad_workspace(self, "15039-15045_CAAD_normalised_iteration_" + str(exit_iteration), functions)
        _test_caad_workspace(self, "15039-15045_CAAD_sum_iteration_" + str(exit_iteration), functions)

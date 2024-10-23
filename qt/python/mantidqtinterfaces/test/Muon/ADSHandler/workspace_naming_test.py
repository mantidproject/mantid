# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    add_phasequad_extensions,
    add_rebin_to_name,
    check_phasequad_name,
    create_covariance_matrix_name,
    create_fitted_workspace_name,
    create_model_fitting_parameter_combination_name,
    create_model_fitting_parameters_group_name,
    create_multi_domain_fitted_workspace_name,
    create_parameter_table_name,
    get_first_run_from_run_string,
    get_group_or_pair_from_name,
    get_pair_phasequad_name,
    get_period_from_raw_name,
    get_raw_data_workspace_name,
    get_run_number_from_raw_name,
    get_run_numbers_as_string_from_workspace_name,
    remove_rebin_from_name,
)
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantid.api import AnalysisDataService, FileFinder
from mantid import ConfigService
from mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename


class WorkspaceNamingTest(unittest.TestCase):
    def setUp(self):
        return

    def test_getGroupOrPairName(self):
        self.assertEqual(get_group_or_pair_from_name("MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA"), "bkwd")
        self.assertEqual(get_group_or_pair_from_name("MUSR62260; Group; fwd; Asymmetry; Periods; #2; MA"), "fwd")
        self.assertEqual(get_group_or_pair_from_name("MUSR62260; Pair Asym; long; Periods; #1; MA"), "long")

        self.assertEqual(get_group_or_pair_from_name("MUSR62260; PhaseQuad; test_Re_;MA"), "test_Re_")
        self.assertEqual(get_group_or_pair_from_name("MUSR62260; Diff; diff; Asymmetry; MA"), "diff")

    def test_removeRebinFromName(self):
        # std name
        name = "MUSR62260; Group; bkwd; Asymmetry; MA"
        self.assertEqual(name, remove_rebin_from_name(name))
        # with periods
        period_name = "MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA"
        self.assertEqual(period_name, remove_rebin_from_name(period_name))

        # std name and rebin
        name_rebin = "MUSR62260; Group; bkwd; Asymmetry; Rebin; MA"
        self.assertEqual(name, remove_rebin_from_name(name_rebin))

        # with periods and rebin
        period_name_rebin = "MUSR62260; Group; bkwd; Asymmetry; Periods; #1; Rebin; MA"
        self.assertEqual(period_name, remove_rebin_from_name(period_name_rebin))

    def test_add_rebin_to_name(self):
        # std name
        name = "MUSR62260; Group; bkwd; Asymmetry; MA"
        name_rebin = "MUSR62260; Group; bkwd; Asymmetry; Rebin; MA"
        self.assertEqual(name_rebin, add_rebin_to_name(name))
        self.assertEqual(name_rebin, add_rebin_to_name(name_rebin))

        # with periods
        period_name = "MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA"
        period_name_rebin = "MUSR62260; Group; bkwd; Asymmetry; Periods; #1; Rebin; MA"
        self.assertEqual(period_name_rebin, add_rebin_to_name(period_name))
        self.assertEqual(period_name_rebin, add_rebin_to_name(period_name_rebin))

    def test_get_run_number_from_raw_name(self):
        run = "2532"
        name = get_raw_data_workspace_name("HIFI", run, False)

        self.assertEqual(run, get_run_number_from_raw_name(name, "HIFI"))

    def test_get_period_from_raw_name_single(self):
        run = "2532"
        name = get_raw_data_workspace_name("HIFI", run, False)

        self.assertEqual("", get_period_from_raw_name(name, "MA"))

    def test_get_period_from_raw_name_mutli(self):
        run = "2532"
        name = get_raw_data_workspace_name("HIFI", run, True, "3")

        self.assertEqual("_period3 ", get_period_from_raw_name(name, "MA"))

    def test_check_phasequad_name(self):
        self.assertEqual(True, check_phasequad_name("Ref_data_Re_"))
        self.assertEqual(True, check_phasequad_name("Ref_data_Im_"))
        self.assertEqual(True, check_phasequad_name("Image_data_Re_"))
        self.assertEqual(True, check_phasequad_name("Image_data_Im_"))
        self.assertEqual(False, check_phasequad_name("Ref_data"))
        self.assertEqual(False, check_phasequad_name("Image_data"))

    def test_add_phasequad_extension(self):
        self.assertEqual("Ref_data_Re__Im_", add_phasequad_extensions("Ref_data"))

    def test_get_pair_phasequad_name(self):
        AnalysisDataService.clear()
        ConfigService["MantidOptions.InvisibleWorkspaces"] = "True"
        filepath = FileFinder.findRuns("EMU00019489.nxs")[0]

        load_result, run_number, filename, psi_data = load_workspace_from_filename(filepath)

        context = setup_context()
        context.gui_context.update({"RebinType": "None"})
        context.data_context.instrument = "EMU"

        context.data_context._loaded_data.add_data(workspace=load_result, run=[run_number], filename=filename, instrument="EMU")
        context.data_context.current_runs = [[run_number]]
        context.data_context.update_current_data()

        self.assertEqual("EMU19489; PhaseQuad; test_Re; MA", get_pair_phasequad_name(context, "test_Re", "19489", False))
        self.assertEqual("EMU19489; PhaseQuad; test_Re; Rebin; MA", get_pair_phasequad_name(context, "test_Re", "19489", True))

    def test_create_fitted_workspace_name(self):
        input_workspace_name = "MUSR22725; Group; top; Asymmetry; #1"
        trial_function_name = "GausOsc"
        expected_directory_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc/"
        expected_workspace_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc; Workspace"

        name, directory = create_fitted_workspace_name(input_workspace_name, trial_function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_parameter_table_name(self):
        input_workspace_name = "MUSR22725; Group; top; Asymmetry; #1"
        trial_function_name = "GausOsc"
        expected_directory_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc/"
        expected_workspace_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc; Parameters"

        name, directory = create_parameter_table_name(input_workspace_name, trial_function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_covariance_matrix_name(self):
        input_workspace_name = "MUSR22725; Group; top; Asymmetry; #1"
        trial_function_name = "GausOsc"
        expected_directory_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc/"
        expected_workspace_name = "MUSR22725; Group; top; Asymmetry; #1; Fitted;GausOsc; Normalised Covariance Matrix"

        name, directory = create_covariance_matrix_name(input_workspace_name, trial_function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_model_fitting_parameter_combination_name(self):
        results_table_name = "Result1"
        x_parameter = "A0"
        y_parameter = "A1"

        name = create_model_fitting_parameter_combination_name(results_table_name, x_parameter, y_parameter)

        self.assertEqual(name, "Result1; A1 vs A0")

    def test_create_model_fitting_parameters_group_name(self):
        results_table_name = "Result1"

        name = create_model_fitting_parameters_group_name(results_table_name)

        self.assertEqual(name, "Result1; Parameter Combinations")

    def test_create_multi_domain_fitted_workspace_name(self):
        input_workspace_name = "MUSR22725; Group; top; Asymmetry; #1"
        trial_function_name = "Polynomial"
        expected_directory_name = "MUSR22725; Group; top; Asymmetry; #1+ ...; Fitted;Polynomial/"
        expected_workspace_name = "MUSR22725; Group; top; Asymmetry; #1+ ...; Fitted;Polynomial"

        name, directory = create_multi_domain_fitted_workspace_name(input_workspace_name, trial_function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_get_run_numbers_as_string_from_workspace_name(self):
        instrument = "MUSR"
        runs = "62260"

        # test formatting
        workspace_name = "MUSR62260_raw_data MA"
        self.assertEqual(runs, get_run_numbers_as_string_from_workspace_name(workspace_name, instrument))
        workspace_name = "MUSR62260; GROUP; fwd; Asymmetry; MA"
        self.assertEqual(runs, get_run_numbers_as_string_from_workspace_name(workspace_name, instrument))

        # test multiple run numbers
        runs = "62260-62261,62263"
        workspace_name = "MUSR62260-62261,62263; GROUP; fwd; Asymmetry; MA"
        self.assertEqual(runs, get_run_numbers_as_string_from_workspace_name(workspace_name, instrument))

    def test_get_first_run_from_run_string_one_number(self):
        run_string = "62260"
        self.assertEqual("62260", get_first_run_from_run_string(run_string))

    def test_get_first_run_from_run_string_hyphen_first(self):
        run_string = "62260-2,62264,62267-9"
        self.assertEqual("62260", get_first_run_from_run_string(run_string))

    def test_get_first_run_from_run_string_comma_first(self):
        run_string = "62260,62262-62264,62267-9"
        self.assertEqual("62260", get_first_run_from_run_string(run_string))


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)

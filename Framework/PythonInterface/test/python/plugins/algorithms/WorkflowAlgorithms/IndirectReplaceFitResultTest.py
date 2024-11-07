# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    CompareWorkspaces,
    ConvolutionFitSequential,
    ConvolutionFitSimultaneous,
    LoadNexus,
    IndirectReplaceFitResult,
)
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup
from mantid.buildconfig import GSL_VERSION


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name)


def add_to_ads(workspace_name, workspace):
    return AnalysisDataService.addOrReplace(workspace_name, workspace)


def execute_algorithm(input_workspace, single_fit_workspace, output_name):
    IndirectReplaceFitResult(InputWorkspace=input_workspace, SingleFitWorkspace=single_fit_workspace, OutputWorkspace=output_name)


def get_sequential_input_string(workspace_name, number_of_histograms):
    input_string = ""
    for i in range(number_of_histograms):
        input_string += "{0},i{1};".format(workspace_name, i)
    return input_string


def current_os_has_gslv2():
    """Check whether the current OS is using GSLv2"""
    return int(GSL_VERSION[0]) >= 2


class IndirectReplaceFitResultTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._input_name = "iris26176_graphite002_conv_1L_s0_to_17__Result"
        cls._single_fit_name = "iris26176_graphite002_red_conv_1L_1__Result"
        cls._result_group_name = "iris26176_graphite002_conv_1L_s0_to_17_Results"
        cls._single_fit_group_name = "iris26176_graphite002_red_conv_1L_1_Results"
        cls._output_name = "iris26176_graphite002_conv_1L_s0_to_17_Result_Edit"

        red_name = "iris26176_graphite002_red"
        res_name = "iris26173_graphite002_res"
        fit_function = (
            "composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution, "
            "Workspace=__ConvFitResolution0,WorkspaceIndex=0,X=(),Y=();name=Lorentzian,"
        )
        start_x = "-0.54761329492537092"
        end_x = "0.54325428563315703"
        convolve_members = True
        max_iterations = "5000"

        # Perform fits to produce the result workspaces to use for a data replacement
        LoadNexus(Filename=red_name + ".nxs", OutputWorkspace=red_name)
        LoadNexus(Filename=res_name + ".nxs", OutputWorkspace=res_name)
        LoadNexus(Filename="conv_fit_resolution.nxs", OutputWorkspace="__ConvFitResolution0")
        ConvolutionFitSequential(
            Input=get_sequential_input_string(red_name, 18),
            Function=fit_function + "Amplitude=1,PeakCentre=0,FWHM=0.0175",
            MaxIterations=max_iterations,
            ConvolveMembers=convolve_members,
            PassWSIndexToFunction="1",
            StartX=start_x,
            EndX=end_x,
            OutputWorkspace=cls._result_group_name,
            OutputParameterWorkspace="iris26176_graphite002_conv_1L_s0_to_17_Parameters",
            OutputWorkspaceGroup="iris26176_graphite002_conv_1L_s0_to_17_Workspaces",
        )
        ConvolutionFitSimultaneous(
            InputWorkspace=red_name,
            Function=fit_function + "Amplitude=4.96922,PeakCentre=-0.000735068,FWHM=0.0671319",
            MaxIterations=max_iterations,
            ConvolveMembers=convolve_members,
            WorkspaceIndex="1",
            StartX=start_x,
            EndX=end_x,
            OutputWorkspace=cls._single_fit_group_name,
            OutputParameterWorkspace="iris26176_graphite002_conv_1L_s1_Parameters",
            OutputWorkspaceGroup="iris26176_graphite002_conv_1L_s1_Workspaces",
        )

    def setUp(self):
        self._input_workspace = get_ads_workspace(self._input_name)
        self._single_fit_workspace = get_ads_workspace(self._single_fit_name)
        self._result_group = get_ads_workspace(self._result_group_name)

    def tearDown(self) -> None:
        AnalysisDataService.remove(self._output_name)

    def test_that_the_result_group_contains_a_single_workspace(self):
        self.assertTrue(isinstance(self._result_group, WorkspaceGroup))
        self.assertEqual(self._result_group.getNumberOfEntries(), 1)
        self.assertEqual(self._result_group.getNames()[0], self._input_name)

    def test_that_the_algorithm_does_not_throw_when_given_valid_properties(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)

        output = get_ads_workspace(self._output_name)
        self.assertTrue(isinstance(output, MatrixWorkspace))

    def test_that_the_algorithm_will_create_an_output_group_with_two_workspaces_when_the_output_name_is_different(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)
        self.assertEqual(self._result_group.getNumberOfEntries(), 2)

    def test_that_the_algorithm_will_create_an_output_group_with_one_workspace_when_the_output_name_is_the_same(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._input_name)
        self.assertEqual(self._result_group.getNumberOfEntries(), 1)

    def test_that_the_algorithm_will_throw_when_given_a_single_fit_workspace_containing_more_than_one_fit(self):
        with self.assertRaisesRegex(RuntimeError, "The single fit workspace must contain data from a single fit."):
            execute_algorithm(self._input_workspace, self._input_workspace, self._output_name)

    def test_that_the_algorithm_will_throw_when_given_an_input_workspace_containing_data_for_one_fit(self):
        with self.assertRaisesRegex(RuntimeError, "The input workspace must contain result data from a fit involving 2 or more spectra."):
            execute_algorithm(self._single_fit_workspace, self._single_fit_workspace, self._output_name)

    def test_that_the_algorithm_will_throw_when_provided_an_empty_output_workspace_name(self):
        with self.assertRaisesRegex(RuntimeError, "No OutputWorkspace name was provided."):
            execute_algorithm(self._input_workspace, self._single_fit_workspace, "")

    def test_that_the_algorithm_produces_a_workspace_containing_the_expected_values(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)

        output = get_ads_workspace(self._output_name)
        expected_name = self._output_name + "_gslv2" if current_os_has_gslv2() else self._output_name
        expected_output = LoadNexus(Filename=expected_name + ".nxs")
        result, _ = CompareWorkspaces(output, expected_output, Tolerance=0.0001)
        self.assertTrue(result)

    def test_that_get_indices_of_equivalent_labels_will_return_the_correct_indices(self):
        from IndirectReplaceFitResult import get_indices_of_equivalent_labels

        indices = get_indices_of_equivalent_labels(self._input_workspace, self._single_fit_workspace)

        self.assertEqual(indices[0], 0)
        self.assertEqual(indices[1], 1)

    def test_that_fit_parameter_missing_returns_false_when_there_are_no_fit_parameters_missing(self):
        from IndirectReplaceFitResult import fit_parameter_missing

        self.assertFalse(fit_parameter_missing(self._single_fit_workspace, self._input_workspace, ["Chi_squared"]))

    def test_that_fit_parameter_missing_returns_true_when_a_fit_parameter_is_missing(self):
        from IndirectReplaceFitResult import fit_parameter_missing

        self.assertFalse(fit_parameter_missing(self._single_fit_workspace, self._input_workspace, []))

    def test_that_get_x_insertion_index_returns_the_expected_insertion_index(self):
        from IndirectReplaceFitResult import get_x_insertion_index

        index = get_x_insertion_index(self._input_workspace, self._single_fit_workspace)

        self.assertEqual(index, 1)


if __name__ == "__main__":
    unittest.main()

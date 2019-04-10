# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import (CompareWorkspaces, ConvolutionFitSequential, ConvolutionFitSimultaneous, LoadNexus,
                              IndirectReplaceFitResult, RenameWorkspace)
from mantid.api import (AnalysisDataService, MatrixWorkspace, WorkspaceGroup)


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name)


def add_to_ads(workspace_name, workspace):
    return AnalysisDataService.addOrReplace(workspace_name, workspace)


def execute_algorithm(input_workspace, single_fit_workspace, output_name):
    IndirectReplaceFitResult(InputWorkspace=input_workspace,
                             SingleFitWorkspace=single_fit_workspace,
                             OutputWorkspace=output_name)


class IndirectReplaceFitResultTest(unittest.TestCase):
    def setUp(self):
        self._input_name = 'iris26176_graphite002_conv_1L_s0_to_17_Result'
        self._single_fit_name = 'iris26176_graphite002_conv_1L_s1_Result'
        self._result_group_name = 'iris26176_graphite002_conv_1L_s0_to_17_Results'
        self._output_name = 'iris26176_graphite002_conv_1L_s0_to_17_Result_Edit'

        LoadNexus(Filename='conv_fit_resolution.nxs', OutputWorkspace='__ConvFitResolution0')
        LoadNexus(Filename='iris26176_graphite002_red.nxs', OutputWorkspace='iris26176_graphite002_red')
        LoadNexus(Filename='iris26173_graphite002_res.nxs', OutputWorkspace='iris26173_graphite002_res')
        ConvolutionFitSequential(Input='iris26176_graphite002_red,i0;iris26176_graphite002_red,i1;iris26176_graphite'
                                       '002_red,i2;iris26176_graphite002_red,i3;iris26176_graphite002_red,i4;iris26176'
                                       '_graphite002_red,i5;iris26176_graphite002_red,i6;iris26176_graphite002_red,i7;'
                                       'iris26176_graphite002_red,i8;iris26176_graphite002_red,i9;iris26176_graphite'
                                       '002_red,i10;iris26176_graphite002_red,i11;iris26176_graphite002_red,i12;'
                                       'iris26176_graphite002_red,i13;iris26176_graphite002_red,i14;iris26176_graphite'
                                       '002_red,i15;iris26176_graphite002_red,i16;iris26176_graphite002_red,i17;',
                                 OutputWorkspace='iris26176_graphite002_conv_1L_s0_to_17_Results',
                                 OutputParameterWorkspace='iris26176_graphite002_conv_1L_s0_to_17_Parameters',
                                 OutputWorkspaceGroup='iris26176_graphite002_conv_1L_s0_to_17_Workspaces',
                                 Function='composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,'
                                          'Workspace=__ConvFitResolution0,WorkspaceIndex=0,X=(),Y=();name=Lorentzian,'
                                          'Amplitude=1,PeakCentre=0,FWHM=0.0175',
                                 StartX='-0.54761329492537092', EndX='0.54325428563315703',  PassWSIndexToFunction='1',
                                 MaxIterations='5000', ConvolveMembers='1')
        RenameWorkspace(InputWorkspace='iris26176_graphite002_conv_1L_s0_to_17_Results_1',
                        OutputWorkspace=self._input_name)
        ConvolutionFitSimultaneous(Function='composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,'
                                            'Workspace=__ConvFitResolution0,WorkspaceIndex=0,X=(),Y=();name=Lorentzian,'
                                            'Amplitude=4.96922,PeakCentre=-0.000735068,FWHM=0.0671319',
                                   InputWorkspace='iris26176_graphite002_red',  MaxIterations='5000',
                                   ConvolveMembers='1',  OutputWorkspace='iris26176_graphite002_conv_1L_s1_Results',
                                   OutputParameterWorkspace='iris26176_graphite002_conv_1L_s1_Parameters',
                                   OutputWorkspaceGroup='iris26176_graphite002_conv_1L_s1_Workspaces',
                                   WorkspaceIndex='1', StartX='-0.54761329492537092', EndX='0.54325428563315703')
        RenameWorkspace(InputWorkspace='iris26176_graphite002_conv_1L_s1_Results_1',
                        OutputWorkspace=self._single_fit_name)

        #LoadNexus(Filename=self._input_name + '.nxs', OutputWorkspace=self._input_name)
        #LoadNexus(Filename=self._single_fit_name + '.nxs', OutputWorkspace=self._single_fit_name)

        result_group = WorkspaceGroup()
        result_group.addWorkspace(get_ads_workspace(self._input_name))
        add_to_ads(self._result_group_name, result_group)

        self._input_workspace = get_ads_workspace(self._input_name)
        self._single_fit_workspace = get_ads_workspace(self._single_fit_name)
        self._result_group = result_group

    def test_that_the_result_group_contains_the_correct_number_of_workspaces(self):
        self.assertTrue(isinstance(self._result_group, WorkspaceGroup))
        self.assertEqual(self._result_group.getNumberOfEntries(), 1)

    def test_that_the_group_workspace_contains_the_workspace_with_the_correct_name(self):
        self.assertEquals(len(self._result_group.getNames()), 1)
        self.assertEquals(self._result_group.getNames()[0], self._input_name)

    def test_that_the_workspaces_set_up_have_the_correct_names(self):
        self.assertEqual(self._input_workspace.getName(), self._input_name)
        self.assertEqual(self._single_fit_workspace.getName(), self._single_fit_name)
        self.assertEqual(self._result_group.getName(), self._result_group_name)

    def test_that_the_algorithm_does_not_throw_when_given_valid_properties(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)

        output = get_ads_workspace(self._output_name)
        self.assertTrue(isinstance(output, MatrixWorkspace))

    def test_that_the_algorithm_will_create_an_output_group_with_two_workspaces_when_the_output_name_is_different(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)
        self.assertEquals(self._result_group.getNumberOfEntries(), 2)

    def test_that_the_algorithm_will_create_an_output_group_with_one_workspace_when_the_output_name_is_the_same(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._input_name)
        self.assertEquals(self._result_group.getNumberOfEntries(), 1)

    def test_that_the_algorithm_will_throw_when_given_a_single_fit_workspace_containing_more_than_one_fit(self):
        with self.assertRaises(RuntimeError):
            execute_algorithm(self._input_workspace, self._input_workspace, self._output_name)

    def test_that_the_algorithm_will_throw_when_given_an_input_workspace_containing_data_for_one_fit(self):
        with self.assertRaises(RuntimeError):
            execute_algorithm(self._single_fit_workspace, self._single_fit_workspace, self._output_name)

    def test_that_the_algorithm_will_throw_when_provided_an_empty_output_workspace_name(self):
        with self.assertRaises(RuntimeError):
            execute_algorithm(self._input_workspace, self._single_fit_workspace, '')

    def test_that_the_algorithm_will_throw_when_provided_an_single_fit_workspace_which_is_a_group(self):
        with self.assertRaises(RuntimeError):
            execute_algorithm(self._input_workspace, self._result_group, self._output_name)

    def test_that_the_algorithm_produces_a_workspace_containing_the_expected_values(self):
        execute_algorithm(self._input_workspace, self._single_fit_workspace, self._output_name)

        output = get_ads_workspace(self._output_name)
        expected_output = LoadNexus(Filename=self._output_name + '.nxs')

        (result, messages) = CompareWorkspaces(output, expected_output, Tolerance=0.0000001)
        self.assertEqual(result, True)


if __name__ == '__main__':
    unittest.main()

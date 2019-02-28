# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import (CreateSimulationWorkspace, CreateWorkspace, CompareWorkspaces, DeleteWorkspace,
                              ISISIndirectEnergyTransfer, ISISIndirectEnergyTransferWrapper)
from mantid.api import WorkspaceGroup

import numpy as np


def _generate_calibration_workspace(instrument, ws_name='__calib'):
    simulation_workspace = CreateSimulationWorkspace(Instrument=instrument,
                                                     BinParams='0,1,2')
    number_of_histograms = simulation_workspace.getNumberHistograms()

    calib_workspace = CreateWorkspace(OutputWorkspace=ws_name,
                                      DataX=np.ones(1),
                                      DataY=np.ones(number_of_histograms),
                                      NSpec=number_of_histograms,
                                      ParentWorkspace=simulation_workspace)

    DeleteWorkspace(simulation_workspace)

    return calib_workspace


class ISISIndirectEnergyTransferWrapperTest(unittest.TestCase):

    def test_that_a_basic_reduction_will_result_in_an_output_group_containing_a_workspace_with_the_correct_name(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

    def test_that_a_basic_reduction_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_basic_reduction_will_produce_a_workspace_with_the_correct_x_axis_units(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getAxis(0).getUnit().unitID(), 'DeltaE')

    def test_that_a_basic_reduction_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53])

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_performed_with_a_spectra_range_will_result_in_a_workspace_with_a_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[35, 40])

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 6)

    def test_that_a_reduction_performed_with_a_grouping_method_of_all_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      GroupingMethod='All')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 1)

    def test_that_a_reduction_performed_with_a_grouping_method_of_all_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               GroupingMethod='All')

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      GroupingMethod='All')

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_performed_with_a_grouping_method_of_individual_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      GroupingMethod='Individual')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_reduction_performed_with_a_grouping_method_of_individual_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               GroupingMethod='Individual')

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      GroupingMethod='Individual')

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_with_a_background_subtraction_will_produce_an_output_group_containing_a_workspace_with_the_correct_name(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      BackgroundRange=[70000, 75000])

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

    def test_that_a_reduction_with_a_background_subtraction_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               BackgroundRange=[70000, 75000])

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      BackgroundRange=[70000, 75000])

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_which_is_given_an_output_unit_will_produce_a_workspace_with_the_correct_units(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      UnitX='DeltaE_inWavenumber')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)
        self.assertEqual(red_workspace.getAxis(0).getUnit().unitID(), 'DeltaE_inWavenumber')

    def test_that_a_reduction_with_a_detailed_balance_produces_an_output_workspace_with_the_correct_naming_and_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      DetailedBalance='300')

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_reduction_with_a_detailed_balance_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               DetailedBalance='300')

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      DetailedBalance='300')

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_using_a_map_file_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['OSI97919.raw'],
                                                      Instrument='OSIRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[963, 1004],
                                                      GroupingMethod='File',
                                                      MapFile='osi_002_14Groups.map')

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'osiris97919_graphite002_red')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 14)

    def test_that_a_reduction_using_a_map_file_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['OSI97919.raw'],
                                               Instrument='OSIRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[963, 1004],
                                               GroupingMethod='File',
                                               MapFile='osi_002_14Groups.map')

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['OSI97919.raw'],
                                                      Instrument='OSIRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[963, 1004],
                                                      GroupingMethod='File',
                                                      MapFile='osi_002_14Groups.map')

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_using_a_calibration_workspace_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_reduction_using_a_calibration_workspace_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_with_a_calibration_workspace_and_range_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[35, 40],
                                                      CalibrationWorkspace=_generate_calibration_workspace('IRIS'))

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 6)

    def test_that_a_multi_file_reduction_will_produce_multiple_reduced_workspaces_with_the_correct_names(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(workspace), 2)
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')
        self.assertEqual(workspace.getNames()[1], 'iris26173_graphite002_red')

    def test_that_a_multi_file_reduction_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53])

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_a_reduction_with_multiple_files_when_summing_each_of_the_runs_will_produce_a_workspace_with_the_correct_name(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                                      SumFIles=True,
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(workspace), 1)
        self.assertEqual(workspace.getNames()[0], 'iris26176_multi_graphite002_red')

    def test_a_reduction_with_multiple_files_when_summing_each_of_the_runs_will_produce_a_workspace_with_the_correct_run_numbers(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                                      SumFIles=True,
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        red_workspace = workspace[0]
        self.assertTrue('multi_run_numbers' in red_workspace.getRun())
        self.assertEqual(red_workspace.getRun().get('multi_run_numbers').value, '26176,26173')

    def test_a_reduction_with_multiple_files_when_summing_each_of_the_runs_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                               SumFIles=True,
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53])

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW', 'IRS26173.RAW'],
                                                      SumFIles=True,
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_runtime_error_is_raised_when_there_is_an_instrument_validation_failure(self):
        with self.assertRaises(RuntimeError):
            ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                              Instrument='IRIS',
                                              Analyser='graphite',
                                              Reflection='006',
                                              SpectraRange=[3, 53],
                                              OutputWorkspace='__ISISIndirectEnergyTransferTest_ws')

    def test_that_a_runtime_error_is_raised_when_the_grouping_method_is_Workspace_but_no_workspace_is_provided(self):
        with self.assertRaises(RuntimeError):
            ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                              Instrument='IRIS',
                                              Analyser='graphite',
                                              Reflection='002',
                                              SpectraRange=[3, 53],
                                              GroupingMethod='Workspace',
                                              OutputWorkspace='__ISISIndirectEnergyTransferTest_ws')

    def test_that_a_reduction_with_a_manual_efixed_will_return_a_workspace_with_the_correct_naming_and_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      Efixed=1.9)

        self.assertTrue(isinstance(workspace, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(workspace.getNames()[0], 'iris26176_graphite002_red')

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_reduction_with_a_manual_efixed_which_is_the_same_as_the_default_will_produce_an_identical_workspace(self):
        reference = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      Efixed=1.845)

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_with_a_manual_efixed_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               Efixed=1.9)

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      Efixed=1.9)

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_a_reduction_with_scale_factor_will_produce_a_workspace_with_the_correct_number_of_histograms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      ScaleFactor=0.5)

        red_workspace = workspace.getItem(0)
        self.assertEqual(red_workspace.getNumberHistograms(), 51)

    def test_that_a_reduction_with_scale_factor_will_produce_the_same_result_as_the_ISISIndirectEnergyTransfer_algorithm(self):
        reference = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                               Instrument='IRIS',
                                               Analyser='graphite',
                                               Reflection='002',
                                               SpectraRange=[3, 53],
                                               ScaleFactor=0.5)

        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53],
                                                      ScaleFactor=0.5)

        self.assertTrue(CompareWorkspaces(reference, workspace)[0])

    def test_that_the_history_contains_only_one_parent_algorithm_which_has_the_correct_name(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        history = workspace.getItem(0).getHistory()
        algorithm_names = [alg.name() for alg in history]

        self.assertEqual(len(algorithm_names), 1)
        self.assertEqual(algorithm_names[0], 'ISISIndirectEnergyTransferWrapper')

    def test_that_the_history_of_ISISIndirectEnergyTransferWrapper_has_no_child_algorithms(self):
        workspace = ISISIndirectEnergyTransferWrapper(InputFiles=['IRS26176.RAW'],
                                                      Instrument='IRIS',
                                                      Analyser='graphite',
                                                      Reflection='002',
                                                      SpectraRange=[3, 53])

        history = workspace.getItem(0).getHistory()

        child_histories = history.getAlgorithmHistory(0).getChildHistories()
        algorithm_names = [alg.name() for alg in child_histories]

        self.assertFalse(algorithm_names)


if __name__ == '__main__':
    unittest.main()

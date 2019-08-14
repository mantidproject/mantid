# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import AnalysisDataService, FileFinder
from mantid import ConfigService
from mantid.dataobjects import Workspace2D
from mantid.simpleapi import CreateWorkspace
from collections import Counter
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class MuonContextTest(unittest.TestCase):
    def setUp(self):
        AnalysisDataService.clear()
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'True'
        self.filepath = FileFinder.findRuns('EMU00019489.nxs')[0]

        self.load_result, self.run_number, self.filename, psi_data = load_workspace_from_filename(self.filepath)
        self.assert_(not psi_data)

        self.context = setup_context()
        self.context.gui_context.update({'RebinType': 'None'})
        self.loaded_data = self.context.data_context._loaded_data
        self.data_context = self.context.data_context
        self.gui_context = self.context.gui_context
        self.group_pair_context = self.context.group_pair_context
        self.data_context.instrument = 'EMU'

        self.loaded_data.add_data(workspace=self.load_result, run=[self.run_number], filename=self.filename,
                                  instrument='EMU')
        self.data_context.current_runs = [[self.run_number]]
        self.data_context.update_current_data()
        self.group_pair_context.reset_group_and_pairs_to_default(self.load_result['OutputWorkspace'][0]._workspace,
                                                                 'EMU', '')

    def tearDown(self):
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'False'

    def populate_ADS(self):
        self.context.calculate_all_groups()
        self.context.show_all_groups()
        self.context.calculate_all_pairs()
        self.context.show_all_pairs()
        workspace = CreateWorkspace([0], [0], StoreInADS=False)
        self.context.phase_context.add_phase_quad(
            MuonWorkspaceWrapper(workspace, 'EMU19489; PhaseQuad; PhaseTable EMU19489'))

    def test_reset_groups_and_pairs_to_default(self):
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])
        self.assertEqual(self.group_pair_context.pair_names, ['long'])

    def test_calculate_group_calculates_group_for_given_run(self):
        counts_workspace, asymmetry_workspace, group_asymmetry_unormalised = self.context.calculate_group('fwd',
                                                                                                          run=[19489])

        self.assertEqual(type(counts_workspace), Workspace2D)
        self.assertEqual(type(asymmetry_workspace), Workspace2D)
        self.assertEqual(type(group_asymmetry_unormalised), Workspace2D)

    def test_calculate_pair_calculates_pair_for_given_run(self):
        pair_asymmetry = self.context.calculate_pair('long', run=[19489])

        self.assertEqual(type(pair_asymmetry), Workspace2D)

    def test_show_all_groups_calculates_and_shows_all_groups(self):
        self.context.show_all_groups()

        self.assertEquals(Counter(AnalysisDataService.getObjectNames()),
                          Counter(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                   '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                   'EMU19489 MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                   'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; fwd; Asymmetry; MA',
                                   'EMU19489; Group; fwd; Counts; MA']))

    def test_that_show_all_calculates_and_shows_all_groups_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_groups()

        self.assertEquals(Counter(AnalysisDataService.getObjectNames()),
                          Counter(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                   '__EMU19489; Group; bwd; Asymmetry; Rebin; MA_unnorm',
                                   '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                   '__EMU19489; Group; fwd; Asymmetry; Rebin; MA_unnorm',
                                   'EMU19489 MA',
                                   'EMU19489; Group; bwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; Rebin; MA',
                                   'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; bwd; Counts; Rebin; MA',
                                   'EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; fwd; Asymmetry; Rebin; MA',
                                   'EMU19489; Group; fwd; Counts; MA', 'EMU19489; Group; fwd; Counts; Rebin; MA']))

    def test_show_all_pairs_calculates_and_shows_all_pairs(self):
        self.context.show_all_pairs()

        self.assertEquals(Counter(AnalysisDataService.getObjectNames()),
                          Counter(['EMU19489 MA', 'EMU19489; Pair Asym; long; MA']))

    def test_that_show_all_calculates_and_shows_all_pairs_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_pairs()

        self.assertEquals(Counter(AnalysisDataService.getObjectNames()),
                          Counter(['EMU19489 MA', 'EMU19489; Pair Asym; long; MA',
                                   'EMU19489; Pair Asym; long; Rebin; MA']))

    def test_update_current_data_sets_current_run_in_data_context(self):
        self.context.update_current_data()

        self.assertEqual(self.data_context.current_data, self.load_result)

    def test_update_current_data_sets_groups_and_pairs(self):
        self.context.update_current_data()

        self.assertEqual(self.group_pair_context.pair_names, ['long'])
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])

    def test_show_raw_data_puts_raw_data_into_the_ADS(self):
        self.context.show_raw_data()

        self.assertEquals(Counter(AnalysisDataService.getObjectNames()),
                          Counter(['EMU19489 MA', 'EMU19489_raw_data MA']))

    def test_that_first_good_data_returns_correctly_when_from_file_chosen_option(self):
        self.gui_context.update({'FirstGoodDataFromFile': True})

        first_good_data = self.context.first_good_data([19489])

        self.assertEqual(first_good_data, 0.11)

    def test_first_good_data_returns_correctly_when_manually_specified_used(self):
        self.gui_context.update({'FirstGoodDataFromFile': False, 'FirstGoodData': 5})

        first_good_data = self.context.first_good_data([19489])

        self.assertEqual(first_good_data, 5)

    def test_that_last_good_data_returns_correctly_when_from_file_chosen_option(self):
        self.gui_context.update({'LastGoodDataFromFile': True})

        last_good_data = self.context.last_good_data([19489])

        self.assertEqual(last_good_data, 31.76)

    def test_last_good_data_returns_correctly_when_manually_specified_used(self):
        self.gui_context.update({'LastGoodDataFromFile': False, 'LastGoodData': 5})

        last_good_data = self.context.last_good_data([19489])

        self.assertEqual(last_good_data, 5)

    def test_that_dead_time_table_from_ADS_returns_table_name(self):
        self.gui_context.update({'DeadTimeSource': 'FromADS', 'DeadTimeTable': 'deadtime_table_name'})

        deadtime_table = self.context.dead_time_table([19489])

        self.assertEqual(deadtime_table, 'deadtime_table_name')

    def test_get_workspace_names_returns_all_stored_workspaces_if_all_selected(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit('19489', 'fwd, bwd, long', True)

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA', 'EMU19489; PhaseQuad; PhaseTable EMU19489']))

    def test_get_workspace_names_returns_nothing_if_no_parameters_passed(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit()

        self.assertEqual(workspace_list, [])

    def test_get_workspaces_names_copes_with_bad_groups(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit('19489', 'fwd, bwd, long, random, wrong', True)

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA', 'EMU19489; PhaseQuad; PhaseTable EMU19489']))

    def test_get_workspaces_names_copes_with_non_existent_runs(self):
        self.populate_ADS()

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489, 22222', 'fwd, bwd, long', True)

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA', 'EMU19489; PhaseQuad; PhaseTable EMU19489']))

    def test_that_run_ranged_correctly_parsed(self):
        self.populate_ADS()

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489-95', 'fwd, bwd, long',
                                                                     True)

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA', 'EMU19489; PhaseQuad; PhaseTable EMU19489']))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

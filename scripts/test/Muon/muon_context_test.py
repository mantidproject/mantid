# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.calculate_pair_and_group import run_pre_processing
from mantidqt.utils.qt.testing import start_qapplication
from unittest import mock

from mantid.api import AnalysisDataService, FileFinder
from mantid import ConfigService
from mantid.simpleapi import CreateWorkspace
from collections import Counter
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


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
        self.group_pair_context.reset_group_and_pairs_to_default(self.load_result['OutputWorkspace'][0].workspace,
                                                                 'EMU', '', 1)

    def tearDown(self):
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'False'
        self.context.ads_observer.unsubscribe()

    def populate_ADS(self):
        self.context.calculate_all_groups()
        self.context.show_all_groups()
        self.context.calculate_all_pairs()
        self.context.show_all_pairs()
        CreateWorkspace([0], [0], OutputWorkspace='EMU19489; PhaseQuad; PhaseTable EMU19489')
        self.context.phase_context.add_phase_quad(
            MuonWorkspaceWrapper('EMU19489; PhaseQuad; PhaseTable EMU19489'), '19489')

    def _assert_list_in_ADS(self, workspace_name_list):
        ads_list = AnalysisDataService.getObjectNames()
        for item in workspace_name_list:
            self.assertTrue(item in ads_list)

    def test_window(self):
        self.assertEquals("Muon Analysis", self.context.window_title)

    def test_get_runs(self):
        runs = self.context.get_runs(" 19489 ")
        self.assertEquals(runs, [[19489]])

    def test_get_group_or_pair(self):
        group_and_pair = self.context.get_group_and_pair("All")
        self.assertEquals(group_and_pair, (["fwd", "bwd"], ["long"]))

    def test_get_group(self):
        group_and_pair = self.context.get_group_and_pair(" fwd , bwd ")
        self.assertEquals(group_and_pair, (["fwd", "bwd"], []))

    def test_get_pair(self):
        group_and_pair = self.context.get_group_and_pair(" long ")
        self.assertEquals(group_and_pair, ([], ["long"]))

    def test_reset_groups_and_pairs_to_default(self):
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])
        self.assertEqual(self.group_pair_context.pair_names, ['long'])

    def test_calculate_group_calculates_group_for_given_run(self):
        # Generate the pre_process_data workspace
        run_pre_processing(self.context, [self.run_number], rebin=False)
        counts_workspace, asymmetry_workspace, group_asymmetry_unormalised = self.context.calculate_group(MuonGroup('fwd'),
                                                                                                          run=[19489])

        self.assertEqual(counts_workspace, 'EMU19489; Group; fwd; Counts; MA')
        self.assertEqual(asymmetry_workspace, 'EMU19489; Group; fwd; Asymmetry; MA')
        self.assertEqual(group_asymmetry_unormalised, '__EMU19489; Group; fwd; Asymmetry; MA_unnorm')

    def test_calculate_group_with_no_relevant_periods_returns_none(self):
        # Generate the pre_process_data workspace
        run_pre_processing(self.context, [self.run_number], rebin=False)
        counts_workspace, asymmetry_workspace, group_asymmetry_unormalised = self.context.calculate_group(MuonGroup('fwd', periods=(3,4)),
                                                                                                          run=[19489])

        self.assertEqual(counts_workspace, None)
        self.assertEqual(asymmetry_workspace, None)
        self.assertEqual(group_asymmetry_unormalised, None)

    def test_calculate_pair_calculates_pair_for_given_run(self):
        self.context.show_all_groups()
        long = MuonPair('long', 'fwd', 'bwd')
        pair_asymmetry = self.context.calculate_pair(long, [19489], False)

        self.assertEqual(pair_asymmetry, 'EMU19489; Pair Asym; long; MA')

    def test_calculate_pair_returns_nothing_if_relevant_groups_do_not_exist(self):
        self.context.show_all_groups()
        long = MuonPair('long', 'fwd', 'bwd')
        pair_asymmetry = self.context.calculate_pair(long, [19489], True)

        self.assertEqual(pair_asymmetry, None)

    def test_show_all_groups_calculates_and_shows_all_groups(self):
        self.context.show_all_groups()

        self._assert_list_in_ADS(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                  'EMU19489 MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; fwd; Asymmetry; MA',
                                  'EMU19489; Group; fwd; Counts; MA'])

    def test_that_show_all_calculates_and_shows_all_groups_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_groups()

        self._assert_list_in_ADS(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; bwd; Asymmetry; Rebin; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; Rebin; MA_unnorm',
                                  'EMU19489 MA',
                                  'EMU19489; Group; bwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; Rebin; MA',
                                  'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; bwd; Counts; Rebin; MA',
                                  'EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; fwd; Asymmetry; Rebin; MA',
                                  'EMU19489; Group; fwd; Counts; MA', 'EMU19489; Group; fwd; Counts; Rebin; MA'])

    def test_show_all_pairs_calculates_and_shows_all_pairs(self):
        self.context.show_all_groups()
        self.context.show_all_pairs()

        self._assert_list_in_ADS(['EMU19489 MA', 'EMU19489; Pair Asym; long; MA'])

    def test_that_show_all_calculates_and_shows_all_pairs_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_groups()
        self.context.show_all_pairs()

        self._assert_list_in_ADS(['EMU19489 MA', 'EMU19489; Pair Asym; long; MA',
                                  'EMU19489; Pair Asym; long; Rebin; MA'])

    def test_update_current_data_sets_current_run_in_data_context(self):
        self.context.update_current_data()

        self.assertEqual(self.data_context.current_data, self.load_result)

    def test_update_current_data_sets_groups_and_pairs(self):
        self.context.update_current_data()

        self.assertEqual(self.group_pair_context.pair_names, ['long'])
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])

    def test_show_raw_data_puts_raw_data_into_the_ADS(self):
        self.context.show_raw_data()

        self._assert_list_in_ADS(['EMU19489 MA', 'EMU19489_raw_data MA'])

    def test_that_first_good_data_returns_correctly_when_from_file_chosen_option(self):
        self.gui_context.update({'FirstGoodDataFromFile': True})

        first_good_data = self.context.first_good_data([19489])

        self.assertEqual(first_good_data, 0.113)

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

    def test_calculate_all_pairs(self):
        self.context._calculate_pairs = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=False)

        self.context.calculate_all_pairs()
        self.context._calculate_pairs.assert_called_with(rebin=False)
        self.assertEqual(self.context._calculate_pairs.call_count,1)

    def test_calculate_all_groups(self):
        self.context._calculate_groups = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=False)

        self.context.calculate_all_groups()
        self.context._calculate_groups.assert_called_with(rebin=False)
        self.assertEqual(self.context._calculate_groups.call_count,1)

    def test_calculate_all_pairs_rebin(self):
        self.context._calculate_pairs = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=True)

        self.context.calculate_all_pairs()
        self.context._calculate_pairs.assert_any_call(rebin=False)
        self.context._calculate_pairs.assert_called_with(rebin=True)
        self.assertEqual(self.context._calculate_pairs.call_count,2)

    def test_calculate_all_groups_rebin(self):
        self.context._calculate_groups = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=True)

        self.context.calculate_all_groups()
        self.context._calculate_groups.assert_any_call(rebin=False)
        self.context._calculate_groups.assert_called_with(rebin=True)
        self.assertEqual(self.context._calculate_groups.call_count,2)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

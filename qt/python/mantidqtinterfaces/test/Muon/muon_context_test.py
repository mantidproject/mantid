# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from collections import Counter

from mantid.api import AnalysisDataService, FileFinder
from mantid import ConfigService

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import create_empty_table, run_create_workspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Muon.GUI.Common.calculate_pair_and_group import run_pre_processing
from mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.muon_base_pair import MuonBasePair
from mantidqtinterfaces.Muon.GUI.Common.muon_diff import MuonDiff
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.muon_phasequad import MuonPhasequad


def run_side_effect(param, name):
    return name


def rebin_side_effect(name, rebin):
    return name


def return_list(name):
    return [name+"1", name+"2"]


def crop_side_effect(name, xmin, xmax):
    return name


def average_side_effect(name, dt):
    return name


def convert_side_effect(name):
    return name


def divide_side_effect(LHS, RHS, name):
    return name


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

        self.run_list = [19489]
        self.groups = [MuonGroup("bwd"), MuonGroup("fwd")]
        self.rebins = [False, False]
        self.pairs = [MuonPair("long", "bwd", "fwd")]

    def tearDown(self):
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'False'
        self.context.ads_observer.unsubscribe()

    def add_group_diff(self):
        diff = MuonDiff('group_diff', 'fwd', 'bwd')
        self.group_pair_context.add_diff(diff)
        return diff

    def add_pair_diff(self):
        long2 = MuonPair('long2', 'bwd', 'fwd')
        self.group_pair_context.add_pair(long2)
        diff = MuonDiff('pair_diff', 'long', 'long2', 'pair')
        self.group_pair_context.add_diff(diff)
        return diff

    def _calculate_data_for(self, run_list: list, groups: list = [], rebins: list = [], pairs: list = [], diffs: list = []):
        self.context.calculate_all_counts()
        for group, rebin in zip(groups, rebins):
            self.context.calculate_asymmetry_for(run_list, group, rebin)
            self.context.show_group(run_list, group, rebin)
        for pair in pairs:
            self.context.calculate_pair_for(run_list, pair)
            self.context.show_pair(run_list, pair)
        for diff in diffs:
            self.context.calculate_diff_for(run_list, diff)
            self.context.show_diff(run_list, diff)

    def _assert_list_in_ADS(self, workspace_name_list):
        ads_list = AnalysisDataService.getObjectNames()
        for item in workspace_name_list:
            self.assertTrue(item in ads_list)

    def test_window(self):
        self.assertEqual("Muon Analysis", self.context.window_title)

    def test_get_runs(self):
        runs = self.context.get_runs(" 19489 ")
        self.assertEqual(runs, [[19489]])

    def test_get_group_or_pair(self):
        group_and_pair = self.context.get_group_and_pair("All")
        self.assertEqual(group_and_pair, (["fwd", "bwd"], ["long"]))

    def test_get_group(self):
        group_and_pair = self.context.get_group_and_pair(" fwd , bwd ")
        self.assertEqual(group_and_pair, (["fwd", "bwd"], []))

    def test_get_pair(self):
        group_and_pair = self.context.get_group_and_pair(" long ")
        self.assertEqual(group_and_pair, ([], ["long"]))

    def test_reset_groups_and_pairs_to_default(self):
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])
        self.assertEqual(self.group_pair_context.pair_names, ['long'])

    def test_calculate_group_calculates_group_for_given_run(self):
        # Generate the pre_process_data workspace
        run_pre_processing(self.context, [self.run_number], rebin=False)

        group = MuonGroup('fwd')
        counts_workspace = self.context.calculate_counts(self.run_list, group)
        asymmetry_workspace, group_asymmetry_unormalised = self.context.calculate_asymmetry(self.run_list, group)

        self.assertEqual(counts_workspace, 'EMU19489; Group; fwd; Counts; MA')
        self.assertEqual(asymmetry_workspace, 'EMU19489; Group; fwd; Asymmetry; MA')
        self.assertEqual(group_asymmetry_unormalised, '__EMU19489; Group; fwd; Asymmetry; MA_unnorm')

    def test_calculate_group_with_no_relevant_periods_returns_none(self):
        # Generate the pre_process_data workspace
        run_pre_processing(self.context, [self.run_number], rebin=False)

        group = MuonGroup('fwd', periods=(3, 4))
        counts_workspace = self.context.calculate_counts(self.run_list, group)
        asymmetry_workspaces = self.context.calculate_asymmetry(self.run_list, group)

        self.assertEqual(counts_workspace, None)
        self.assertEqual(asymmetry_workspaces, None)

    def test_calculate_pair_calculates_pair_for_given_run(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins)

        long = MuonPair('long', 'fwd', 'bwd')
        pair_asymmetry = self.context.calculate_pair(long, self.run_list, False)

        self.assertEqual(pair_asymmetry, 'EMU19489; Pair Asym; long; MA')

    def test_calculate_pair_returns_nothing_if_relevant_groups_do_not_exist(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins)

        long = MuonPair('long', 'fwd', 'bwd')
        pair_asymmetry = self.context.calculate_pair(long, self.run_list, True)

        self.assertEqual(pair_asymmetry, None)

    def test_calculate_group_diff_calculates_diff_for_given_run(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins)

        diff = MuonDiff('diff', 'fwd', 'bwd')
        diff_asymmetry = self.context.calculate_diff(diff, self.run_list, False)

        self.assertEqual(diff_asymmetry, 'EMU19489; Diff; diff; Asymmetry; MA')

    def test_calculate_pair_diff_calculates_diff_for_given_run(self):
        diff = self.add_pair_diff()

        pairs = [MuonPair("long", "bwd", "fwd"), MuonPair("long2", "bwd", "fwd")]
        diffs = [diff]

        self._calculate_data_for(self.run_list, self.groups, self.rebins, pairs, diffs)

        self._assert_list_in_ADS(['EMU19489; Diff; pair_diff; Asymmetry; MA'])

    def test_calculate_group_diff_returns_nothing_if_relevant_groups_do_not_exist(self):
        self.context.calculate_all_counts()
        diff = MuonDiff('diff', 'fwd', 'bwd')
        diff_asymmetry = self.context.calculate_diff(diff, [19489], True)

        self.assertEqual(diff_asymmetry, None)

    def test_calculate_pair_diff_returns_nothing_if_relevant_pairs_do_not_exist(self):
        diff = self.add_pair_diff()
        self.context.calculate_all_counts()
        diff_asymmetry = self.context.calculate_diff(diff, [19489], True)
        self.assertEqual(diff_asymmetry, None)

    def test_all_groups_are_calculated_as_expected(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins)

        self._assert_list_in_ADS(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                  'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; fwd; Asymmetry; MA',
                                  'EMU19489; Group; fwd; Counts; MA'])

    def test_that_show_all_calculates_and_shows_all_groups_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        groups = [MuonGroup("bwd"), MuonGroup("bwd"), MuonGroup("fwd"), MuonGroup("fwd")]
        rebins = [False, True, False, True]

        self._calculate_data_for(self.run_list, groups, rebins)

        self._assert_list_in_ADS(['__EMU19489; Group; bwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; bwd; Asymmetry; Rebin; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; MA_unnorm',
                                  '__EMU19489; Group; fwd; Asymmetry; Rebin; MA_unnorm',
                                  'EMU19489; Group; bwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; Rebin; MA',
                                  'EMU19489; Group; bwd; Counts; MA', 'EMU19489; Group; bwd; Counts; Rebin; MA',
                                  'EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; fwd; Asymmetry; Rebin; MA',
                                  'EMU19489; Group; fwd; Counts; MA', 'EMU19489; Group; fwd; Counts; Rebin; MA'])

    def test_show_all_pairs_calculates_and_shows_all_pairs(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        self._assert_list_in_ADS(['EMU19489; Pair Asym; long; MA'])

    def test_that_show_all_calculates_and_shows_all_pairs_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        self._assert_list_in_ADS(['EMU19489; Pair Asym; long; MA',
                                  'EMU19489; Pair Asym; long; Rebin; MA'])

    def test_that_show_all_calculates_and_shows_all_diffs(self):
        group_diff = self.add_group_diff()
        pair_diff = self.add_pair_diff()

        pairs = [MuonPair("long", "bwd", "fwd"), MuonPair("long2", "bwd", "fwd")]
        diffs = [group_diff, pair_diff]
        self._calculate_data_for(self.run_list, self.groups, self.rebins, pairs, diffs)

        self._assert_list_in_ADS(['EMU19489; Diff; group_diff; Asymmetry; MA',
                                  'EMU19489; Diff; pair_diff; Asymmetry; MA'])

    def test_that_show_all_calculates_and_shows_all_diffs_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        groups = [MuonGroup("bwd"), MuonGroup("bwd"), MuonGroup("fwd"), MuonGroup("fwd")]
        rebins = [False, True, False, True]

        group_diff = self.add_group_diff()
        pair_diff = self.add_pair_diff()

        pairs = [MuonPair("long", "bwd", "fwd"), MuonPair("long2", "bwd", "fwd")]
        diffs = [group_diff, pair_diff]
        self._calculate_data_for(self.run_list, groups, rebins, pairs, diffs)

        self._assert_list_in_ADS(['EMU19489; Diff; group_diff; Asymmetry; MA',
                                  'EMU19489; Diff; group_diff; Asymmetry; Rebin; MA',
                                  'EMU19489; Diff; pair_diff; Asymmetry; MA',
                                  'EMU19489; Diff; pair_diff; Asymmetry; Rebin; MA'])

    def test_update_current_data_sets_current_run_in_data_context(self):
        self.context.update_current_data()

        self.assertEqual(self.data_context.current_data, self.load_result)

    def test_update_current_data_sets_groups_and_pairs(self):
        self.context.update_current_data()

        self.assertEqual(self.group_pair_context.pair_names, ['long'])
        self.assertEqual(self.group_pair_context.group_names, ['fwd', 'bwd'])

    def test_show_raw_data_puts_raw_data_into_the_ADS(self):
        self.context.show_raw_data()

        self._assert_list_in_ADS(['EMU19489_raw_data MA'])

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
        self.context.corrections_context.dead_time_source = "FromADS"
        self.context.corrections_context.dead_time_table_name_from_ads = "deadtime_table_name"

        deadtime_table = self.context.corrections_context.current_dead_time_table_name_for_run(
            self.context.data_context.instrument, [19489])

        self.assertEqual(deadtime_table, 'deadtime_table_name')

    def test_get_workspace_names_returns_all_stored_workspaces_if_all_selected(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489', 'fwd, bwd, long')

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA']))

    def test_get_workspace_names_returns_nothing_if_no_parameters_passed(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins)
        workspace_list = self.context.get_names_of_workspaces_to_fit()

        self.assertEqual(workspace_list, [])

    def test_get_workspaces_names_copes_with_bad_groups(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489', 'fwd, bwd, long, random, wrong')

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA']))

    def test_get_workspaces_names_copes_with_non_existent_runs(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489, 22222', 'fwd, bwd, long')

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA']))

    def test_that_run_ranged_correctly_parsed(self):
        self._calculate_data_for(self.run_list, self.groups, self.rebins, self.pairs)

        workspace_list = self.context.get_names_of_workspaces_to_fit('19489-95', 'fwd, bwd, long')

        self.assertEqual(Counter(workspace_list),
                         Counter(['EMU19489; Group; fwd; Asymmetry; MA', 'EMU19489; Group; bwd; Asymmetry; MA',
                                  'EMU19489; Pair Asym; long; MA']))

    def test_that_find_pairs_containing_groups_will_return_an_empty_list_if_the_provided_group_is_not_in_a_pair(self):
        groups = ["top"]
        pairs = self.context.find_pairs_containing_groups(groups)

        self.assertEqual(len(pairs), 0)

    def test_that_find_pairs_containing_groups_will_return_pairs_containing_the_provided_group(self):
        groups = ["bwd"]
        pairs = self.context.find_pairs_containing_groups(groups)

        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0].name, "long")

    def test_that_find_diffs_containing_groups_or_pairs_will_return_an_empty_list_if_the_provided_group_is_not_in_a_pair(self):
        groups = ["top"]
        diffs = self.context.find_pairs_containing_groups(groups)

        self.assertEqual(len(diffs), 0)

    def test_that_find_pairs_containing_groups_does_not_raise_if_one_of_the_pairs_is_a_MuonBasePair(self):
        self.group_pair_context.add_pair(MuonBasePair("PhaseQuad_Re_"))
        groups = ["bwd"]
        pairs = self.context.find_pairs_containing_groups(groups)

        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0].name, "long")

    def test_that_find_diffs_containing_groups_or_pairs_will_return_diffs_containing_the_provided_pair(self):
        pair_diff = self.add_pair_diff()

        groups = ["long"]
        diffs = self.context.find_diffs_containing_groups_or_pairs(groups)

        self.assertEqual(len(diffs), 1)
        self.assertEqual(diffs[0], pair_diff)

    def test_that_find_diffs_containing_groups_or_pairs_will_return_an_empty_list_if_the_provided_group_is_not_in_a_diff(self):
        self.add_group_diff()

        groups = ["top"]
        diffs = self.context.find_diffs_containing_groups_or_pairs(groups)

        self.assertEqual(len(diffs), 0)

    def test_calculate_all_counts(self):
        self.context._calculate_all_counts = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=False)

        self.context.calculate_all_counts()
        self.context._calculate_all_counts.assert_called_with(rebin=False)
        self.assertEqual(self.context._calculate_all_counts.call_count, 1)

    def test_calculate_all_counts_rebin(self):
        self.context._calculate_all_counts = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=True)

        self.context.calculate_all_counts()
        self.context._calculate_all_counts.assert_any_call(rebin=False)
        self.context._calculate_all_counts.assert_called_with(rebin=True)
        self.assertEqual(self.context._calculate_all_counts.call_count, 2)

    def test_that_calculate_pair_for_calls_the_expected_methods(self):
        self.context._calculate_pair_for = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=False)

        pair = MuonPair("long", "fwd", "bwd")
        self.context.calculate_pair_for(self.run_list, pair)
        self.context._calculate_pair_for.assert_called_with(self.run_list, pair, rebin=False)
        self.assertEqual(self.context._calculate_pair_for.call_count, 1)

    def test_that_calculate_pair_for_calls_the_expected_methods_for_rebin(self):
        self.context._calculate_pair_for = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=True)

        pair = MuonPair("long", "fwd", "bwd")
        self.context.calculate_pair_for(self.run_list, pair)
        self.context._calculate_pair_for.assert_any_call(self.run_list, pair, rebin=False)
        self.context._calculate_pair_for.assert_called_with(self.run_list, pair, rebin=True)
        self.assertEqual(self.context._calculate_pair_for.call_count, 2)

    def test_that_calculate_diff_for_calls_the_expected_methods(self):
        self.context._calculate_diff_for = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=False)

        diff = self.add_group_diff()

        self.context.calculate_diff_for(self.run_list, diff)
        self.context._calculate_diff_for.assert_called_with(self.run_list, diff, rebin=False)
        self.assertEqual(self.context._calculate_diff_for.call_count, 1)

    def test_that_calculate_diff_for_calls_the_expected_methods_for_rebin(self):
        self.context._calculate_diff_for = mock.Mock()
        self.context._do_rebin = mock.Mock(return_value=True)

        diff = self.add_group_diff()

        self.context.calculate_diff_for(self.run_list, diff)
        self.context._calculate_diff_for.assert_any_call(self.run_list, diff, rebin=False)
        self.context._calculate_diff_for.assert_called_with(self.run_list, diff, rebin=True)
        self.assertEqual(self.context._calculate_diff_for.call_count, 2)

    def test_update_phasequads(self):
        phasequad = MuonPhasequad("test", "test_table")
        self.context.group_pair_context.add_phasequad(phasequad)
        self.context._calculate_phasequads = mock.Mock()
        self.assertEqual(["long", "test_Re_", "test_Im_"], self.context.group_pair_context.pair_names)
        self.assertEqual("test", self.context.group_pair_context._phasequad[0].name)
        self.assertEqual(1, len(self.context.group_pair_context._phasequad))

        self.context.update_phasequads()

        self.assertEqual(["long", "test_Re_", "test_Im_"], self.context.group_pair_context.pair_names)
        self.assertEqual("test", self.context.group_pair_context._phasequad[0].name)
        self.assertEqual(1, len(self.context.group_pair_context._phasequad))
        self.assertEqual(1, self.context._calculate_phasequads.call_count)

    def test_calculate_phasequads(self):
        self.context._calculate_phasequads = mock.Mock()
        self.context._run_deadtime = mock.Mock(side_effect=run_side_effect)
        self.context._do_rebin = mock.Mock(return_value=False)
        self.context.calculate_phasequads(mock.Mock())
        self.assertEqual(1, self.context._calculate_phasequads.call_count)

        self.context._do_rebin.return_value=True
        self.context.calculate_phasequads(mock.Mock())
        # 2 + 1
        self.assertEqual(3, self.context._calculate_phasequads.call_count)

    def set_up_phasequad_rebin_mock(self):
        self.context._get_bin_width = mock.Mock(return_value = 0.1)
        self.context._average_by_bin_widths = mock.Mock(side_effect=average_side_effect)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_PhaseQuad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.split_phasequad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_crop_workspace')
    def test_calculate_phasequad(self, crop_mock, split_mock, run_mock):
        table = "test_table"
        phasequad = MuonPhasequad("test", table)
        name = "EMU5234; PhaseQuad; test_Re__Im_; MA"
        self.context._run_deadtime = mock.Mock(return_value=name)
        self.context.data_context._current_runs = [5234]
        run_mock.side_effect = run_side_effect
        self.context._run_rebin = mock.Mock(side_effect=rebin_side_effect)
        split_mock.side_effect = return_list
        crop_mock.side_effect = crop_side_effect
        self.set_up_phasequad_rebin_mock()

        result = self.context.calculate_phasequad(phasequad, 5234, False)
        # names are wrong due to split mock
        self.assertEqual(result, [name+"1", name+"2"])
        self.context._run_rebin.assert_not_called()
        run_mock.assert_called_with({"PhaseTable": table, 'InputWorkspace': name}, name)
        split_mock.assert_called_with(name)
        crop_mock.assert_called_with(name, 0.0, 0.0)
        self.context._get_bin_width.assert_not_called()
        self.context._average_by_bin_widths.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_PhaseQuad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.split_phasequad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_crop_workspace')
    def test_calculate_phasequad_rebin(self, crop_mock, split_mock, run_mock):
        table = "test_table"
        name = "EMU5234; PhaseQuad; test_Re__Im_; Rebin; MA"
        self.context._run_deadtime = mock.Mock(return_value=name)
        self.context.data_context._current_runs = [5234]
        phasequad = MuonPhasequad("test", table)
        run_mock.side_effect = run_side_effect
        self.context._run_rebin = mock.Mock(side_effect=rebin_side_effect)
        split_mock.side_effect = return_list
        crop_mock.side_effect = crop_side_effect
        self.set_up_phasequad_rebin_mock()

        result = self.context.calculate_phasequad(phasequad, 5234, True)
        # names are wrong due to split mock
        self.assertEqual(result, [name+"1", name+"2"])
        self.context._run_rebin.assert_called_with(name, True)
        run_mock.assert_called_with({"PhaseTable": table, 'InputWorkspace': name}, name)
        split_mock.assert_called_with(name)
        crop_mock.assert_called_with(name, 0.0, 0.0)
        self.context._get_bin_width.assert_called_once_with(name)
        self.context._average_by_bin_widths.assert_called_once_with(name,0.1)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_PhaseQuad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.split_phasequad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_crop_workspace')
    def test_calculate_phasequad_deadtime(self, crop_mock, split_mock, run_mock):
        table = "test_table"
        phasequad = MuonPhasequad("test",table)
        self.context._run_deadtime = mock.Mock(side_effect=run_side_effect)
        self.context.data_context._current_runs = [5234]
        run_mock.side_effect = run_side_effect
        self.context._run_rebin = mock.Mock(side_effect=rebin_side_effect)
        split_mock.side_effect = return_list
        crop_mock.side_effect = crop_side_effect
        self.set_up_phasequad_rebin_mock()

        result = self.context.calculate_phasequad(phasequad, 5234, False)
        # names are wrong due to split mock
        name = "EMU5234; PhaseQuad; test_Re__Im_; MA"
        self.assertEqual(result, [name+"1", name+"2"])
        self.context._run_rebin.assert_not_called()
        run_mock.assert_called_with({"PhaseTable": table, 'InputWorkspace': name}, name)
        split_mock.assert_called_with(name)
        crop_mock.asser_called_with(name, 0.0, 0.0)
        self.context._get_bin_width.assert_not_called()
        self.context._average_by_bin_widths.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_PhaseQuad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.split_phasequad')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_crop_workspace')
    def test_calculate_phasequad_rebin_deadtime(self, crop_mock, split_mock, run_mock):
        table = "test_table"
        self.context._run_deadtime = mock.Mock(side_effect=run_side_effect)
        self.context.data_context._current_runs = [5234]
        phasequad = MuonPhasequad("test", table)
        run_mock.side_effect = run_side_effect
        self.context._run_rebin = mock.Mock(side_effect=rebin_side_effect)
        split_mock.side_effect = return_list
        crop_mock.side_effect = crop_side_effect
        self.set_up_phasequad_rebin_mock()

        result = self.context.calculate_phasequad(phasequad, 5234, True)
        # names are wrong due to split mock
        name = "EMU5234; PhaseQuad; test_Re__Im_; Rebin; MA"
        self.assertEqual(result, [name+"1", name+"2"])
        self.context._run_rebin.assert_called_with(name, True)
        run_mock.assert_called_with({"PhaseTable": table, 'InputWorkspace': name}, name)
        split_mock.assert_called_with(name)
        crop_mock.assert_called_with(name, 0.0, 0.0)
        self.context._get_bin_width.assert_called_once_with(name)
        self.context._average_by_bin_widths.assert_called_once_with(name,0.1)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_convert_to_histogram')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_convert_to_points')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_create_workspace')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.run_divide')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.delete_ws')
    def test_average_by_bin_widths(self, delete, divide, create, points, histo):
        self.context._get_x_data = mock.Mock(return_value = [0,1,3,4,6])
        divide.side_effect = divide_side_effect
        name = "test"
        tmp = "tmp"
        create.return_value = tmp
        points.side_effect = convert_side_effect
        histo.side_effect = convert_side_effect

        self.context._average_by_bin_widths(name, .5)
        histo.assert_called_once_with(name)
        create.assert_called_once_with([0,1,3,4,6], [2, 4, 2, 4], tmp)
        self.assertEqual(points.call_count, 2)
        points.assert_any_call(name)
        points.assert_any_call(tmp)
        divide.assert_called_once_with(name, tmp, name)
        delete.assert_called_once_with(tmp)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.get_raw_data_workspace_name')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.apply_deadtime')
    def test_run_deadtime_false(self, apply_mock, name_mock):
        name = "EMU5234; PhaseQuad; test_Re__Im_; Rebin; MA"
        raw_name = 'EMU5234_raw_data MA'
        name_mock.return_value = raw_name
        self.context.dead_time_table = mock.Mock(return_value = None)

        result = self.context._run_deadtime("5234", name)
        self.assertEqual(0, apply_mock.call_count)
        self.assertEqual(result, raw_name)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.get_raw_data_workspace_name')
    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.apply_deadtime')
    def test_run_deadtime_true(self, apply_mock, name_mock):
        name = "EMU5234; PhaseQuad; test_Re__Im_; Rebin; MA"
        raw_name = 'EMU5234_raw_data MA'
        name_mock.return_value = raw_name
        self.context.corrections_context.dead_time_source = "FromFile"
        self.context.corrections_context.get_default_dead_time_table_name_for_run = mock.Mock(return_value = "table")
        apply_mock.return_value = name

        result = self.context._run_deadtime("5234", name)
        apply_mock.assert_called_with(raw_name, name, "table")
        self.assertEqual(result, name)

    def test_multi_period_phasequad(self):
        self.context._data_context.num_periods = mock.Mock(return_value=4)
        self.assertRaises(ValueError, self.context._calculate_phasequads, mock.Mock(), True)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context.add_to_group')
    def test_do_grouping(self, group_mock):
        self.context.do_grouping()
        group_mock.assert_called_once_with(self.data_context.instrument, self.context.workspace_suffix)

    def test_remove_workspace_str(self):
        self.context.data_context.remove_workspace_by_name = mock.Mock()
        self.context.group_pair_context.remove_workspace_by_name = mock.Mock()
        self.context.phase_context.remove_workspace_by_name = mock.Mock()
        self.context.fitting_context.remove_workspace_by_name = mock.Mock()
        self.context.update_view_from_model_notifier.notify_subscribers = mock.Mock()
        self.context.deleted_plots_notifier.notify_subscribers = mock.Mock()
        name = "test"

        self.context.remove_workspace(name)

        self.context.data_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.group_pair_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.phase_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.fitting_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.update_view_from_model_notifier.notify_subscribers.assert_called_once_with(name)
        self.context.deleted_plots_notifier.notify_subscribers.assert_called_once_with(name)

    def test_remove_workspace_ws(self):
        self.context.data_context.remove_workspace_by_name = mock.Mock()
        self.context.group_pair_context.remove_workspace_by_name = mock.Mock()
        self.context.phase_context.remove_workspace_by_name = mock.Mock()
        self.context.fitting_context.remove_workspace_by_name = mock.Mock()
        self.context.update_view_from_model_notifier.notify_subscribers = mock.Mock()
        self.context.deleted_plots_notifier.notify_subscribers = mock.Mock()

        name = run_create_workspace([0,1], [0,1], "test")
        ws = retrieve_ws("test")

        self.context.remove_workspace(ws)

        self.context.data_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.group_pair_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.phase_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.fitting_context.remove_workspace_by_name.assert_called_once_with(name)
        self.context.update_view_from_model_notifier.notify_subscribers.assert_called_once_with(name)
        self.context.deleted_plots_notifier.notify_subscribers.assert_called_once_with(ws)

    def test_remove_workspace_table(self):
        self.context.data_context.remove_workspace_by_name = mock.Mock()
        self.context.group_pair_context.remove_workspace_by_name = mock.Mock()
        self.context.phase_context.remove_workspace_by_name = mock.Mock()
        self.context.fitting_context.remove_workspace_by_name = mock.Mock()
        self.context.update_view_from_model_notifier.notify_subscribers = mock.Mock()
        self.context.deleted_plots_notifier.notify_subscribers = mock.Mock()

        ws = create_empty_table("test")

        self.context.remove_workspace(ws)

        self.context.data_context.remove_workspace_by_name.assert_not_called()
        self.context.group_pair_context.remove_workspace_by_name.assert_not_called()
        self.context.phase_context.remove_workspace_by_name.assert_not_called()
        self.context.fitting_context.remove_workspace_by_name.assert_not_called()
        self.context.update_view_from_model_notifier.notify_subscribers.assert_not_called()
        self.context.deleted_plots_notifier.notify_subscribers.assert_not_called()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

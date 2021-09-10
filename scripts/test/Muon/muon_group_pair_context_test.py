# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.muon_base import MuonRun
from Muon.GUI.Common.muon_diff import MuonDiff
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_phasequad import MuonPhasequad
from Muon.GUI.Common.test_helpers.general_test_helpers import create_group_populated_by_two_workspace
from mantid.simpleapi import CreateSampleWorkspace, LoadInstrument


class MuonGroupPairContextTest(unittest.TestCase):
    def setUp(self):
        self.context = MuonGroupPairContext()

    def test_can_be_created(self):
        self.assertTrue(self.context)

    def test_groups_and_pairs_initially_empty(self):
        self.assertEqual(self.context.groups, [])
        self.assertEqual(self.context.pairs, [])
        self.assertEqual(self.context.diffs, [])

    def test_group_can_be_added(self):
        group = MuonGroup('group_1', [1,3,5,7,9])

        self.context.add_group(group)

        self.assertEqual(self.context['group_1'], group)

    def test_non_group_cannot_be_added(self):
        pair = MuonPair('pair_1')
        diff = MuonDiff('diff_1', 'positive', 'negative')

        self.assertRaises(AssertionError, self.context.add_group, pair)
        self.assertRaises(AssertionError, self.context.add_group, diff)

    def test_cannot_add_group_with_duplicate_name(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_1', [1, 3, 5, 7, 9])

        self.context.add_group(group_1)

        self.assertRaises(ValueError, self.context.add_group, group_2)

    def test_pair_can_be_added(self):
        pair = MuonPair('pair_1')

        self.context.add_pair(pair)

        self.assertEqual(self.context['pair_1'], pair)

    def test_non_pair_cannot_be_added(self):
        group = MuonGroup('group_1', [1, 3, 5, 7, 9])
        diff = MuonDiff('diff_1', 'positive', 'negative')

        # Value error as cannot assert isinstance in code since could add MuonPair or MuonBasePair
        self.assertRaises(ValueError, self.context.add_pair, group)
        self.assertRaises(ValueError, self.context.add_pair, diff)

    def test_cannot_add_pair_with_duplicate_name(self):
        pair_1 = MuonPair('pair')
        pair_2 = MuonPair('pair')

        self.context.add_pair(pair_1)

        self.assertRaises(ValueError, self.context.add_pair, pair_2)

    def test_diff_can_be_added(self):
        diff = MuonDiff('diff_1', 'positive', 'negative')

        self.context.add_diff(diff)

        self.assertEqual(self.context['diff_1'], diff)

    def test_non_diff_cannot_be_added(self):
        group = MuonGroup('group_1', [1, 3, 5, 7, 9])
        pair = MuonPair('pair_1')

        self.assertRaises(AssertionError, self.context.add_diff, group)
        self.assertRaises(AssertionError, self.context.add_diff, pair)

    def test_cannot_add_diff_with_duplicate_name(self):
        diff_1 = MuonDiff('diff', 'positive', 'negative')
        diff_2 = MuonDiff('diff', 'positive', 'negative')

        self.context.add_diff(diff_1)

        self.assertRaises(ValueError, self.context.add_diff, diff_2)

    def test_groups_pairs_diffs_should_all_have_unique_names(self):
        group_1 = MuonGroup('group', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('pair', [1, 3, 5, 7, 9])
        group_3 = MuonGroup('diff', [1, 3, 5, 7, 9])
        pair_1 = MuonPair('pair')
        pair_2 = MuonPair('group')
        pair_3 = MuonPair('diff')
        diff_1 = MuonDiff('diff', 'positive', 'negative')
        diff_2 = MuonDiff('group', 'positive', 'negative')
        diff_3 = MuonDiff('pair', 'positive', 'negative')

        # Add correct group, pair and diff
        self.context.add_group(group_1)
        self.context.add_pair(pair_1)
        self.context.add_diff(diff_1)

        # Now check cannot duplicate names
        self.assertRaises(ValueError, self.context.add_group, group_2)
        self.assertRaises(ValueError, self.context.add_group, group_3)
        self.assertRaises(ValueError, self.context.add_pair, pair_2)
        self.assertRaises(ValueError, self.context.add_pair, pair_3)
        self.assertRaises(ValueError, self.context.add_diff, diff_2)
        self.assertRaises(ValueError, self.context.add_diff, diff_3)

    def test_group_names_returns_ordered_list_of_names(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])

        self.context.add_group(group_1)
        self.context.add_group(group_3)
        self.context.add_group(group_2)

        self.assertEqual(self.context.group_names, ['group_1', 'group_3', 'group_2'])

    def test_pair_names_returns_ordered_list_of_names(self):
        pair_1 = MuonPair('pair_1')
        pair_2 = MuonPair('pair_2')
        pair_3 = MuonPair('pair_3')

        self.context.add_pair(pair_1)
        self.context.add_pair(pair_2)
        self.context.add_pair(pair_3)

        self.assertEqual(self.context.pair_names, ['pair_1', 'pair_2', 'pair_3'])

    def test_diff_names_returns_ordered_list_of_names(self):
        diff_1 = MuonDiff('diff_1', 'positive', 'negative')
        diff_2 = MuonDiff('diff_2', 'positive', 'negative')
        diff_3 = MuonDiff('diff_3', 'positive', 'negative')

        self.context.add_diff(diff_1)
        self.context.add_diff(diff_2)
        self.context.add_diff(diff_3)

        self.assertEqual(self.context.diff_names, ['diff_1', 'diff_2', 'diff_3'])

    def test_can_remove_groups_as_expected(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_group(group_3)

        self.context.remove_group('group_1')

        self.assertEqual(self.context.group_names, ['group_2', 'group_3'])

    def test_can_remove_pairs_as_expected(self):
        pair_1 = MuonPair('pair_1')
        pair_2 = MuonPair('pair_2')
        pair_3 = MuonPair('pair_3')
        self.context.add_pair(pair_1)
        self.context.add_pair(pair_2)
        self.context.add_pair(pair_3)

        self.context.remove_pair('pair_2')

        self.assertEqual(self.context.pair_names, ['pair_1', 'pair_3'])

    def test_can_remove_diffs_as_expected(self):
        diff_1 = MuonDiff('diff_1', 'positive', 'negative')
        diff_2 = MuonDiff('diff_2', 'positive', 'negative')
        diff_3 = MuonDiff('diff_3', 'positive', 'negative')
        self.context.add_diff(diff_1)
        self.context.add_diff(diff_2)
        self.context.add_diff(diff_3)

        self.context.remove_diff('diff_2')

        self.assertEqual(self.context.diff_names, ['diff_1', 'diff_3'])

    def test_get_group_workspace_names_returns_correct_workspace_names(self):
        group = create_group_populated_by_two_workspace()
        self.context.add_group(group)

        workspace_list = self.context.get_group_workspace_names([[33333]], ['group1'], False)

        self.assertEqual(workspace_list, ['asymmetry_name_33333'])

    def test_that_reset_to_default_groups_creates_correct_groups_and_pairs_for_single_period_data(self):
        workspace = CreateSampleWorkspace()
        LoadInstrument(workspace, InstrumentName="EMU", RewriteSpectraMap=True)

        self.context.reset_group_and_pairs_to_default(workspace, 'EMU', 'longitudanal', 1)

        self.assertEquals(self.context.group_names, ['fwd', 'bwd'])
        self.assertEquals(self.context.pair_names, ['long'])
        self.assertEquals(self.context.diff_names, [])
        for group in self.context.groups:
            self.assertEquals(group.periods, [1])

    def test_that_reset_to_default_groups_creates_correct_groups_and_pairs_for_multi_period_data(self):
        workspace = CreateSampleWorkspace()
        LoadInstrument(workspace, InstrumentName="EMU", RewriteSpectraMap=True)

        self.context.reset_group_and_pairs_to_default(workspace, 'EMU', 'longitudanal', 2)

        self.assertEquals(self.context.group_names, ['fwd1', 'bwd1', 'fwd2', 'bwd2'])
        self.assertEquals(self.context.pair_names, ['long1', 'long2'])
        self.assertEquals(self.context.diff_names, ['pair_diff1'])
        self.assertEquals(self.context.groups[0].periods, [1])
        self.assertEquals(self.context.groups[1].periods, [1])
        self.assertEquals(self.context.groups[2].periods, [2])
        self.assertEquals(self.context.pairs[0].forward_group, 'fwd1')
        self.assertEquals(self.context.pairs[0].backward_group, 'bwd1')
        self.assertEquals(self.context.pairs[1].forward_group, 'fwd2')
        self.assertEquals(self.context.pairs[1].backward_group, 'bwd2')
        self.assertEquals(self.context.diffs[0].positive, 'long1')
        self.assertEquals(self.context.diffs[0].negative, 'long2')
        self.assertEquals(self.context.selected, 'long1')

    def test_get_group_pair_name_and_run_from_workspace_name(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_group(group_3)
        group_1.update_counts_workspace(MuonRun([62260]), 'group_1_counts', False)
        group_1.update_asymmetry_workspace(MuonRun([62260]), 'group_1_asym', 'group_1_asym_unorm', False)
        workspace_name_list = self.context.get_group_workspace_names(runs = [[62260]], groups=['group_1'], rebin=False)

        group_name, run = self.context.get_group_pair_name_and_run_from_workspace_name(workspace_name_list[0])

        self.assertEqual(group_name, 'group_1')
        self.assertEqual(run, '62260')

    def test_get_group_pair_name_and_run_works_for_co_added_runs(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_group(group_3)
        group_1.update_counts_workspace(MuonRun([62260, 62261]), 'group_1_counts', False)
        group_1.update_asymmetry_workspace(MuonRun([62260, 62261]), 'group_1_asym', 'group_1_asym_unorm', False)
        workspace_name_list = self.context.get_group_workspace_names(runs = [[62260, 62261]], groups=['group_1'], rebin=False)

        group_name, run = self.context.get_group_pair_name_and_run_from_workspace_name(workspace_name_list[0])

        self.assertEqual(group_name, 'group_1')
        self.assertEqual(run, '62260-62261')

    def test_that_get_group_pair_name_and_run_works_for_fit_workspace_names_containing_original_worspace(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_group(group_3)
        group_1.update_counts_workspace(MuonRun([62260, 62261]), 'group_1_counts', False)
        group_1.update_asymmetry_workspace(MuonRun([62260, 62261]), 'group_1_asym', 'group_1_asym_unorm', False)
        workspace_name_list = self.context.get_group_workspace_names(runs = [[62260, 62261]], groups=['group_1'], rebin=False)

        group_name, run = self.context.get_group_pair_name_and_run_from_workspace_name(workspace_name_list[0] + '; Fit Seq Flatbackground')

        self.assertEqual(group_name, 'group_1')
        self.assertEqual(run, '62260-62261')

    def test_add_phasequad(self):
        phasequad = MuonPhasequad("test", "table")
        self.assertEqual(len(self.context._phasequad),0)
        self.assertEqual(len(self.context._pairs),0)

        self.context.add_phasequad(phasequad)
        self.assertEqual(len(self.context._phasequad),1)
        self.assertEqual(len(self.context._pairs),2)

        self.assertEqual(self.context._phasequad[0].name,"test")
        self.assertEqual(self.context._pairs[0].name,"test_Re_")
        self.assertEqual(self.context._pairs[1].name,"test_Im_")

    def test_rm_phasequad(self):
        phasequad = MuonPhasequad("test", "table")
        phasequad2 = MuonPhasequad("test2", "table2")
        self.context.add_phasequad(phasequad)
        self.context.add_phasequad(phasequad2)
        self.assertEqual(len(self.context._phasequad),2)
        self.assertEqual(len(self.context._pairs),4)

        self.context.remove_phasequad(phasequad)
        self.assertEqual(len(self.context._phasequad),1)
        self.assertEqual(len(self.context._pairs),2)

        self.assertEqual(self.context._phasequad[0].name,"test2")
        self.assertEqual(self.context._pairs[0].name,"test2_Re_")
        self.assertEqual(self.context._pairs[1].name,"test2_Im_")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

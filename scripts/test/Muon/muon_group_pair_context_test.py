# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest

from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.observer_pattern import GenericObserverWithArgPassing
from mantid.py3compat import mock
from Muon.GUI.Common.test_helpers.general_test_helpers import create_group_populated_by_two_workspace


class MuonGroupPairContextTest(unittest.TestCase):
    def setUp(self):
        self.context = MuonGroupPairContext()

    def test_can_be_created(self):
        self.assertTrue(self.context)

    def test_groups_and_pairs_initially_empty(self):
        self.assertEqual(self.context.groups, [])
        self.assertEqual(self.context.pairs, [])

    def test_group_can_be_added(self):
        group = MuonGroup('group_1', [1,3,5,7,9])

        self.context.add_group(group)

        self.assertEqual(self.context['group_1'], group)

    def test_non_group_cannot_be_added(self):
        pair = MuonPair('pair_1')

        self.assertRaises(AssertionError, self.context.add_group, pair)

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
        pair = MuonPair('pair_1')

        self.assertRaises(AssertionError, self.context.add_group, pair)

    def test_cannot_add_pair_with_duplicate_name(self):
        pair_1 = MuonPair('pair')
        pair_2 = MuonPair('pair')

        self.context.add_pair(pair_1)

        self.assertRaises(ValueError, self.context.add_pair, pair_2)

    def test_show_adds_group_or_pair_to_ADS(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        pair = MuonPair('pair_1')
        group_1.show = mock.MagicMock()
        pair.show = mock.MagicMock()
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_pair(pair)

        self.context.show('group_1', [12345])
        self.context.show('pair_1', [12345])

        group_1.show.assert_called_once_with(str([12345]))
        pair.show.assert_called_once_with(str([12345]))

    def test_group_names_returns_ordered_list_of_names(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])

        self.context.add_group(group_1)
        self.context.add_group(group_3)
        self.context.add_group(group_2)

        self.assertEquals(self.context.group_names, ['group_1', 'group_3', 'group_2'])

    def test_pair_names_returns_ordered_list_of_names(self):
        pair_1 = MuonPair('pair_1')
        pair_2 = MuonPair('pair_2')
        pair_3 = MuonPair('pair_3')

        self.context.add_pair(pair_1)
        self.context.add_pair(pair_2)
        self.context.add_pair(pair_3)

        self.assertEquals(self.context.pair_names, ['pair_1', 'pair_2', 'pair_3'])

    def test_can_remove_groups_as_expected(self):
        group_1 = MuonGroup('group_1', [1, 3, 5, 7, 9])
        group_2 = MuonGroup('group_2', [1, 3, 4, 7, 9])
        group_3 = MuonGroup('group_3', [1, 3, 4, 7, 9])
        self.context.add_group(group_1)
        self.context.add_group(group_2)
        self.context.add_group(group_3)

        self.context.remove_group('group_1')

        self.assertEquals(self.context.group_names, ['group_2', 'group_3'])

    def test_can_remove_pairs_as_expected(self):
        pair_1 = MuonPair('pair_1')
        pair_2 = MuonPair('pair_2')
        pair_3 = MuonPair('pair_3')
        self.context.add_pair(pair_1)
        self.context.add_pair(pair_2)
        self.context.add_pair(pair_3)

        self.context.remove_pair('pair_2')

        self.assertEquals(self.context.pair_names, ['pair_1', 'pair_3'])

    def test_get_group_workspace_names_returns_correct_workspace_names(self):
        group = create_group_populated_by_two_workspace()
        self.context.add_group(group)

        workspace_list = self.context.get_group_workspace_names([[33333]], ['group1'], False)

        self.assertEqual(workspace_list, ['asymmetry_name_33333'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

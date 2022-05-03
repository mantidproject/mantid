# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel


class GroupingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self._num_periods_store = {62260: 4, 62261: 2, 62263: 1}
        self.context.num_periods = lambda run : self._num_periods_store[run]

    def test_reset_group_and_pairs_to_default_correctly_identifies_maximum_number_of_periods_for_single_run(self):
        self.context.current_runs = [62260]
        model = GroupingTabModel(self.context)

        status = model.reset_groups_and_pairs_to_default()

        self.assertEqual(status, "success")
        self.context.group_pair_context.reset_group_and_pairs_to_default.assert_called_once_with(mock.ANY, mock.ANY, mock.ANY, 4)

    def test_reset_group_and_pairs_to_default_correctly_identifies_maximum_number_of_periods_for_multiple_runs(self):
        self.context.current_runs = [62260, 62261, 62263]
        model = GroupingTabModel(self.context)

        status = model.reset_groups_and_pairs_to_default()
        self.assertEqual(status, "success")

        self.context.group_pair_context.reset_group_and_pairs_to_default.assert_called_once_with(mock.ANY, mock.ANY, mock.ANY, 4)

    def test_reset_group_and_pairs_to_default_for_no_loaded_runs(self):
        self.context.current_runs = []
        model = GroupingTabModel(self.context)

        status = model.reset_groups_and_pairs_to_default()
        self.assertEqual(status, "failed")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

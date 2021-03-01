# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.difference_table_widget.difference_widget_presenter import DifferencePresenter
from Muon.GUI.Common.muon_group import MuonDiff, MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from mantidqt.utils.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests

MAX_NUMBER_OF_DIFFS = 20


def enter_diff_name_side_effect():
    name = []
    for i in range(MAX_NUMBER_OF_DIFFS + 1):
        name.append("diff_" + str(i))
    return name


@start_qapplication
class DifferenceWidgetPresenterTest(unittest.TestCase):

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.model = GroupingTabModel(context=self.context)
        self.presenter = DifferencePresenter(self.model)

        # Mock user input for getting diff name
        self.presenter.group_view.enter_diff_name = mock.Mock(side_effect=enter_diff_name_side_effect())
        self.presenter.pair_view.enter_diff_name = mock.Mock(side_effect=enter_diff_name_side_effect())

        # Mock warning methods
        self.presenter.group_view.warning_popup = mock.Mock()
        self.presenter.pair_view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(0, len(self.model.diff_names))
        self.assertEqual(0, len(self.model.diffs))

    def assert_view_empty(self):
        self.assertEqual(0, self.presenter.group_view.num_rows())
        self.assertEqual(0, self.presenter.pair_view.num_rows())

    def add_two_groups(self):
        group0 = MuonGroup(group_name="group_0", detector_ids=[1])
        group1 = MuonGroup(group_name="group_1", detector_ids=[2])
        self.model.add_group(group0)
        self.model.add_group(group1)

    def add_two_pairs(self):
        pair0 = MuonPair(pair_name="pair_0", forward_group_name="group_0", backward_group_name="group_1", alpha=1.0)
        pair1 = MuonPair(pair_name="pair_1", forward_group_name="group_1", backward_group_name="group_0", alpha=1.0)
        self.model.add_pair(pair0)
        self.model.add_pair(pair1)

    def add_two_group_diffs(self):
        if not self.model.group_names:
            self.add_two_groups()
        diff0 = MuonDiff('group_diff_0', 'group_0', 'group_1')
        diff1 = MuonDiff('group_diff_1', 'group_1', 'group_0')
        self.presenter.group_widget.add_diff(diff0)
        self.presenter.group_widget.add_diff(diff1)

    def add_two_pair_diffs(self):
        if not self.model.group_names:
            self.add_two_groups()
        if not self.model.pair_names:
            self.add_two_pairs()
        diff0 = MuonDiff('pair_diff_0', 'pair_0', 'pair_1', group_or_pair='pair')
        diff1 = MuonDiff('pair_diff_1', 'pair_1', 'pair_0', group_or_pair='pair')
        self.presenter.pair_widget.add_diff(diff0)
        self.presenter.pair_widget.add_diff(diff1)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Tests for the widget presenter
    # ------------------------------------------------------------------------------------------------------------------

    def test_update_view_from_model_is_empty(self):
        self.add_two_group_diffs()
        self.add_two_pair_diffs()

        # Change the model
        self.model.clear_diffs('group')
        self.model.clear_diffs('pair')

        # Now update the view
        self.presenter.update_view_from_model()

        self.assert_view_empty()
        self.assert_model_empty()

    def test_pair_model_unchanged(self):
        self.add_two_group_diffs()

        # Change model
        self.group_context.remove_diff('group_diff_0')

        # Update view
        self.presenter.update_view_from_model()

        self.assertEqual(0, self.presenter.pair_view.num_rows())
        self.assertEqual(1, self.presenter.group_view.num_rows())

    def test_group_model_unchanged(self):
        self.add_two_pair_diffs()

        # Change model
        self.group_context.remove_diff('pair_diff_0')

        # Update view
        self.presenter.update_view_from_model()

        self.assertEqual(1, self.presenter.pair_view.num_rows())
        self.assertEqual(0, self.presenter.group_view.num_rows())

    def test_tables_are_disabled_correctly(self):
        self.presenter.group_view.disable_editing = mock.Mock()
        self.presenter.pair_view.disable_editing = mock.Mock()
        self.presenter.disable_editing()

        self.assertEqual(1, self.presenter.group_view.disable_editing.call_count)
        self.assertEqual(1, self.presenter.pair_view.disable_editing.call_count)

    def test_tables_are_enabled_correctly(self):
        self.presenter.group_view.enable_editing = mock.Mock()
        self.presenter.pair_view.enable_editing = mock.Mock()
        self.presenter.enable_editing()

        self.assertEqual(1, self.presenter.group_view.enable_editing.call_count)
        self.assertEqual(1, self.presenter.pair_view.enable_editing.call_count)

    def test_add_subscribers(self):
        # Make some fake observers
        observer_1 = Observer()
        observer_2 = Observer()
        self.presenter.add_subscribers([observer_1, observer_2])

        self.assertEqual([observer_1, observer_2],
                         self.presenter.group_widget.selected_diff_changed_notifier._subscribers)
        self.assertEqual([observer_1, observer_2],
                         self.presenter.pair_widget.selected_diff_changed_notifier._subscribers)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)

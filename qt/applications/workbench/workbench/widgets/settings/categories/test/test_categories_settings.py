# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import unittest

from unittest.mock import call, Mock, ANY, MagicMock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.mocks.mock_qt import MockQWidget
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.categories.presenter import CategoriesSettings

from qtpy.QtCore import Qt


class MockQTreeWidgetParentItem(MockQWidget):
    def __init__(self):
        self.addChild = StrictMock()
        self.childCount = Mock(return_value=0)
        self.setCheckState = Mock()
        self.parent = Mock(return_value=False)
        self.childCount = Mock(return_value=1)
        self.child = Mock(return_value=MockQTreeWidgetChildItem(self))


class MockQTreeWidgetChildItem(MockQWidget):
    def __init__(self, parent=None):
        self.addChild = StrictMock()
        self.childCount = Mock(return_value=0)
        self.setCheckState = Mock()
        self.checkState = Mock()
        self.parent = Mock(return_value=parent)
        self.childCount = Mock(return_value=0)
        self.child = Mock(return_value=False)


class MockQTreeWidget(MockQWidget):
    def __init__(self):
        self.clear = StrictMock()
        self.setHeaderLabel = StrictMock()
        self.addTopLevelItem = StrictMock()
        self.topLevelItemCount = Mock(return_value=3)
        self.topLevelItem = StrictMock(return_value=MockQTreeWidgetParentItem())
        self.itemClicked = Mock()
        self.itemChanged = Mock()


class MockCategoriesView(object):
    def __init__(self):
        self.algorithm_tree_widget = MockQTreeWidget()
        self.interface_tree_widget = MockQTreeWidget()
        self.add_checked_widget_item = StrictMock(return_value=MockQTreeWidgetChildItem())


class MockMainWindow(MockQWidget):
    def __init__(self):
        self.interface_list = ["Direct", "Indirect", "Muon", "Reflectometry", "SANS"]


class MockCategoriesSettingsModel:
    def __init__(self):
        self.set_hidden_algorithms = MagicMock()
        self.set_hidden_interfaces = MagicMock()
        self.get_hidden_interfaces = MagicMock()
        self.algorithm_and_states = {
            "Arithmetic": False,
            "Arithmetic\\Errors": True,
            "Arithmetic\\FFT": False,
            "ISIS": False,
            "Workflow": True,
            "Workflow\\Diffraction\\DataHandling": True,
            "Workflow\\Diffraction": True,
        }
        self.get_algorithm_factory_category_map = MagicMock(return_value=self.algorithm_and_states)


@start_qapplication
class CategoriesSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.categories.presenter.ConfigService"

    def setUp(self):
        self.mock_view = MockCategoriesView()
        self.mock_model = MockCategoriesSettingsModel()

    def test_algorithm_state_correct_when_created(self):
        CategoriesSettings(None, view=self.mock_view, model=self.mock_model)
        for category, state in self.mock_model.algorithm_and_states.items():
            if "\\" in category:
                expected_calls = [call(self.mock_view.algorithm_tree_widget, category.split("\\")[-1], state, ANY)]
            else:
                expected_calls = [call(self.mock_view.algorithm_tree_widget, category.split("\\")[-1], state)]
            self.mock_view.add_checked_widget_item.assert_has_calls(expected_calls)

    def test_algorithm_categories_state_changes_correctly_when_bottom_level_box_clicked(self):
        # create a mock item with one parent
        parent_item = MockQTreeWidgetParentItem()
        child_item = parent_item.child()

        child_item.checkState = Mock(return_value=Qt.Checked)
        presenter = CategoriesSettings(None, view=self.mock_view)
        presenter.nested_box_clicked(child_item, 0)
        child_item.setCheckState.assert_called_once_with(0, Qt.Checked)
        parent_item.setCheckState.assert_called_once_with(0, Qt.Checked)

    def test_algorithm_categories_partial_states_change_correctly_when_bottom_level_box_clicked(self):
        parent_item = MockQTreeWidgetParentItem()
        child_item1 = parent_item.child()
        child_item2 = MockQTreeWidgetChildItem(parent_item)
        parent_item.childCount = Mock(return_value=2)
        parent_item.child.side_effect = [child_item1, child_item2]

        child_item1.checkState = Mock(return_value=Qt.Checked)
        child_item2.checkState = Mock(return_value=Qt.Unchecked)

        presenter = CategoriesSettings(None, view=self.mock_view)
        presenter.nested_box_clicked(child_item1, 0)
        child_item1.setCheckState.assert_called_once_with(0, Qt.Checked)
        parent_item.setCheckState.assert_called_once_with(0, Qt.PartiallyChecked)

    def test_set_hidden_algorithms_string(self):
        presenter = CategoriesSettings(None, view=self.mock_view, model=self.mock_model)
        hidden_algorithim_string = [
            i for i in sorted(self.mock_model.algorithm_and_states.keys()) if self.mock_model.algorithm_and_states[i] is True
        ]
        presenter._create_hidden_categories_string = Mock(return_value=hidden_algorithim_string)

        presenter.set_hidden_algorithms_string(None)
        self.mock_model.set_hidden_algorithms.assert_called_once_with(";".join(hidden_algorithim_string))

    def test_interface_state_correct_when_created(self):
        mock_main_window = MockMainWindow()
        hidden_interfaces = "Indirect; Muon; Reflectometry"
        self.mock_model.get_hidden_interfaces.return_value = hidden_interfaces
        CategoriesSettings(mock_main_window, view=self.mock_view, model=self.mock_model)

        expected_calls = []
        for category in mock_main_window.interface_list:
            is_hidden = False
            if category in hidden_interfaces.split(";"):
                is_hidden = True
            expected_calls.append(call(self.mock_view.interface_tree_widget, category, is_hidden))

        self.mock_view.add_checked_widget_item.assert_has_calls(expected_calls)

    def test_set_hidden_interface_string(self):
        self.mock_model.get_hidden_interfaces.return_value = ""
        presenter = CategoriesSettings(None, view=self.mock_view, model=self.mock_model)
        hidden_interface_string = "Indirect; Muon; Reflectometry"

        presenter._create_hidden_categories_string = Mock(return_value=hidden_interface_string)
        presenter.set_hidden_interfaces_string(None)
        self.mock_model.set_hidden_interfaces.assert_called_once_with(";".join(hidden_interface_string))

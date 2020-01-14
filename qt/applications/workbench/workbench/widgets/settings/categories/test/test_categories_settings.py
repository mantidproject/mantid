# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

import unittest

from mantid.py3compat.mock import call, Mock, MagicMock, patch, ANY
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.mocks.mock_qt import MockQWidget
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.categories.presenter import CategoriesSettings
from mantid.api import AlgorithmFactoryImpl

from qtpy.QtWidgets import QTreeWidgetItem
from qtpy.QtCore import Qt


class MockConfigService(object):

    def __init__(self):
        self.setString = StrictMock()


class MockQTreeWidgetParentItem(MockQWidget):
    def __init__(self):
        self.addChild = StrictMock()
        self.childCount = Mock(return_value = 0)
        self.setCheckState = Mock()
        self.parent = Mock(return_value = False)
        self.childCount = Mock(return_value = 1)
        self.child = Mock(return_value = MockQTreeWidgetChildItem(self))


class MockQTreeWidgetChildItem(MockQWidget):
    def __init__(self, parent=None):
        self.addChild = StrictMock()
        self.childCount = Mock(return_value = 0)
        self.setCheckState = Mock()
        self.checkState = Mock()
        self.parent = Mock(return_value = parent)
        self.childCount = Mock(return_value = 0)
        self.child = Mock(return_value = False)


class MockQTreeWidget(MockQWidget):
    def __init__(self):
        self.clear = StrictMock()
        self.setHeaderLabel = StrictMock()
        self.addTopLevelItem = StrictMock()
        self.topLevelItemCount = Mock(return_value = 3)
        self.topLevelItem = StrictMock(return_value = MockQTreeWidgetParentItem())
        self.itemClicked = Mock()
        self.itemChanged = Mock()

class MockCategoriesView(object):
    def __init__(self):
        self.algorithm_tree_widget = MockQTreeWidget()
        self.add_checked_widget_item = StrictMock(return_value=MockQTreeWidgetChildItem())

categories_and_states = {'Arithmetic' : False,
                         'Arithmetic\\Errors' : True,
                         'Arithmetic\\FFT' : False,
                         'ISIS' : False,
                         'Workflow' : True,
                         'Workflow\\Diffraction\\DataHandling' : True,
                         'Workflow\\Diffraction' : True}

mock_get_category_and_state = Mock(return_value = categories_and_states)

@start_qapplication
@patch.object(AlgorithmFactoryImpl, 'getCategoriesandState', mock_get_category_and_state)
class CategoriesSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.categories.presenter.ConfigService"

    def test_algorithm_categories_created_correctly(self):
        mock_view = MockCategoriesView()
        CategoriesSettings(None, mock_view)
        self.assertEqual(len(categories_and_states), mock_view.add_checked_widget_item.call_count)

    def test_algorithm_state_correct_when_created(self):
        mock_view = MockCategoriesView()
        CategoriesSettings(None, mock_view)
        for category, state in categories_and_states.items():
            if "\\" in category:
                expected_calls = [call(category.split("\\")[-1], state, ANY)]
            else:
                expected_calls = [call(category.split("\\")[-1], state)]
            mock_view.add_checked_widget_item.assert_has_calls(expected_calls)

    def test_algorithm_categories_state_changes_correctly_when_bottom_level_box_clicked(self):
        # create a mock item with one parent
        parent_item = MockQTreeWidgetParentItem()
        child_item = parent_item.child()

        child_item.checkState = Mock(return_value=Qt.Checked)
        mock_view = MockCategoriesView()
        presenter = CategoriesSettings(None, mock_view)
        presenter.box_clicked(child_item, 0)
        child_item.setCheckState.assert_called_once_with(0, Qt.Checked)
        parent_item.setCheckState.assert_called_once_with(0, Qt.Checked)

    def test_algorithm_categories_partial_states_change_correctly_when_bottom_level_box_clicked(self):
        # create a mock item with one parent
        parent_item = MockQTreeWidgetParentItem()
        child_item1 = parent_item.child()
        child_item2 = MockQTreeWidgetChildItem(parent_item)
        parent_item.childCount = Mock(return_value=2)
        parent_item.child.side_effect = [child_item1, child_item2]

        child_item1.checkState = Mock(return_value=Qt.Checked)
        child_item2.checkState = Mock(return_value=Qt.Unchecked)

        mock_view = MockCategoriesView()
        presenter = CategoriesSettings(None, mock_view)
        presenter.box_clicked(child_item1, 0)
        child_item1.setCheckState.assert_called_once_with(0, Qt.Checked)
        parent_item.setCheckState.assert_called_once_with(0, Qt.PartiallyChecked)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_set_hidden_algorithms_string(self, mock_ConfigService):
        presenter = CategoriesSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.set_hidden_algorithms_string(None)
        expected_string = ";".join([i for i in sorted(categories_and_states.keys()) if categories_and_states[i] is True])
        mock_ConfigService.setString.assert_called_once_with(CategoriesSettings.HIDDEN_ALGORITHMS, expected_string)

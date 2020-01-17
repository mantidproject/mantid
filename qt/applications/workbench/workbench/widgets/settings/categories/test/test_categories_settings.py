# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

import unittest

from mantid.py3compat.mock import call, Mock, patch, ANY
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.mocks.mock_qt import MockQWidget
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.categories.presenter import CategoriesSettings
from mantid.api import AlgorithmFactoryImpl

from qtpy.QtCore import Qt


class MockConfigService(object):

    def __init__(self):
        self.setString = StrictMock()
        self.getString = Mock(return_value='')


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
        self.interface_tree_widget = MockQTreeWidget()
        self.add_checked_widget_item = StrictMock(return_value=MockQTreeWidgetChildItem())


class MockMainWindow(MockQWidget):
    def __init__(self):
        self.interface_list = ['Direct', 'Indirect', 'Muon', 'Reflectometry', 'SANS']


algorithm_and_states = {'Arithmetic' : False,
                        'Arithmetic\\Errors' : True,
                        'Arithmetic\\FFT' : False,
                        'ISIS' : False,
                        'Workflow' : True,
                        'Workflow\\Diffraction\\DataHandling' : True,
                        'Workflow\\Diffraction' : True}

mock_get_category_and_state = Mock(return_value = algorithm_and_states)


@start_qapplication
class CategoriesSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.categories.presenter.ConfigService"

    @patch.object(AlgorithmFactoryImpl, 'getCategoriesandState', mock_get_category_and_state)
    def test_algorithm_state_correct_when_created(self):
        mock_view = MockCategoriesView()
        CategoriesSettings(None, mock_view)
        for category, state in algorithm_and_states.items():
            if "\\" in category:
                expected_calls = [call(mock_view.algorithm_tree_widget, category.split("\\")[-1], state, ANY)]
            else:
                expected_calls = [call(mock_view.algorithm_tree_widget, category.split("\\")[-1], state)]
            mock_view.add_checked_widget_item.assert_has_calls(expected_calls)

    def test_algorithm_categories_state_changes_correctly_when_bottom_level_box_clicked(self):
        # create a mock item with one parent
        parent_item = MockQTreeWidgetParentItem()
        child_item = parent_item.child()

        child_item.checkState = Mock(return_value=Qt.Checked)
        mock_view = MockCategoriesView()
        presenter = CategoriesSettings(None, mock_view)
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

        mock_view = MockCategoriesView()
        presenter = CategoriesSettings(None, mock_view)
        presenter.nested_box_clicked(child_item1, 0)
        child_item1.setCheckState.assert_called_once_with(0, Qt.Checked)
        parent_item.setCheckState.assert_called_once_with(0, Qt.PartiallyChecked)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_set_hidden_algorithms_string(self, mock_ConfigService):
        mock_view = MockCategoriesView()
        presenter = CategoriesSettings(None, mock_view)
        hidden_algorthim_string = [i for i in sorted(algorithm_and_states.keys()) if algorithm_and_states[i] is True]
        presenter._create_hidden_categories_string = Mock(return_value = hidden_algorthim_string)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.set_hidden_algorithms_string(None)
        mock_ConfigService.setString.assert_called_once_with(presenter.HIDDEN_ALGORITHMS,
                                                             ';'.join(hidden_algorthim_string))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_interface_state_correct_when_created(self, mock_ConfigService):
        mock_main_window = MockMainWindow()
        mock_view = MockCategoriesView()
        hidden_interfaces = 'Indirect; Muon; Reflectometry'
        mock_ConfigService.getString = Mock(return_value=hidden_interfaces)
        CategoriesSettings(mock_main_window, mock_view)

        expected_calls = []
        for category in mock_main_window.interface_list:
            is_hidden = False
            if category in hidden_interfaces.split(';'):
                is_hidden = True
            expected_calls.append(call(mock_view.interface_tree_widget, category, is_hidden))

        mock_view.add_checked_widget_item.assert_has_calls(expected_calls)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_set_hidden_interface_string(self, mock_ConfigService):
        mock_view = MockCategoriesView()
        mock_ConfigService.getString = Mock(return_value='')
        presenter = CategoriesSettings(None, mock_view)
        hidden_interface_string = 'Indirect; Muon; Reflectometry'

        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()
        presenter._create_hidden_categories_string = Mock(return_value=hidden_interface_string)
        presenter.set_hidden_interfaces_string(None)
        mock_ConfigService.setString.assert_called_once_with(presenter.HIDDEN_INTERFACES,
                                                             ';'.join(hidden_interface_string))

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mock import Mock

from mantidqt.widgets.TableWorkspacedisplay.table_view_model import TableWorkspaceTableViewModelType


class MockQTableHeader(object):
    def __init__(self):
        self.addAction = Mock()


class MockQSelection:
    def __init__(self):
        self.mock_item_range = MockQItemRange()
        self.first = Mock(return_value=self.mock_item_range)


class MockQItemRange(object):
    def __init__(self):
        self.top = Mock(return_value=0)
        self.bottom = Mock(return_value=2)
        self.left = Mock(return_value=0)
        self.right = Mock(return_value=2)


class MockQSelectionModel:
    def __init__(self):
        self.hasSelection = Mock()
        self.selectedRows = None
        self.selectedColumns = None
        self.currentIndex = None
        self.mock_selection = MockQSelection()
        self.selection = Mock(return_value=self.mock_selection)


class MockQTableViewModel:
    def __init__(self):
        self.type = TableWorkspaceTableViewModelType.x


class MockQTableView:
    def __init__(self):
        self.setContextMenuPolicy = Mock()
        self.addAction = Mock()
        self.mock_horizontalHeader = MockQTableHeader()
        self.mock_verticalHeader = MockQTableHeader()
        self.horizontalHeader = Mock(return_value=self.mock_horizontalHeader)
        self.verticalHeader = Mock(return_value=self.mock_verticalHeader)
        self.setModel = Mock()
        self.mock_model = MockQTableViewModel()
        self.model = Mock(return_value=self.mock_model)

        self.mock_selection_model = MockQSelectionModel()

        self.selectionModel = Mock(return_value=self.mock_selection_model)


class MockTableWorkspaceDisplayView:
    def __init__(self):
        self.set_context_menu_actions = Mock()
        self.table_x = MockQTableView()
        self.table_y = MockQTableView()
        self.table_e = MockQTableView()
        self.set_model = Mock()
        self.copy_to_clipboard = Mock()
        self.show_mouse_toast = Mock()
        self.ask_confirmation = None


class MockTableWorkspaceDisplayModel:
    def __init__(self):
        self.get_spectrum_plot_label = Mock()
        self.get_bin_plot_label = Mock()

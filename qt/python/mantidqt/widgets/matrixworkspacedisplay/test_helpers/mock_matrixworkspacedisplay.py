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


class MockQTableHeader(object):
    def __init__(self):
        self.addAction = Mock()


class MockQSelectionModel:
    def __init__(self):
        self.hasSelection = Mock()
        self.selectedRows = None
        self.selectedColumns = None
        self.currentIndex = None


class MockQTableView:
    def __init__(self):
        self.setContextMenuPolicy = Mock()
        self.addAction = Mock()
        self.mock_horizontalHeader = MockQTableHeader()
        self.mock_verticalHeader = MockQTableHeader()
        self.horizontalHeader = Mock(return_value=self.mock_horizontalHeader)
        self.verticalHeader = Mock(return_value=self.mock_verticalHeader)
        self.setModel = Mock()

        self.mock_selection_model = MockQSelectionModel()

        self.selectionModel = Mock(return_value=self.mock_selection_model)


class MockMatrixWorkspaceDisplayView:
    def __init__(self):
        self.set_context_menu_actions = Mock()
        self.table_x = MockQTableView()
        self.table_y = MockQTableView()
        self.table_e = MockQTableView()
        self.set_model = Mock()
        self.copy_to_clipboard = Mock()
        self.show_mouse_toast = Mock()
        self.ask_confirmation = None


class MockMatrixWorkspaceDisplayModel:
    def __init__(self):
        self.get_spectrum_plot_label = Mock()
        self.get_bin_plot_label = Mock()

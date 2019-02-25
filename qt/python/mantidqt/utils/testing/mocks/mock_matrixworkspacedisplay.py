# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

from mantid.py3compat.mock import Mock

from mantidqt.utils.testing.mocks.mock_qt import MockQTab, MockQTableView


class MockMatrixWorkspaceDisplayView:

    def __init__(self):
        self.set_context_menu_actions = Mock()
        self.table_x = MockQTableView()
        self.table_y = MockQTableView()
        self.table_e = MockQTableView()
        self.set_model = Mock()
        self.ask_confirmation = None
        self.emit_close = Mock()
        self.mock_tab = MockQTab()
        self.get_active_tab = Mock(return_value=self.mock_tab)


class MockMatrixWorkspaceDisplayModel:
    def __init__(self):
        self.get_spectrum_plot_label = Mock()
        self.get_bin_plot_label = Mock()

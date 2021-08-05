# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import mock
from mantidqt.utils.testing.mocks.mock_qt import MockQSelectionModel


class MockSequentialTableView(object):

    def __init__(self):
        self.setItemDelegateForColumn = mock.MagicMock()
        self.blockSignals = mock.MagicMock()
        self.resizeColumnsToContents = mock.MagicMock()
        self.setModel = mock.MagicMock()
        self.mock_selection_model = MockQSelectionModel()
        self.selectionModel = mock.MagicMock(return_value=self.mock_selection_model)


class MockSequentialTableModel(object):
    def __init__(self):
        self.set_fit_parameters_and_values = mock.MagicMock()
        self.set_fit_quality = mock.MagicMock()
        self.get_workspace_name_information = mock.MagicMock()
        self.get_run_information = mock.MagicMock()
        self.get_group_information = mock.MagicMock()
        self.rowCount = mock.MagicMock()

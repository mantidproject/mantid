# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

import functools
import unittest
from unittest.mock import Mock

from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspaceGroup
from mantidqt.widgets.workspacedisplay.table.presenter_group import TableWorkspaceDataPresenterGroup
from mantidqt.widgets.workspacedisplay.table.group_model import GroupTableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView


def with_mock_presenter(func):
    @functools.wraps(func)
    def wrapper(self):
        view = Mock(spec=TableWorkspaceDisplayView)
        model = Mock(spec=GroupTableWorkspaceDisplayModel)
        presenter = TableWorkspaceDataPresenterGroup(view=view, model=model)
        return func(self, presenter)

    return wrapper


class TableWorkspaceDisplayTableModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        GroupTableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspaceGroup)

    @with_mock_presenter
    def test_load_data(self, presenter):
        model = Mock()
        table = Mock()
        table.model = Mock(return_value=model)
        model.load_data = Mock(return_value=None)

        presenter.load_data(table)
        model.load_data.assert_called_with(presenter.model)

        presenter.sort_data = (0, True)
        presenter.view.sortBySelectedColumn = Mock()
        presenter.view.hideColumn = Mock()
        presenter.load_data(table)
        model.load_data.assert_called_with(presenter.model)
        presenter.view.sortBySelectedColumn.assert_called_once_with(0, True)
        presenter.view.hideColumn.assert_called_once_with(0)

    @with_mock_presenter
    def test_update_column_headers(self, presenter):
        model = Mock()
        presenter.view.model = Mock(return_value=model)

        col_headers = ["h", "k", "l"]
        presenter.model.original_column_headers = Mock(return_value=col_headers)
        presenter.model.build_current_labels = Mock(return_value=[])

        presenter.update_column_headers()
        model.setHorizontalHeaderLabels.assert_called_with(col_headers)

        presenter.model.build_current_labels = Mock(return_value=[(0, "-m")])
        col_headers[0] += "-m"
        model.setHorizontalHeaderLabels.assert_called_with(col_headers)

    @with_mock_presenter
    def test_delete_rows(self, presenter):
        model = Mock()
        model.index = Mock(return_value=0)
        model.data = Mock(return_value=0)
        presenter.view.model = Mock(return_value=model)
        presenter.model.delete_rows = Mock()

        presenter.delete_rows([0])
        presenter.model.delete_rows.assert_called_once_with([(0, 0)])

    @with_mock_presenter
    def test_sort(self, presenter):
        presenter.view.sortBySelectedColumn = Mock()
        presenter.view.hideColumn = Mock()

        selected_col = 0
        sort_ascending = True
        presenter.sort(selected_col, sort_ascending)
        presenter.view.sortBySelectedColumn.assert_called_once()
        presenter.view.hideColumn.assert_called_once()
        self.assertEqual(presenter.sort_data, (selected_col, sort_ascending))


if __name__ == "__main__":
    unittest.main()

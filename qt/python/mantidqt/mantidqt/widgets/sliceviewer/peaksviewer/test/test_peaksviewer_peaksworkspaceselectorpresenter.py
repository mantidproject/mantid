# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import create_autospec

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.workspaceselection import (
    PeaksWorkspaceSelectorModel,
    PeaksWorkspaceSelectorPresenter,
    PeaksWorkspaceSelectorView,
)


class PeaksWorkspaceSelectorPresenterTest(unittest.TestCase):
    def test_construction_subscribes_to_view_and_sets_workspaces(self):
        mock_view, mock_model = (
            create_autospec(PeaksWorkspaceSelectorView, instance=True),
            create_autospec(PeaksWorkspaceSelectorModel, instance=True),
        )
        presenter = PeaksWorkspaceSelectorPresenter(mock_view, mock_model)

        mock_view.subscribe.assert_called_once_with(presenter)
        mock_model.names_and_statuses.assert_called_once()


if __name__ == "__main__":
    unittest.main()

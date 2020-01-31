# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# std imports
import unittest

# 3rdparty imports
from mantid.api import MatrixWorkspace
from mantid.dataobjects import PeaksWorkspace
from mantid.py3compat.mock import create_autospec, patch, ANY, MagicMock
from mantidqt.widgets.sliceviewer.peaksviewer.presenter \
    import PeaksViewerPresenter, TableWorkspaceDataPresenter


def create_mock_peaks_workspace(name):
    """Create a mock object that looks like a PeaksWorkspace"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    return mock_ws

@patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenter",
       autospec=TableWorkspaceDataPresenter)
class PeaksViewerPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    def test_presenter_subscribes_to_view_updates(self, _):
        mock_view = MagicMock()

        presenter = PeaksViewerPresenter(create_mock_peaks_workspace('ws'),
                                         mock_view)

        mock_view.subscribe.assert_called_once_with(presenter)

    def test_view_populated_on_presenter_construction(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = 'ws1'
        mock_ws = create_mock_peaks_workspace(name)

        PeaksViewerPresenter(mock_ws, mock_view)

        mock_view.set_title.assert_called_once_with(name)
        # peaks list presenter construction
        mock_peaks_list_presenter.assert_called_once_with(ANY, mock_view.table_view)

    # -------------------- failure tests -----------------------------
    def test_construction_with_non_peaksworkspace_raises_error(self, _):
        self.assertRaises(ValueError,
                          PeaksViewerPresenter, create_autospec(MatrixWorkspace),
                          MagicMock())


if __name__ == '__main__':
    unittest.main()

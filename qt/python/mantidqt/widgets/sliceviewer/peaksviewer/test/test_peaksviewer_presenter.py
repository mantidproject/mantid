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
from mantid.dataobjects import PeaksWorkspace
from mantid.py3compat.mock import create_autospec, patch, ANY, MagicMock
from mantidqt.widgets.sliceviewer.peaksviewer import PeaksViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer.presenter\
    import PeaksViewerPresenter, TableWorkspaceDataPresenter


def create_fake_model(name):
    """Create a mock object that looks like a PeaksWorkspace"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    return PeaksViewerModel(mock_ws)

@patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenter",
       autospec=TableWorkspaceDataPresenter)
class PeaksViewerPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    def test_presenter_subscribes_to_view_updates(self, _):
        mock_view = MagicMock()

        presenter = PeaksViewerPresenter(create_fake_model('ws'),
                                         mock_view)

        mock_view.subscribe.assert_called_once_with(presenter)

    def test_view_populated_on_presenter_construction(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = 'ws1'
        mock_ws = create_fake_model(name)

        PeaksViewerPresenter(mock_ws, mock_view)

        mock_view.set_title.assert_called_once_with(name)
        # peaks list presenter construction
        mock_peaks_list_presenter.assert_called_once_with(ANY, mock_view.table_view)


if __name__ == '__main__':
    unittest.main()

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
from mantid.py3compat.mock import call, create_autospec, patch, MagicMock
from mantidqt.widgets.sliceviewer.peaksviewer.presenter \
    import PeaksViewerPresenter, PeaksViewerCollectionPresenter, TableWorkspaceDataPresenter


def create_mock_peaks_workspace():
    """Create a mock object that looks like a PeaksWorkspace"""
    return create_autospec(PeaksWorkspace)


class PeaksViewerCollectionPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    @patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter")
    def test_presenter_constructs_PeaksViewerPresenter_per_workspace(self, mock_single_presenter):
        peak_workspaces = (create_mock_peaks_workspace() for _ in range(2))
        mock_view = MagicMock()

        presenter = PeaksViewerCollectionPresenter(peak_workspaces, mock_view)

        mock_single_presenter.assert_has_calls([call(peaks_ws) for peaks_ws in peak_workspaces])
        self.assertEqual(2, presenter.view.append_peaksviewer.call_count)

    # -------------------- failure tests -----------------------------
    def test_presenter_constructed_with_non_peaksworkspace_raises_error(self):
        self.assertRaises(ValueError,
                          PeaksViewerCollectionPresenter,
                          [create_mock_peaks_workspace(), 2],
                          MagicMock())


if __name__ == '__main__':
    unittest.main()

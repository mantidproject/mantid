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
from mantid.py3compat.mock import call, create_autospec, patch, ANY, MagicMock
from mantidqt.widgets.sliceviewer.peaksviewer \
    import PeaksViewerPresenter, PeaksViewerCollectionPresenter, PeaksViewerModel


def create_mock_peaks_workspace():
    """Create a mock object that looks like a PeaksWorkspace"""
    return create_autospec(PeaksWorkspace)


class PeaksViewerCollectionPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    @patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter")
    def test_append_constructs_PeaksViewerPresenter(self, mock_single_presenter):
        peaks_workspace = PeaksViewerModel(create_mock_peaks_workspace())
        mock_collection_view = MagicMock()
        mock_peaks_view = MagicMock()
        mock_collection_view.append_peaksviewer.return_value = mock_peaks_view

        presenter = PeaksViewerCollectionPresenter(mock_collection_view)
        presenter.append_peaksworkspace(peaks_workspace)

        mock_single_presenter.assert_has_calls([call(peaks_workspace, mock_peaks_view)])
        self.assertEqual(1, presenter.view.append_peaksviewer.call_count)

    def test_notify_called_for_each_subpresenter(self):
        peak_workspaces = (PeaksViewerModel(create_mock_peaks_workspace()) for _ in range(2))

        with patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter"
                   ) as presenter_mock:
            view = MagicMock()
            # make each call to to mock_presenter return a new mock object
            presenter_mock.side_effect = [MagicMock(), MagicMock()]
            presenter = PeaksViewerCollectionPresenter(view)
            child_presenters = []
            for peaks_ws in peak_workspaces:
                child_presenters.append(presenter.append_peaksworkspace(peaks_ws))
            presenter.notify(PeaksViewerPresenter.Event.OverlayPeaks)
            for child in child_presenters:
                child.notify.assert_called_once_with(PeaksViewerPresenter.Event.OverlayPeaks)


if __name__ == '__main__':
    unittest.main()

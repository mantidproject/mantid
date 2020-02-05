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

    def test_peaks_info_returns_combined_PeakRepresentation_for_each_workspace(self):
        peak_workspaces = (create_mock_peaks_workspace() for _ in range(2))

        class PeaksViewerPresenterMock(MagicMock):
            """Mock created when PeaksViewerPresenterMock is a called.
            Changes the number of mock Peaks returned by PeaksViewerPresenter.peaks_info"""
            child_count = 0
            # number of "peaks" return by successive calls
            npeaks = (3, 4)

            def peaks_info(self):
                peaks = [MagicMock() for _ in range(self.npeaks[self.child_count])]
                self.__class__.child_count += 1
                return peaks

        with patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter",
                   new_callable=PeaksViewerPresenterMock):
            view = MagicMock()
            presenter = PeaksViewerCollectionPresenter(peak_workspaces, view)
            peaks_info = presenter.peaks_info()

        self.assertEqual(7, len(list(peaks_info)))

    # -------------------- failure tests -----------------------------
    def test_presenter_constructed_with_non_peaksworkspace_raises_error(self):
        self.assertRaises(ValueError,
                          PeaksViewerCollectionPresenter,
                          [create_mock_peaks_workspace(), 2],
                          MagicMock())


if __name__ == '__main__':
    unittest.main()

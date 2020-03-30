# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import call, create_autospec, patch, MagicMock

# 3rdparty imports
from mantid.dataobjects import PeaksWorkspace
from mantidqt.widgets.sliceviewer.peaksviewer \
    import PeaksViewerPresenter, PeaksViewerCollectionPresenter, PeaksViewerModel


def create_mock_peaks_workspace(name="peaks"):
    """Create a mock object that looks like a PeaksWorkspace"""
    mock = create_autospec(PeaksWorkspace)
    mock.name.return_value = name
    return mock


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
            presenter, child_presenters = self._create_collection_presenter(
                peak_workspaces, presenter_mock)
            presenter.notify(PeaksViewerPresenter.Event.OverlayPeaks)
            for child in child_presenters:
                child.notify.assert_called_once_with(PeaksViewerPresenter.Event.OverlayPeaks)

    def test_workspace_names_returns_all_displayed_names(self):
        names = ("peaks1", "peaks2")
        peak_workspaces = (PeaksViewerModel(create_mock_peaks_workspace(name)) for name in names)
        with patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksViewerPresenter"
                   ) as presenter_mock:
            presenter, child_presenters = self._create_collection_presenter(
                peak_workspaces, presenter_mock)
            overlayed_names = presenter.workspace_names()

            for overlayed_name in overlayed_names:
                self.assertTrue(overlayed_name in names)

    # private
    def _create_collection_presenter(self, models, single_presenter_mock):
        view = MagicMock()
        # make each call to to mock_presenter return a new mock object with
        # the appropriate return_value for the model
        side_effects = []
        for model in models:
            mock = MagicMock()
            mock.model.return_value = model
            side_effects.append(mock)
        single_presenter_mock.side_effect = side_effects
        presenter = PeaksViewerCollectionPresenter(view)
        child_presenters = []
        for peaks_ws in models:
            child_presenters.append(presenter.append_peaksworkspace(peaks_ws))

        return presenter, child_presenters


if __name__ == '__main__':
    unittest.main()

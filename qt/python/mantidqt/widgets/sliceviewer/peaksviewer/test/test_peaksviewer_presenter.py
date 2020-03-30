# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import create_autospec, patch, ANY, MagicMock

# 3rdparty imports
from mantid.dataobjects import PeaksWorkspace
from mantidqt.widgets.sliceviewer.peaksviewer import PeaksViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer.presenter\
    import PeaksViewerPresenter, TableWorkspaceDataPresenter
from mantidqt.widgets.sliceviewer.peaksviewer.representation\
    import NonIntegratedPeakRepresentation


def create_test_model(name):
    """Create a test model object from a mock workspace"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    return PeaksViewerModel(mock_ws)


def create_mock_model(name):
    """Create a mock object to test calss"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    mock = create_autospec(PeaksViewerModel)
    return mock


@patch(
    "mantidqt.widgets.sliceviewer.peaksviewer.presenter.TableWorkspaceDataPresenter",
    autospec=TableWorkspaceDataPresenter)
class PeaksViewerPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    def test_presenter_subscribes_to_view_updates(self, _):
        mock_view = MagicMock()

        presenter = PeaksViewerPresenter(create_mock_model('ws'), mock_view)

        mock_view.subscribe.assert_called_once_with(presenter)

    def test_view_populated_on_presenter_construction(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = 'ws1'
        test_model = create_test_model(name)

        PeaksViewerPresenter(test_model, mock_view)

        mock_view.set_title.assert_called_once_with(name)
        # peaks list presenter construction
        mock_peaks_list_presenter.assert_called_once_with(ANY, mock_view.table_view)

    def test_single_peak_selection(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = 'ws1'
        mock_model = create_mock_model(name)
        peak = create_autospec(NonIntegratedPeakRepresentation)  #1.5, 2.5, 10.5, 0.7, 'r')
        mock_model.peak_representation_at.return_value = peak
        presenter = PeaksViewerPresenter(mock_model, mock_view)

        presenter.notify(PeaksViewerPresenter.Event.PeakSelectionChanged)

        mock_view.snap_to.assert_called_once_with(peak)


if __name__ == '__main__':
    unittest.main()

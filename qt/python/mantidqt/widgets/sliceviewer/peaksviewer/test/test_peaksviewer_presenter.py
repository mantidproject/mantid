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
from mantidqt.widgets.sliceviewer.peaksviewer.presenter \
    import PeaksViewerPresenter, PeaksWorkspaceDataPresenter
from mantidqt.widgets.sliceviewer.peaksviewer.test.modeltesthelpers\
    import create_peaks_viewer_model, create_slice_info  # noqa


def create_test_model(name):
    """Create a test model object from a mock workspace"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    return PeaksViewerModel(mock_ws, 'r', 'b')


def create_mock_model(name):
    """Create a mock object to test calss"""
    mock_ws = create_autospec(PeaksWorkspace)
    mock_ws.name.return_value = name
    mock = create_autospec(PeaksViewerModel)
    return mock


@patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksWorkspaceDataPresenter",
       autospec=PeaksWorkspaceDataPresenter)
class PeaksViewerPresenterTest(unittest.TestCase):

    # -------------------- success tests -----------------------------
    def test_presenter_subscribes_to_view_updates(self, _):
        mock_view = MagicMock()

        presenter = PeaksViewerPresenter(create_peaks_viewer_model([], 'r'), mock_view)

        mock_view.subscribe.assert_called_once_with(presenter)

    def test_view_populated_on_presenter_construction(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = "ws1"
        fg_color = "r"
        test_model = create_peaks_viewer_model([(1, 2, 3)], fg_color=fg_color, name=name)

        PeaksViewerPresenter(test_model, mock_view)

        mock_view.set_title.assert_called_once_with(name)
        mock_view.set_peak_color.assert_called_once_with(fg_color)
        # peaks list presenter construction
        mock_peaks_list_presenter.assert_called_once_with(ANY, mock_view.table_view)

    def test_clear_removes_painted_peaks(self, mock_peaks_list_presenter):
        centers = ((1, 2, 3), (4, 5, 3.01))
        slice_info = create_slice_info(centers, slice_value=3, slice_width=5)
        test_model = create_peaks_viewer_model(centers, fg_color="r")
        painter, axes = MagicMock(), MagicMock()
        axes.get_xlim.return_value = (-1, 1)
        painter.axes = axes
        test_model.draw_peaks(slice_info, painter)
        mock_view = MagicMock()
        mock_view.painter = painter
        presenter = PeaksViewerPresenter(test_model, mock_view)

        presenter.notify(PeaksViewerPresenter.Event.ClearPeaks)

        self.assertEqual(2, mock_view.painter.remove.call_count)

    def test_slice_point_changed_clears_old_peaks_and_overlays_visible(
            self, mock_peaks_list_presenter):
        centers = ((1, 2, 3), (4, 5, 3.01))
        slice_info = create_slice_info(centers, slice_value=3, slice_width=5)
        test_model = create_peaks_viewer_model(centers, fg_color="r")
        # draw some peaks first so we can test clearing them
        painter, axes = MagicMock(), MagicMock()
        axes.get_xlim.return_value = (-1, 1)
        painter.axes = axes

        test_model.draw_peaks(slice_info, painter)
        # clear draw calls
        painter.cross.reset_mock()
        mock_view = MagicMock()
        mock_view.painter = painter
        mock_view.sliceinfo = create_slice_info(centers, slice_value=3, slice_width=5)
        presenter = PeaksViewerPresenter(test_model, mock_view)

        presenter.notify(PeaksViewerPresenter.Event.SlicePointChanged)

        self.assertEqual(2, mock_view.painter.remove.call_count)
        self.assertEqual(2, mock_view.painter.cross.call_count)

    def test_single_peak_selection(self, mock_peaks_list_presenter):
        mock_view = MagicMock()
        name = 'ws1'
        mock_model = create_mock_model(name)
        viewlimits = (-1, 1), (-2, 2)
        mock_model.viewlimits.return_value = viewlimits
        mock_view.selected_index = 0
        presenter = PeaksViewerPresenter(mock_model, mock_view)

        presenter.notify(PeaksViewerPresenter.Event.PeakSelected)

        mock_model.viewlimits.assert_called_once_with(0)
        mock_view.set_axes_limits.assert_called_once_with(*viewlimits, auto_transform=False)


if __name__ == '__main__':
    unittest.main()

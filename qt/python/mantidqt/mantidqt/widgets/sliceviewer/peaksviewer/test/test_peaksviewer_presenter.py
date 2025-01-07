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
from mantid.kernel import SpecialCoordinateSystem
from mantidqt.widgets.sliceviewer.peaksviewer import PeaksViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer.presenter import PeaksViewerPresenter, PeaksWorkspaceDataPresenter
from mantidqt.widgets.sliceviewer.peaksviewer.view import PeaksViewerView, _PeaksWorkspaceTableView
from mantidqt.widgets.sliceviewer.peaksviewer.test.modeltesthelpers import create_peaks_viewer_model, create_slice_info


def create_test_model(name):
    """Create a test model object from a mock workspace"""
    mock_ws = create_autospec(PeaksWorkspace, instance=True)
    mock_ws.name.return_value = name
    return PeaksViewerModel(mock_ws, "r", "b")


def create_mock_model(name):
    """Create a mock object to test calss"""
    mock_ws = create_autospec(PeaksWorkspace, instance=True)
    mock_ws.name.return_value = name
    mock = create_autospec(PeaksViewerModel, instance=True)
    return mock


def create_mock_view():
    mock_view = create_autospec(PeaksViewerView, instance=True)
    mock_view.table_view = create_autospec(_PeaksWorkspaceTableView, instance=True)
    return mock_view


@patch("mantidqt.widgets.sliceviewer.peaksviewer.presenter.PeaksWorkspaceDataPresenter", autospec=PeaksWorkspaceDataPresenter)
class PeaksViewerPresenterTest(unittest.TestCase):
    def setUp(self):
        self.mock_view = create_mock_view()

    # -------------------- success tests -----------------------------
    def test_presenter_subscribes_to_view_updates(self, _):
        presenter = PeaksViewerPresenter(create_peaks_viewer_model([], "r"), self.mock_view)

        self.mock_view.subscribe.assert_called_once_with(presenter)

    def test_view_populated_on_presenter_construction(self, mock_peaks_list_presenter):
        name = "ws1"
        fg_color = "r"
        test_model = create_peaks_viewer_model([(1, 2, 3)], fg_color=fg_color, name=name)

        PeaksViewerPresenter(test_model, self.mock_view)

        self.mock_view.set_title.assert_called_once_with(name)
        self.mock_view.set_peak_color.assert_called_once_with(fg_color)
        # peaks list presenter construction
        mock_peaks_list_presenter.assert_called_once_with(ANY, self.mock_view.table_view)
        self.mock_view.table_view.enable_sorting.assert_called_once()

    def test_clear_removes_painted_peaks(self, mock_peaks_list_presenter):
        centers = ((1, 2, 3), (4, 5, 3.01))
        slice_info = create_slice_info(centers, slice_value=3, slice_width=5)
        test_model = create_peaks_viewer_model(centers, fg_color="r")
        painter, axes = MagicMock(), MagicMock()
        axes.get_xlim.return_value = (-1, 1)
        painter.axes = axes
        test_model.draw_peaks(slice_info, painter, SpecialCoordinateSystem.QSample)
        self.mock_view.painter = painter
        presenter = PeaksViewerPresenter(test_model, self.mock_view)

        presenter.notify(PeaksViewerPresenter.Event.ClearPeaks)

        self.assertEqual(2, self.mock_view.painter.remove.call_count)

    def test_slice_point_changed_clears_old_peaks_and_overlays_visible(self, mock_peaks_list_presenter):
        centers = ((1, 2, 3), (4, 5, 3.01))
        slice_info = create_slice_info(centers, slice_value=3, slice_width=5)
        test_model = create_peaks_viewer_model(centers, fg_color="r")
        # draw some peaks first so we can test clearing them
        painter, axes = MagicMock(), MagicMock()
        axes.get_xlim.return_value = (-1, 1)
        painter.axes = axes

        test_model.draw_peaks(slice_info, painter, SpecialCoordinateSystem.QSample)
        # clear draw calls
        painter.cross.reset_mock()
        self.mock_view.painter = painter
        self.mock_view.sliceinfo = create_slice_info(centers, slice_value=3, slice_width=5)
        presenter = PeaksViewerPresenter(test_model, self.mock_view)

        presenter.notify(PeaksViewerPresenter.Event.SlicePointChanged)

        self.assertEqual(2, self.mock_view.painter.remove.call_count)
        self.assertEqual(2, self.mock_view.painter.cross.call_count)

    def test_single_peak_selection(self, mock_peaks_list_presenter):
        name = "ws1"
        mock_model = create_mock_model(name)
        mock_model.has_representations_drawn.return_value = True
        viewlimits = (-1, 1), (-2, 2)
        mock_model.viewlimits.return_value = viewlimits
        self.mock_view.selected_index = 0
        presenter = PeaksViewerPresenter(mock_model, self.mock_view)

        presenter.notify(PeaksViewerPresenter.Event.PeakSelected)

        mock_model.viewlimits.assert_called_once_with(0)
        self.mock_view.set_axes_limits.assert_called_once_with(*viewlimits, auto_transform=False)

    def test_single_peak_selection_if_peaks_not_drawn(self, mock_peaks_list_presenter):
        # peaks not drawn if one fo viewing axes non-Q
        name = "ws1"
        mock_model = create_mock_model(name)
        mock_model.has_representations_drawn.return_value = False
        self.mock_view.selected_index = 0
        presenter = PeaksViewerPresenter(mock_model, self.mock_view)

        presenter.notify(PeaksViewerPresenter.Event.PeakSelected)

        mock_model.viewlimits.assert_not_called()
        self.mock_view.set_axes_limits.assert_not_called()

    def test_add_delete_peaks(self, mock_peaks_list_presenter):
        name = "ws1"
        mock_model = create_mock_model(name)
        self.mock_view.frame = "Frame"
        presenter = PeaksViewerPresenter(mock_model, self.mock_view)
        presenter.add_peak([1, 2, 3])
        mock_model.add_peak.assert_called_once_with([1, 2, 3], "Frame")
        presenter.delete_peak([1, 2, 3])
        mock_model.delete_peak.assert_called_once_with([1, 2, 3], "Frame")


if __name__ == "__main__":
    unittest.main()

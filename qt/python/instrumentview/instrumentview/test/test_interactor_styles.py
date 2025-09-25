from instrumentview.InteractorStyles import CustomInteractorStyleRubberBand3D, CustomInteractorStyleZoomAndSelect, on_selection_changed
from pyvistaqt import BackgroundPlotter
import unittest
from unittest import mock


class TestInteractorStyles(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._plotter = BackgroundPlotter()

    def _make_mock_caller(self, start_position, end_position, window_size):
        mock_caller = mock.MagicMock()
        mock_caller.GetStartPosition.return_value = start_position
        mock_caller.GetEndPosition.return_value = end_position
        mock_picker = mock.MagicMock()
        mock_picker.AreaPick.return_value = mock.MagicMock()
        mock_interactor = mock.MagicMock()
        mock_interactor.GetPicker.return_value = mock_picker
        render_window_mock = mock.MagicMock()
        render_window_mock.GetSize.return_value = window_size
        mock_interactor.GetRenderWindow.return_value = render_window_mock
        mock_caller.GetInteractor.return_value = mock_interactor
        mock_caller.GetCurrentRenderer.return_value = mock.MagicMock()
        return mock_caller, mock_picker

    @mock.patch("instrumentview.InteractorStyles.on_selection_changed")
    def test_custom_3d_selection_changed(self, callback_mock):
        self._plotter.iren.style = CustomInteractorStyleRubberBand3D()
        self._plotter.iren.interactor.GetInteractorStyle().InvokeEvent("SelectionChangedEvent")
        callback_mock.assert_called_once()

    @mock.patch("instrumentview.InteractorStyles.on_selection_changed")
    def test_custom_2d_selection_changed(self, callback_mock):
        style = CustomInteractorStyleZoomAndSelect()
        style.set_interactor(self._plotter.iren.interactor)
        self._plotter.iren.style = style
        style.rubber_band.InvokeEvent("SelectionChangedEvent")
        callback_mock.assert_called_once()

    def test_on_selection_changed_good_coords(self):
        mock_caller, mock_picker = self._make_mock_caller((5, 10), (4, 11), (15, 15))
        on_selection_changed(mock_caller)
        mock_picker.AreaPick.assert_called_with(4, 10, 5, 11, mock_caller.GetCurrentRenderer())

    def test_on_selection_changed_coords_bigger_than_window(self):
        mock_caller, mock_picker = self._make_mock_caller((5, 17), (20, 11), (15, 15))
        on_selection_changed(mock_caller)
        mock_picker.AreaPick.assert_called_with(5, 11, 13, 13, mock_caller.GetCurrentRenderer())

    def test_on_selection_changed_coords_less_than_zero(self):
        mock_caller, mock_picker = self._make_mock_caller((0, 10), (4, -1), (15, 15))
        on_selection_changed(mock_caller)
        mock_picker.AreaPick.assert_called_with(0, 0, 4, 10, mock_caller.GetCurrentRenderer())

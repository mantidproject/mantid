# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

from mantidqt.utils.qt.testing import start_qapplication
from instrumentview.isisreflectometry.ReflectometryInstrumentViewView import ReflectometryInstrumentViewView
from instrumentview.ShapeWidgets import RectangleSelectionShape


@start_qapplication
class TestReflectometryInstrumentViewView(unittest.TestCase):
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def setUp(self, mock_bg_plotter):
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            self._view = ReflectometryInstrumentViewView()
        self._mock_bg_plotter_cls = mock_bg_plotter
        # QHBoxLayout.addWidget rejects MagicMock; patch it for all tests that call initialise()
        add_widget_patcher = mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
        self._mock_add_widget = add_widget_patcher.start()
        self.addCleanup(add_widget_patcher.stop)

    def test_main_plotter_is_none_before_initialise(self):
        self.assertIsNone(self._view.main_plotter)

    def test_not_initialised_before_initialise(self):
        self.assertFalse(self._view._initialised)

    def test_shape_overlay_manager_is_none_initially(self):
        self.assertIsNone(self._view.shape_overlay_manager)

    def test_layout_has_no_margins(self):
        layout = self._view.layout()
        self.assertIsNotNone(layout)
        self.assertEqual(layout.contentsMargins().left(), 0)
        self.assertEqual(layout.contentsMargins().right(), 0)
        self.assertEqual(layout.contentsMargins().top(), 0)
        self.assertEqual(layout.contentsMargins().bottom(), 0)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_initialise_creates_background_plotter(self, mock_bg_plotter_cls):
        self._view.initialise()
        mock_bg_plotter_cls.assert_called_once_with(show=False, menu_bar=False, toolbar=False, off_screen=False)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_initialise_sets_main_plotter(self, mock_bg_plotter_cls):
        self._view.initialise()
        self.assertIsNotNone(self._view.main_plotter)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_initialise_sets_initialised_flag(self, mock_bg_plotter_cls):
        self._view.initialise()
        self.assertTrue(self._view._initialised)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_initialise_is_idempotent(self, mock_bg_plotter_cls):
        """Calling initialise() twice should only create one BackgroundPlotter."""
        self._view.initialise()
        self._view.initialise()
        mock_bg_plotter_cls.assert_called_once()

    def test_overlay_rectangle_no_op_when_no_plotter(self):
        """overlay_rectangle should do nothing if the plotter has not been created."""
        self._view.overlay_rectangle()
        self.assertIsNone(self._view.shape_overlay_manager)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_overlay_rectangle_creates_shape_overlay_manager(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        self._view.overlay_rectangle()
        mock_manager_cls.assert_called_once_with(self._view.main_plotter)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_overlay_rectangle_reuses_existing_manager(self, mock_bg_plotter_cls, mock_manager_cls):
        """If a ShapeOverlayManager already exists it should not be recreated."""
        self._view.initialise()
        self._view.overlay_rectangle()
        self._view.overlay_rectangle()
        mock_manager_cls.assert_called_once()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_overlay_rectangle_adds_rectangle_shape(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        self._view.overlay_rectangle()
        mock_instance = mock_manager_cls.return_value
        mock_instance.add_shape.assert_called_once()
        shape_arg = mock_instance.add_shape.call_args[0][0]
        self.assertIsInstance(shape_arg, RectangleSelectionShape)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_overlay_rectangle_registers_callback(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        callback = MagicMock()
        self._view.overlay_rectangle(on_shape_changed=callback)
        mock_instance = mock_manager_cls.return_value
        mock_instance.set_on_shape_changed.assert_called_once_with(callback)

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_overlay_rectangle_no_callback_skips_set(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        self._view.overlay_rectangle(on_shape_changed=None)
        mock_instance = mock_manager_cls.return_value
        mock_instance.set_on_shape_changed.assert_not_called()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_remove_shape_calls_manager_remove(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        self._view.overlay_rectangle()
        mock_instance = mock_manager_cls.return_value
        self._view.remove_shape()
        mock_instance.remove_shape.assert_called_once()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.ShapeOverlayManager")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_remove_shape_clears_manager_reference(self, mock_bg_plotter_cls, mock_manager_cls):
        self._view.initialise()
        self._view.overlay_rectangle()
        self._view.remove_shape()
        self.assertIsNone(self._view.shape_overlay_manager)

    def test_remove_shape_no_op_when_no_manager(self):
        """remove_shape should not raise when there is no active manager."""
        self._view.remove_shape()  # no exception expected

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    @mock.patch("qtpy.QtWidgets.QWidget.closeEvent")
    def test_close_event_closes_plotter(self, mock_close_event, mock_bg_plotter_cls):
        self._view.initialise()
        self._view.closeEvent(MagicMock())
        self._view.main_plotter.close.assert_called_once()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    @mock.patch("qtpy.QtWidgets.QWidget.closeEvent")
    def test_close_event_calls_super(self, mock_close_event, mock_bg_plotter_cls):
        self._view.initialise()
        self._view.closeEvent(MagicMock())
        mock_close_event.assert_called_once()

    @mock.patch("qtpy.QtWidgets.QWidget.closeEvent")
    def test_close_event_no_plotter_does_not_raise(self, mock_close_event):
        """Closing before initialise() should not raise."""
        self._view.closeEvent(MagicMock())

    @mock.patch("qtpy.QtWidgets.QWidget.resizeEvent")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_resize_event_starts_debounce_timer(self, mock_bg_plotter_cls, _mock_super_resize):
        self._view.initialise()
        self._view._resize_timer = MagicMock()
        self._view.resizeEvent(MagicMock())
        self._view._resize_timer.start.assert_called_once()

    @mock.patch("qtpy.QtWidgets.QWidget.resizeEvent")
    def test_resize_event_no_op_when_no_plotter(self, _mock_super_resize):
        """resizeEvent should not start the timer before the plotter is created."""
        self._view._resize_timer = MagicMock()
        self._view.resizeEvent(MagicMock())
        self._view._resize_timer.start.assert_not_called()

    @mock.patch("qtpy.QtWidgets.QWidget.resizeEvent")
    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_resize_event_does_not_call_callback_immediately(self, mock_bg_plotter_cls, _mock_super_resize):
        """The callback must not be invoked directly inside resizeEvent."""
        self._view.initialise()
        callback = MagicMock()
        self._view.set_on_resize_callback(callback)
        self._view.resizeEvent(MagicMock())
        callback.assert_not_called()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_on_resize_finished_calls_registered_callback(self, mock_bg_plotter_cls):
        self._view.initialise()
        callback = MagicMock()
        self._view.set_on_resize_callback(callback)
        self._view._on_resize_finished()
        callback.assert_called_once()

    @mock.patch("instrumentview.isisreflectometry.ReflectometryInstrumentViewView.BackgroundPlotter")
    def test_on_resize_finished_calls_reset_camera_when_no_callback(self, mock_bg_plotter_cls):
        self._view.initialise()
        self._view._on_resize_finished()
        self._view.main_plotter.reset_camera.assert_called_once()

    def test_on_resize_finished_no_op_when_no_plotter(self):
        """_on_resize_finished should not raise if the plotter has not been created."""
        self._view._on_resize_finished()  # no exception expected


if __name__ == "__main__":
    unittest.main()

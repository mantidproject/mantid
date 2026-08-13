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
from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from instrumentview.ShapeWidgets import RectangleSelectionShape


@start_qapplication
class TestALFInstrumentViewView(unittest.TestCase):
    # Figure is mocked so that the line plot does not need the "mantid" matplotlib projection
    @mock.patch("instrumentview.FullInstrumentViewWindow.Figure")
    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    def setUp(self, mock_plotter, mock_splitter_add_widget, mock_v_add_widget, mock_h_add_widget, mock_figure_canvas, mock_figure) -> None:
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            self._view = ALFInstrumentViewView()
        self._view._presenter = MagicMock()
        self._view.setup_connections_to_presenter()

    def test_add_roi_button_is_a_checkable_and_enabled_shape_control(self):
        self.assertEqual(self._view._add_selection.text(), "Add ROI")
        self.assertTrue(self._view._add_selection.isCheckable())
        self.assertTrue(self._view._add_selection.isEnabled())
        self.assertFalse(self._view._add_selection.isChecked())

    def test_checking_add_roi_overlays_a_rectangle(self):
        self._view._add_selection.setChecked(True)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, RectangleSelectionShape)

    def test_checking_add_roi_registers_shape_changed_callback(self):
        self._view._add_selection.setChecked(True)
        self.assertEqual(self._view._shape_overlay_manager._on_shape_changed, self._view._presenter.on_roi_shape_changed)

    def test_checking_add_roi_selects_the_region_under_the_new_rectangle(self):
        self._view._add_selection.setChecked(True)
        self._view._presenter.on_roi_shape_changed.assert_called_once()

    def test_unchecking_add_roi_removes_the_rectangle(self):
        self._view._add_selection.setChecked(True)
        self._view._add_selection.setChecked(False)
        self.assertIsNone(self._view._shape_overlay_manager)

    def test_add_roi_stays_enabled_when_no_shape_is_overlaid(self):
        self._view.set_add_selection_and_mask_buttons_enabled(False)
        self.assertTrue(self._view._add_selection.isEnabled())

    def test_disabling_shape_controls_unchecks_and_disables_add_roi(self):
        self._view._add_selection.setChecked(True)
        self._view.set_overlaid_shape_controls_enabled(False)
        self.assertFalse(self._view._add_selection.isChecked())
        self.assertFalse(self._view._add_selection.isEnabled())
        self.assertIsNone(self._view._shape_overlay_manager)

    def test_enabling_shape_controls_enables_add_roi(self):
        self._view.set_overlaid_shape_controls_enabled(False)
        self._view.set_overlaid_shape_controls_enabled(True)
        self.assertTrue(self._view._add_selection.isEnabled())

    def test_set_overlaid_shape_controls_checked_drives_add_roi(self):
        self._view.set_overlaid_shape_controls_checked(True)
        self.assertTrue(self._view._add_selection.isChecked())
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self._view.set_overlaid_shape_controls_checked(False)
        self.assertFalse(self._view._add_selection.isChecked())
        self.assertIsNone(self._view._shape_overlay_manager)


if __name__ == "__main__":
    unittest.main()

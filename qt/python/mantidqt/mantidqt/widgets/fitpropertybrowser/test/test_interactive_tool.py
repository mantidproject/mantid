# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#


import unittest
from unittest.mock import MagicMock, patch

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser.interactive_tool import FitInteractiveTool


@start_qapplication
class FitInteractiveToolTest(unittest.TestCase):

    @patch('mantidqt.plotting.markers.VerticalMarker')
    def setUp(self, mock_v_marker) -> None:
        mock_v_marker._get_y0_y1 = MagicMock(return_value=(0, 1))
        canvas = MagicMock()
        toolbar_manager = MagicMock()
        current_peak_type = MagicMock()
        self.interactor = FitInteractiveTool(canvas, toolbar_manager, current_peak_type)

    def tearDown(self) -> None:
        del self.interactor

    def cursor_hover_over_marker_test_helper(self, above, expected_cursor):
        with patch('mantidqt.plotting.markers.SingleMarker.is_inside_bounds', return_value=(True, 1)):
            with patch('mantidqt.plotting.markers.RangeMarker.get_range', return_value=[0, 10]):
                # is_above = True -> cursor is hovering over a marker
                # is_above = False -> cursor is not hovering ove a marker
                with patch('mantidqt.plotting.markers.SingleMarker.is_above', return_value=above):
                    event = MagicMock()
                    event.xdata = 1
                    event.ydata = 2
                    event.button = None

                    self.interactor.toolbar_manager.is_tool_active = MagicMock(return_value=False)
                    self.interactor.motion_notify_callback(event)

                    self.assertEqual(expected_cursor, QApplication.overrideCursor().shape())

    def test_hover_over_marker_sets_override_cursor(self):
        expected_cursor = Qt.SizeHorCursor
        self.cursor_hover_over_marker_test_helper(True, expected_cursor)

    def test_not_hover_over_marker_restores_override_cursor(self):
        expected_cursor = Qt.ArrowCursor

        # set twice since restoreOverrideCursor returns the previous cursor set
        QApplication.setOverrideCursor(expected_cursor)
        QApplication.setOverrideCursor(expected_cursor)

        self.cursor_hover_over_marker_test_helper(False, expected_cursor)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
import unittest
from unittest.mock import MagicMock, call

from mantidqt.widgets.sliceviewer.cursor import CursorTracker


class CursorTrackerTest(unittest.TestCase):
    def setUp(self):
        self.mock_axes = MagicMock()

    def test_construction_subscribes_to_expected_events_if_autoconnect_default(self):
        tracker = CursorTracker(image_axes=self.mock_axes)

        canvas = self.mock_axes.figure.canvas
        canvas.mpl_connect.assert_has_calls(
            (call('motion_notify_event',
                  tracker.on_mouse_move), call('axes_leave_event', tracker.on_mouse_leave)),
            any_order=True)

    def test_construction_subscribes_to_expected_events_if_autoconnect_True(self):
        tracker = CursorTracker(image_axes=self.mock_axes, autoconnect=True)

        canvas = self.mock_axes.figure.canvas
        canvas.mpl_connect.assert_has_calls(
            (call('motion_notify_event',
                  tracker.on_mouse_move), call('axes_leave_event', tracker.on_mouse_leave)),
            any_order=True)

    def test_on_mouse_move_notifies_if_inaxes(self):
        tracker = _create_tracker(self.mock_axes)
        mock_event = MagicMock(xdata=0.0, ydata=1.0, inaxes=self.mock_axes)

        tracker.on_mouse_move(mock_event)

        tracker.on_cursor_at.assert_called_once_with(0.0, 1.0)

    def test_on_mouse_move_does_nothing_if_not_inaxes(self):
        tracker = _create_tracker(self.mock_axes)
        mock_event = MagicMock(xdata=0.0, ydata=1.0, inaxes=MagicMock())

        tracker.on_mouse_move(mock_event)

        tracker.on_cursor_at.assert_not_called()

    def test_on_mouse_leave_calls_on_cursor_leave(self):
        tracker = _create_tracker(self.mock_axes)
        mock_event = MagicMock()

        tracker.on_mouse_leave(mock_event)

        tracker.on_cursor_outside_axes.assert_called_once()

    def test_disconnect_removes_connections(self):
        tracker = _create_tracker(self.mock_axes)
        move_cid, leave_cid = tracker._mouse_move_cid, tracker._mouse_outside_cid

        tracker.disconnect()

        canvas = self.mock_axes.figure.canvas
        canvas.mpl_disconnect.assert_has_calls((call(move_cid), call(leave_cid)), any_order=True)


def _create_tracker(axes):
    tracker = CursorTracker(image_axes=axes)
    tracker.on_cursor_at = MagicMock()
    tracker.on_cursor_outside_axes = MagicMock()

    return tracker


if __name__ == '__main__':
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

try:
    from unittest import MagicMock, patch
except ImportError:
    from mock import MagicMock, patch

from mantidqt.utils.qt.testing import GuiTest

from workbench.plotting.figuremanager import FigureCanvasQTAgg, FigureManagerWorkbench


class MockLine2d(object):
    def __init__(self, y):
        self.y = y

    def get_ydata(self):
        return self.y


class FigureManagerWorkbenchTest(GuiTest):

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_construction(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertNotEqual(fig_mgr, None)

    @patch("workbench.plotting.qappthreadcall.QAppThreadCall")
    def test_can_get_bounds_of_line_in_container(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)

        mock_line2d = MockLine2d([0.0, 5.0, -2.5])
        mock_lower_caps = MockLine2d([-0.1, 4.99, -2.6])
        mock_upper_caps = MockLine2d([0.1, 5.1, -2.4])
        mock_container = (mock_line2d, (mock_lower_caps, mock_upper_caps))

        min, max = fig_mgr.get_bounds_from_container(mock_container, False)
        self.assertEqual(min, -2.5)
        self.assertEqual(max, 5.0)

    @patch("workbench.plotting.qappthreadcall.QAppThreadCall")
    def test_can_get_errors_as_bounds_when_errors_are_largest(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)

        mock_line2d = MockLine2d([0.0, 5.0, -2.5])
        mock_lower_caps = MockLine2d([-0.1, 4.99, -2.6])
        mock_upper_caps = MockLine2d([0.1, 5.1, -2.4])
        mock_container = (mock_line2d, (mock_lower_caps, mock_upper_caps))

        min, max = fig_mgr.get_bounds_from_container(mock_container, True)
        self.assertEqual(min, -2.6)
        self.assertEqual(max, 5.1)

    @patch("workbench.plotting.qappthreadcall.QAppThreadCall")
    def test_can_get_lines_as_bounds_when_lines_are_largest(self, mock_qappthread):
        """Test that error bounds are not returned if line points are
        more extreme, even if we've selected to include errors"""
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)

        mock_line2d = MockLine2d([0.0, 5.0, -2.5])
        mock_lower_caps = MockLine2d([-0.1, 4.99])
        mock_upper_caps = MockLine2d([0.1, -2.4])
        mock_container = (mock_line2d, (mock_lower_caps, mock_upper_caps))

        min, max = fig_mgr.get_bounds_from_container(mock_container, True)
        self.assertEqual(min, -2.5)
        self.assertEqual(max, 5.0)


if __name__ == "__main__":
    unittest.main()

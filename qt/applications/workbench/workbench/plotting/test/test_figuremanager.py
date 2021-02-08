# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from unittest.mock import MagicMock, patch
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figuremanager import MantidFigureCanvas, FigureManagerWorkbench


@start_qapplication
class FigureManagerWorkbenchTest(unittest.TestCase):

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_construction(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = MantidFigureCanvas(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertNotEqual(fig_mgr, None)

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_window_title(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = MantidFigureCanvas(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertEqual(fig_mgr.get_window_title(), "Figure 1")


if __name__ == "__main__":
    unittest.main()

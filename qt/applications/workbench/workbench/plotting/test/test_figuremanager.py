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


class FigureManagerWorkbenchTest(GuiTest):

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_construction(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertNotEqual(fig_mgr, None)


if __name__ == "__main__":
    unittest.main()

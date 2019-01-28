import unittest
try:
    from unittest import MagicMock, patch
except ImportError:
    from mock import MagicMock, patch

from mantidqt.utils.qt.test import GuiTest

from workbench.plotting.figuremanager import FigureCanvasQTAgg, FigureManagerWorkbench


class FigureManagerWorkbenchTest(GuiTest):

    @patch("workbench.plotting.qappthreadcall.QAppThreadCall")
    def test_construction(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        canvas = FigureCanvasQTAgg(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertTrue(fig_mgr is not None)


if __name__ == "__main__":
    unittest.main()

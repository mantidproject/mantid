# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest
import matplotlib.pyplot as plt

from unittest.mock import MagicMock, patch
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figuremanager import MantidFigureCanvas, FigureManagerWorkbench


@start_qapplication
class FigureManagerWorkbenchTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5, 3, 5],
                                         DataE=[1, 2, 3, 4, 1, 1],
                                         NSpec=3,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace('ws2d_histo')

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_construction(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        fig.bbox.max = [1, 1]
        canvas = MantidFigureCanvas(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertNotEqual(fig_mgr, None)

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_window_title(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        fig = MagicMock()
        fig.bbox.max = [1, 1]
        canvas = MantidFigureCanvas(fig)
        fig_mgr = FigureManagerWorkbench(canvas, 1)
        self.assertEqual(fig_mgr.get_window_title(), "Figure 1")

    def test_reverse_axes_lines(self):
        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
        spec2 = ax.plot(self.ws2d_histo, 'rs', specNum=2, linewidth=6)
        spec3 = ax.plot(self.ws2d_histo, 'rs', specNum=3, linewidth=6)
        spec1 = ax.plot(self.ws2d_histo, 'rs', specNum=1, linewidth=6)
        self.assertEqual(ax.get_lines()[0], spec2[0])
        self.assertEqual(ax.get_lines()[1], spec3[0])
        self.assertEqual(ax.get_lines()[2], spec1[0])
        FigureManagerWorkbench._reverse_axis_lines(ax)
        self.assertEqual(ax.get_lines()[0], spec1[0])
        self.assertEqual(ax.get_lines()[1], spec3[0])
        self.assertEqual(ax.get_lines()[2], spec2[0])


if __name__ == "__main__":
    unittest.main()

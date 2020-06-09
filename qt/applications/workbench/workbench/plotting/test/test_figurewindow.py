# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
from mantid import plots  # noqa: F401  # need mantid projection
from unittest.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figurewindow import FigureWindow

matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt  # noqa: E402


@start_qapplication
class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Patch the show method on MainWindow so we don't get GUI pop-ups
        cls.show_patch = patch('workbench.plotting.figurewindow.QMainWindow.show')
        cls.show_patch.start()

        cls.fig, axs = plt.subplots(1, 2, subplot_kw={'projection': 'mantid'})
        axs[0].plot([0, 1], [1, 0])
        axs[1].plot([0, 2], [2, 0])
        cls.fig_window = FigureWindow(cls.fig.canvas)
        cls.ws = CreateWorkspace(DataX=[0, 3],
                                 DataY=[3, 0],
                                 DataE=[1, 1],
                                 NSpec=1,
                                 OutputWorkspace='ws')

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()
        cls.show_patch.stop()

    def test_drag_and_drop_adds_plot_to_correct_axes(self):
        ax = self.fig.get_axes()[1]
        dpi_ratio = self.fig.canvas.devicePixelRatio() or 1
        # Find the center of the axes and simulate a drop event there
        # Need to use Qt logical pixels to factor in dpi
        ax_x_centre = (ax.xaxis.clipbox.min[0] + ax.xaxis.clipbox.width*0.5)/dpi_ratio
        ax_y_centre = (ax.xaxis.clipbox.min[1] + ax.xaxis.clipbox.height*0.5)/dpi_ratio
        mock_event = Mock(pos=lambda: Mock(x=lambda: ax_x_centre, y=lambda: ax_y_centre))
        mock_event.mimeData().text.return_value = "ws"
        with patch('workbench.plotting.figurewindow.QMainWindow.dropEvent'):
            self.fig_window.dropEvent(mock_event)

        self.assertEqual(2, len(ax.lines))


if __name__ == '__main__':
    unittest.main()

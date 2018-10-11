# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy import PYQT4, PYQT5
from qtpy.QtWidgets import QSizePolicy

import matplotlib
backend = matplotlib.get_backend()
if backend.startswith('module://'):
    if backend.endswith('qt4agg'):
        backend = 'Qt4Agg'
    elif backend.endswith('workbench') or backend.endswith('qt5agg'):
            backend = 'Qt5Agg'
else:  # is this part necessary?
    if PYQT4:
        backend = 'Qt4Agg'
    elif PYQT5:
        backend = 'Qt5Agg'

if backend == 'Qt4Agg':
    from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
elif backend == 'Qt5Agg':
    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
else:
    raise RuntimeError('Unrecognized backend {}'.format(backend))

from matplotlib.figure import Figure


class MplFigureCanvas(FigureCanvas):
    """  A customized Qt widget for matplotlib figure.
    It can be used to replace GraphicsView of QtGui
    """

    def __init__(self, parent):
        """  Initialization
        """
        # Instantialize matplotlib Figure
        self.fig = Figure()
        self.axes = self.fig.add_subplot(111)

        # Initialize parent class and set parent
        FigureCanvas.__init__(self, self.fig)
        self.setParent(parent)

        # Set size policy to be able to expanding and resizable with frame
        FigureCanvas.setSizePolicy(self, QSizePolicy.Expanding,
                                   QSizePolicy.Expanding)

        FigureCanvas.updateGeometry(self)

        return

    def plot(self, x, y):
        """ Plot a set of data
        Argument:
        - x: numpy array X
        - y: numpy array Y
        """
        self.x = x
        self.y = y
        self.axes.plot(self.x, self.y)

        return

    def getPlot(self):
        """ return figure's axes to expose the matplotlib figure to PyQt client
        """
        return self.axes

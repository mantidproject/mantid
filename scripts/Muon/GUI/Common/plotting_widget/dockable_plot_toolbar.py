# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from matplotlib.backends.qt_compat import is_pyqt5

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import (
        NavigationToolbar2QT as NavigationToolbar)
else:
    from matplotlib.backends.backend_qt4agg import (
        NavigationToolbar2QT as NavigationToolbar)


class DockablePlotToolbar(NavigationToolbar):
    def __init__(self, figure_canvas, parent=None):
        self.toolitems = (('Home', 'Reset original view', 'home', 'home'),
                          ('Back', 'Back to previous view', 'back', 'back'),
                          ('Forward', 'Forward to next view', 'forward', 'forward'),
                          (None, None, None, None),
                          ('Pan', 'Pan axes with left mouse, zoom with right', 'move', 'pan'),
                          ('Zoom', 'Zoom to rectangle', 'zoom_to_rect', 'zoom'),
                          (None, None, None, None),
                          ('Subplots', 'Edit subplots', 'subplots', 'configure_subplots'),
                          ('Save', 'Save the figure', 'filesave', 'save_figure'),
                          (None, None, None, None),
                          ('Show/hide legend', 'Toggles the legend on/off', "select", 'toggle_legend'),
                          )

        NavigationToolbar.__init__(self, figure_canvas, parent=parent)

    def toggle_legend(self):
        for ax in self.canvas.figure.get_axes():
            if ax.get_legend() is not None:
                ax.get_legend().set_visible(not ax.get_legend().get_visible())
        self.canvas.figure.tight_layout()
        self.canvas.draw()

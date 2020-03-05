# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from matplotlib.backends.qt_compat import is_pyqt5
from mantidqt.icons import get_icon
from qtpy import QtWidgets, QtCore

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT
else:
    from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT


class FittingPlotToolbar(NavigationToolbar2QT):
    sig_home_clicked = QtCore.Signal()

    toolitems = (
        ('Home', 'Center display on contents', 'mdi.home', 'on_home_clicked', None),
        ('Back', 'Back to previous view', 'mdi.arrow-left', 'back', None),
        ('Forward', 'Forward to next view', 'mdi.arrow-right', 'forward', None),
        (None, None, None, None, None),
        ('Pan', 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan', False),
        ('Zoom', 'Zoom to rectangle', 'mdi.magnify', 'zoom', False)
    )

    def _init_toolbar(self):
        for text, tooltip_text, mdi_icon, callback, checked in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                action = self.addAction(get_icon(mdi_icon), text, getattr(self, callback))
                self._actions[callback] = action
                if checked is not None:
                    action.setCheckable(True)
                    action.setChecked(checked)
                if tooltip_text is not None:
                    action.setToolTip(tooltip_text)

        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(24 * dpi_ratio, 24 * dpi_ratio))

    def on_home_clicked(self):
        self.sig_home_clicked.emit()
        self.push_current()

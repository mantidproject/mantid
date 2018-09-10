from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal


class InstrumentWidgetView(QtGui.QWidget):

    def __init__(self, parent=None):
        super(InstrumentWidgetView, self).__init__(parent)

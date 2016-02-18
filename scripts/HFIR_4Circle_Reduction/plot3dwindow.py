__author__ = 'wzz'

import os
import sys

from PyQt4 import QtCore, QtGui

import ui_View3DWidget


class Plot3DWindow(QtGui.QMainWindow):
    """

    """
    def __init__(self, parent=None):
        """

        :param parent:
        :return:
        """
        # Init
        QtGui.QMainWindow.__init__(self, parent)

        self.ui = ui_View3DWidget.Ui_Form()
        self.ui.setupUi(self)

        return

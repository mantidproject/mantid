# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Presenter import Presenter
from Model import Model
from View import View

import PyQt5.QtWidgets as QtWidgets

import sys


class Demo(QtWidgets.QMainWindow):
    """ Wrapper class for setting the main window"""
    def __init__(self, parent=None):
        super(Demo, self).__init__(parent)

        self.window = QtWidgets.QMainWindow()
        demo_view = View()
        demo_model = Model()
        # create presenter
        self.presenter = Presenter(demo_view, demo_model)
        # set the view for the main window
        self.setCentralWidget(demo_view)
        self.setWindowTitle("MVP Demo")


def qapp():
    if QtWidgets.QApplication.instance():
        _app = QtWidgets.QApplication.instance()
    else:
        _app = QtWidgets.QApplication(sys.argv)
    return _app


if __name__ == "__main__":
    app = qapp()
    window = Demo()
    window.show()
    app.exec_()

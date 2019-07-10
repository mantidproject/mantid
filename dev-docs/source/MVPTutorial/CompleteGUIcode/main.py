from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets

import sys

import model_colour
import model_data
import master_view
import master_presenter

"""
A wrapper class for setting the main window
"""


class Demo(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super(Demo, self).__init__(parent)

        data_model = model_data.DataGenerator()
        colour_list = model_colour.line_colours()

        self.window = QtWidgets.QMainWindow()

        my_view = master_view.MasterView(parent=self)
        self.master_presenter = master_presenter.MasterPresenter(my_view, data_model, colour_list)

        # set the view for the main window
        self.setCentralWidget(my_view)
        self.setWindowTitle("view tutorial")


def qapp():
    if QtWidgets.QApplication.instance():
        _app = QtWidgets.QApplication.instance()
    else:
        _app = QtWidgets.QApplication(sys.argv)
    return _app


app = qapp()
window = Demo()
window.show()
app.exec_()

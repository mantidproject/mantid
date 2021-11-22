# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

import sys

import model_colour
import model_data
import main_view
import main_presenter

"""
A wrapper class for setting the main window
"""


class Demo(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)

        data_model = model_data.DataGenerator()
        colour_list = model_colour.line_colours()

        self.window = QtWidgets.QMainWindow()

        my_view = main_view.MainView(parent=self)
        self.main_presenter = main_presenter.MainPresenter(my_view, data_model, colour_list)

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

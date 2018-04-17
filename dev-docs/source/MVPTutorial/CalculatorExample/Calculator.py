from Presenter import Presenter
from Model import Model
from View import View

import PyQt4.QtGui as QtGui

import sys


class Demo(QtGui.QMainWindow):
    """ Wrapper class for setting the main window"""
    def __init__(self, parent=None):
        super(Demo, self).__init__(parent)

        self.window = QtGui.QMainWindow()
        demo_view = View()
        demo_model = Model()
        # create presenter
        self.presenter = Presenter(demo_view, demo_model)
        # set the view for the main window
        self.setCentralWidget(demo_view)
        self.setWindowTitle("MVP Demo")


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


if __name__ == "__main__":
    app = qapp()
    window = Demo()
    window.show()
    app.exec_()

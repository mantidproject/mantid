"""
    Defines the entry point for the GUI app
"""
import lib1to2.messages as messages
from PyQt4.QtGui import QApplication
from mainwindow import MainWindow

def _qtapp():
    """Returns a QApplication object
    If one does not already  then one is created
    """
    qapp = QApplication.instance()
    if qapp is None:
        qapp = QApplication([])
    return qapp

def start(options, args):
    """Starts the GUI and passes the command line
       arguments along to the GUI
       @param options A dictionary of the options passed to the migration
       @param args The positional command line arguments
       load into the GUI
    """
    messages.notify("Starting GUI")
    app = _qtapp()
    window = MainWindow()
    window.show()
    return app.exec_()
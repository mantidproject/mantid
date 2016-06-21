"""
    The main window for the GUI
"""
from PyQt4.QtCore import SIGNAL, SLOT, pyqtSlot
from PyQt4.QtGui import *
from ui_mainwindow import Ui_MainWindow

class MainWindow(QMainWindow, Ui_MainWindow):
    """The main window for the application
    """

    def __init__(self, parent = None):
        """
        Constructs and lays out the main window
        """
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.connect(self.closeAction, SIGNAL("triggered()"),
                     qApp, SLOT("quit()"))

    @pyqtSlot()
    def chooseFiles():
        """Opens a file browser to allow a user
        to select files for migration.
        Emits the filesSelected() signal if any files
        are selected
        """
        pass

    @pyqtSlot()
    def addToTable(files):
        """
        Adds files to the table, setting the
        status to
        """
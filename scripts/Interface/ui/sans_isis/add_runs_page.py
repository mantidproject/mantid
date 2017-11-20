from PyQt4 import QtGui
from PyQt4.QtCore import pyqtSignal
import ui_add_runs_page


class AddRunsPage(QtGui.QWidget, ui_add_runs_page.Ui_AddRunsPage):
    sum = pyqtSignal()

    def __init__(self, parent=None):
        super(AddRunsPage, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def _connect_signals(self):
        self.sumButton.pressed.connect(self.sum)

    def run_selector_view(self):
        return self.run_selector

    def summation_settings_view(self):
        return self.summation_settings_view

    def setupUi(self, other):
        ui_add_runs_page.Ui_AddRunsPage.setupUi(self, other)

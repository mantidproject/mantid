from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal

import ui_add_runs_page
from sans.gui_logic.presenter.run_selector_presenter import RunSelectorPresenter
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter

class AddRunsPage(QtGui.QWidget, ui_add_runs_page.Ui_AddRunsPage):
    sum = pyqtSignal()

    def __init__(self, parent=None):
        super(AddRunsPage, self).__init__(parent)
        self.setupUi(self)
        self._connect_signals()

    def _connect_signals(self):
        self.sumButton.pressed.connect(self.sum)

    def setupUi(self, other):
        ui_add_runs_page.Ui_AddRunsPage.setupUi(self, other)

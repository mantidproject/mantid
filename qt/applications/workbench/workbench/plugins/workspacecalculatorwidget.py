# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
#
from qtpy.QtWidgets import QVBoxLayout, QSizePolicy

from mantidqt.widgets.workspacecalculator.presenter import WorkspaceCalculator
from workbench.plugins.base import PluginWidget


class WorkspaceCalculatorWidget(PluginWidget):
    """Provides a Workspace Widget for workspace manipulation"""
    def __init__(self, parent):
        super(WorkspaceCalculatorWidget, self).__init__(parent)

        # layout
        self.workspacecalculatorwidget = WorkspaceCalculator(parent)
        layout = QVBoxLayout()
        layout.addWidget(self.workspacecalculatorwidget.view)
        layout.sizeHint()
        self.view = self.workspacecalculatorwidget.view
        self.setLayout(layout)
        self.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Maximum)

    # ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Workspace Calculator"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

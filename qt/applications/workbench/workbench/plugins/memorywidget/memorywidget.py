# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtWidgets import QVBoxLayout

from ..base import PluginWidget
from ..memorywidget.memoryview import MemoryView
from ..memorywidget.memorypresenter import MemoryPresenter


class MemoryWidget(PluginWidget):
    def __init__(self, parent, view=None):
        super(MemoryWidget, self).__init__(parent)

        self.view = view if view else MemoryView(self)
        self.presenter = MemoryPresenter(self.view)

        layout = QVBoxLayout()
        layout.addWidget(self.view.memory_bar)

        self.setLayout(layout)
        self.setWindowTitle(self.get_plugin_title())
        # 70 is chosen as a good value after testing
        # how it looks for different values
        self.setMaximumHeight(70)

    # ----------------- Plugin API --------------------

    def get_plugin_title(self):
        return "Memory Usage"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

    def register_plugin(self, menu=None):
        self.main.add_dockwidget(self)

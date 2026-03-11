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

from ..plugins.base import PluginWidget
from mantidqt.widgets.memorywidget.memoryview import MemoryView
from mantidqt.widgets.memorywidget.memorypresenter import MemoryPresenter


class MemoryWidget(PluginWidget):
    """
    Widget to display system memory usage
    """

    def __init__(self, parent, view=None):
        super(MemoryWidget, self).__init__(parent)

        self.view = view or MemoryView(self)
        self.presenter = MemoryPresenter(self.view)

        layout = QVBoxLayout()
        self.view.mantid_memory_bar.setMinimumHeight(30)
        layout.addWidget(self.view.mantid_memory_bar)
        self.view.memory_bar.setMinimumHeight(30)
        layout.addWidget(self.view.memory_bar)

        self.setLayout(layout)
        self.setWindowTitle(self.get_plugin_title())
        # The following are chosen as a good value after
        # testing how it looks for different values
        self.setMinimumHeight(80)
        self.setMaximumHeight(85)

    # ----------------- Plugin API --------------------

    def get_plugin_title(self):
        return "System Memory Usage"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

    def register_plugin(self, menu=None):
        self.main.add_dockwidget(self)

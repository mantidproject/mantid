# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
# system imports

# third-party library imports
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.globalfiguremanager import GlobalFigureManager
from mantidqt.widgets.rawdataexplorer.presenter import RawDataExplorerPresenter


class RawDataExplorer(PluginWidget):
    """TODO"""

    def __init__(self, parent):
        super(RawDataExplorer, self).__init__(parent)

        self.raw_data_explorer_presenter = RawDataExplorerPresenter()

        # layout
        self.raw_data_explorer_widget = self.raw_data_explorer_presenter.view
        layout = QVBoxLayout()
        layout.addWidget(self.raw_data_explorer_widget)
        self.setLayout(layout)

# ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "RDExp"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

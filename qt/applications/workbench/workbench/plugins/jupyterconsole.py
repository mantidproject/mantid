# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from ..config.fonts import text_font
from ..plugins.base import PluginWidget

# system imports
import warnings

# Must be called before trying to import module with a deprecation warning
warnings.filterwarnings(action="ignore", category=DeprecationWarning, module="ipykernel")
warnings.filterwarnings(action="ignore", category=DeprecationWarning, module=".*jupyter.*")

from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole  # noqa: E402


# should we share this with plugins.editor?
STARTUP_CODE = """
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np
"""


class JupyterConsole(PluginWidget):
    """Provides an in-process Jupyter Qt-based console"""

    def __init__(self, parent):
        super(JupyterConsole, self).__init__(parent)

        # layout
        font = text_font()
        self.console = InProcessJupyterConsole(self, startup_code=STARTUP_CODE, font_family=font.family(), font_size=font.pointSize())
        layout = QVBoxLayout()
        layout.addWidget(self.console)
        self.setLayout(layout)

    # ----------------- Plugin API --------------------

    def get_plugin_title(self):
        return "IPython"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass

    def register_plugin(self, menu=None):
        self.main.add_dockwidget(self)

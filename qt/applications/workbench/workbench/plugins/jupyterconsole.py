# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
# system imports
import warnings

warnings.filterwarnings(action="ignore", category=DeprecationWarning, module="ipykernel")
warnings.filterwarnings(action="ignore", category=DeprecationWarning, module=".*jupyter.*")

# third-party library imports
from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole  # noqa: E402
from qtpy.QtWidgets import QVBoxLayout  # noqa: E402

# local package imports
from ..config.fonts import text_font  # noqa: E402
from ..plugins.base import PluginWidget  # noqa: E402

# from mantidqt.utils.qt import toQSettings when readSettings/writeSettings are implemented

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

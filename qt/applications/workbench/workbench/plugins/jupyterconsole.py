# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

# system imports
import sys

# third-party library imports
from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole
try:
    from IPython.core.usage import quick_guide
except ImportError:  # quick_guide was removed in IPython 6.0
    quick_guide = ''
from IPython.core.usage import release as ipy_release
from matplotlib import __version__ as mpl_version
from numpy.version import version as np_version
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
# from mantidqt.utils.qt import toQSettings when readSettings/writeSettings are implemented

DEFAULT_BANNER_PARTS = [
    'IPython {version} -- An enhanced Interactive Python.\n'.format(
        version=ipy_release.version,
        ),
    quick_guide,
    '\nPython {}, numpy {}, matplotlib {}\n'.format(sys.version.split('\n')[0].strip(), np_version, mpl_version),
    'Type "copyright", "credits" or "license" for more information.\n',
]

BANNER = ''.join(DEFAULT_BANNER_PARTS)

# should we share this with plugins.editor?
STARTUP_CODE = """from __future__ import (absolute_import, division, print_function, unicode_literals)
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np
"""


class JupyterConsole(PluginWidget):
    """Provides an in-process Jupyter Qt-based console"""

    def __init__(self, parent):
        super(JupyterConsole, self).__init__(parent)

        # layout
        self.console = InProcessJupyterConsole(self, banner=BANNER,
                                               startup_code=STARTUP_CODE)
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

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name, unused-import

"""
Module to import and run the PyChop GUI for use either on the commandline or as a MantidPlot interface
"""

import sys
from mantidqtinterfaces.PyChop import PyChopGui
try:
    from mantidqt.gui_helper import set_matplotlib_backend, get_qapplication
except ImportError:
    within_mantid = False
    from qtpy.QtWidgets import QApplication
    app = QApplication(sys.argv)
    parent, flags = None, None
else:
    set_matplotlib_backend()  # must be called before anything tries to use matplotlib
    app, within_mantid = get_qapplication()
    if 'workbench' in sys.modules:
        from workbench.config import get_window_config
        parent, flags = get_window_config()
    else:
        parent, flags = None, None

window = PyChopGui.PyChopGui(parent, flags)
window.show()
if not within_mantid:
    sys.exit(app.exec_())

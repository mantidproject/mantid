# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name, unused-import

"""
Module to import and run the PyChop GUI for use either on the commandline or as a MantidPlot interface
"""

import sys
from PyChop import PyChopGui
from mantidqt.gui_helper import set_matplotlib_backend, get_qapplication
set_matplotlib_backend()  # must be called before anything tries to use matplotlib

app, within_mantid = get_qapplication()
window = PyChopGui.PyChopGui()
window.show()
if not within_mantid:
    sys.exit(app.exec_())

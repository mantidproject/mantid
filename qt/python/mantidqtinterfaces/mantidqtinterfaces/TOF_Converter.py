# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import sys
from mantidqtinterfaces.TofConverter import converterGUI
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()

if "workbench" in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None
reducer = converterGUI.MainWindow(parent, flags)  # the main ui class in this file is called MainWindow
reducer.show()
if not within_mantid:
    app.exec_()

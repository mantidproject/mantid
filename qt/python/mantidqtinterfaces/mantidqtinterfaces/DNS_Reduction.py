# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""
GUI for reduction of elastic and TOF data at the DNS instrument MLZ.
"""

import sys
import os

# Remove script path from sys.path, which is automatically added in python 3.
# Otherwise, Muon and Engineering modules are not found if called from the
# command line
sys.path.pop(0)

from mantidqt.gui_helper import get_qapplication
from qtpy import QtGui, QtWidgets

from mantidqtinterfaces.dns_powder_tof.main_widget import DNSReductionGuiWidget

app, within_mantid = get_qapplication()
reducer_widget = DNSReductionGuiWidget(name="DNS-Reduction", app=app, within_mantid=within_mantid)
view = reducer_widget.view
view.setWindowTitle(view.modus_titles[reducer_widget.modus.name])
screenShape = QtWidgets.QDesktopWidget().screenGeometry()
view.resize(int(screenShape.width() * 0.6), int(screenShape.height() * 0.6))
app_dir = os.path.dirname(__file__)
view.show()
if not within_mantid:
    app.setWindowIcon(QtGui.QIcon(f"{app_dir}/dns_powder_tof/dns_icon.png"))
    sys.exit(app.exec_())
else:
    app.setWindowIcon(QtGui.QIcon(":/images/MantidIcon.ico"))

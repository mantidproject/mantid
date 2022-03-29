# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
GUI for reduction of elastic and TOF data at the DNS instrumentat MLZ
"""
# pylint: disable=invalid-name
import sys
import os

# remove script path from sys.path otherwise Muon and Engineering modules
# are not found if called from command line
sys.path.pop(0)

from mantidqt.gui_helper import get_qapplication
from qtpy import QtGui, QtWidgets

from mantidqtinterfaces.dns.main_widget import DNSReductionGuiWidget

app, within_mantid = get_qapplication()

reducer_widget = DNSReductionGuiWidget(name='DNS-Reduction',
                                       app=app,
                                       within_mantid=within_mantid)
view = reducer_widget.view
view.setWindowTitle('DNS Reduction GUI- Powder TOF')
screenShape = QtWidgets.QDesktopWidget().screenGeometry()
view.resize(int(screenShape.width() * 0.6), int(screenShape.height() * 0.6))
appdir = os.path.dirname(__file__)
app.setWindowIcon(QtGui.QIcon(f'{appdir}/dns/dns_icon.png'))
view.show()

if not within_mantid:
    sys.exit(app.exec_())

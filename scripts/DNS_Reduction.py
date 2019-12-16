# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
GUI for reduction of elastic and TOF data at the DNS instrumentat MLZ
"""
from __future__ import (absolute_import, division, print_function, generators)
import sys
import os
from qtpy import QtWidgets, QtGui

from mantidqt.gui_helper import get_qapplication

from DNSReduction.main_view import DNSReductionGUI_view

app, within_mantid = get_qapplication()

reducer = DNSReductionGUI_view()
reducer.setWindowTitle('DNS Reduction GUI')
screenShape = QtWidgets.QDesktopWidget().screenGeometry()
reducer.resize(screenShape.width()*0.6, screenShape.height()*0.6)

appdir = os.path.dirname(__file__)
app.setWindowIcon(QtGui.QIcon('{}/DNSReduction/dns_icon.png'.format(appdir)))

reducer.show()
if not within_mantid:
    sys.exit(app.exec_())

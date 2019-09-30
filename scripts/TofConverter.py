# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from TofConverter import converterGUI
from mantidqt.gui_helper import get_qapplication

app, within_mantid = get_qapplication()

reducer = converterGUI.MainWindow()#the main ui class in this file is called MainWindow
reducer.show()
if not within_mantid:
    app.exec_()

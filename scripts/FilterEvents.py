# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QApplication)  # noqa
import sys
# set the backend for matplotlib to use
import matplotlib
if not matplotlib.get_backend().startswith('module://'):
    from qtpy import PYQT4, PYQT5
    if PYQT4:
        matplotlib.use('Qt4Agg')
        print('Qt4Agg')
    elif PYQT5:
        matplotlib.use('Qt5Agg')
        print('Qt5Agg')
    else:
        raise RuntimeError('Do not know which matplotlib backend to set')
from FilterEvents import eventFilterGUI  # noqa

def qapp():
    app = QApplication.instance()
    if app:
        return app, app.applicationName().lower().startswith('mantid')
    else:
        return QApplication(sys.argv), False


app, within_mantid = qapp()

reducer = eventFilterGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()
if not within_mantid:
    sys.exit(app.exec_())

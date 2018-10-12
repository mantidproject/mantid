# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QApplication)  # noqa
import matplotlib
import sys


def set_matplotlib_backend():
    '''MUST be called before anything tries to use matplotlib

    This will set the backend if it hasn't been already. It also returns
    the name of the backend to be the name to be used for importing the
    correct matplotlib widgets.'''
    backend = matplotlib.get_backend()
    if backend.startswith('module://'):
        if backend.endswith('qt4agg'):
            backend = 'Qt4Agg'
        elif backend.endswith('workbench') or backend.endswith('qt5agg'):
            backend = 'Qt5Agg'
    else:
        from qtpy import PYQT4, PYQT5  # noqa
        if PYQT4:
            backend = 'Qt4Agg'
        elif PYQT5:
            backend = 'Qt5Agg'
        else:
            raise RuntimeError('Do not know which matplotlib backend to set')
        matplotlib.use(backend)
    return backend


def get_qapplication():
    '''
    app, within_mantid = get_qapplication()
    reducer = eventFilterGUI.MainWindow()  # the main ui class in this file
    reducer.show()
    if not within_mantid:
        sys.exit(app.exec_())'''
    app = QApplication.instance()
    if app:
        return app, app.applicationName().lower().startswith('mantid')
    else:
        return QApplication(sys.argv), False

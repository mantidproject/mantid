# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QApplication)  # noqa
from qtpy import QtCore, QtGui
import matplotlib
import sys
import os


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
    ''' Example usage:

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


def show_interface_help(mantidplot_name, assistant_process, collection_file, qt_url, external_url):
    ''' Shows the help page for a custom interface
    @param mantidplot_name: used by showCustomInterfaceHelp
    @param assistant_process: needs to be started/closed from outside (see example below)
    @param collection_file: qth file containing the help in format used by qtassistant
    @param qt_url: location of the help in the qth file
    @param external_url: location of external page to be displayed in the default browser

    Example:

    #in the __init__ function of the GUI add:
    self.assistant_process = QtCore.QProcess(self)
    self.mantidplot_name='DGS Planner'
    self.collection_file = os.path.join(mantid._bindir, '../docs/qthelp/MantidProject.qhc')
    version = ".".join(mantid.__version__.split(".")[:2])
    self.qt_url = 'qthelp://org.sphinx.mantidproject.' + version + '/doc/interfaces/DGS Planner.html'
    self.external_url = 'http://docs.mantidproject.org/nightly/interfaces/DGS Planner.html'

    #add a help function in the GUI
    def help(self):
       show_interface_help(self.mantidplot_name,
                           self.assistant_process,
                           self.collection_file,
                           self.qt_url,
                           self.external_url)

    #make sure you close the qtassistant when the GUI is closed
    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()
    '''
    try:
        import pymantidplot
        pymantidplot.proxies.showCustomInterfaceHelp(mantidplot_name)
    except: #(ImportError, ModuleNotFoundError) raises the wrong type of error
        assistant_process.close()
        assistant_process.waitForFinished()
        helpapp = QtCore.QLibraryInfo.location(QtCore.QLibraryInfo.BinariesPath) + QtCore.QDir.separator()
        helpapp += 'assistant'
        args = ['-enableRemoteControl', '-collectionFile', collection_file, '-showUrl', qt_url]
        if os.path.isfile(helpapp) and os.path.isfile(collection_file):
            assistant_process.close()
            assistant_process.waitForFinished()
            assistant_process.start(helpapp, args)
        else:
            openUrl=QtGui.QDesktopServices.openUrl
            sysenv=QtCore.QProcessEnvironment.systemEnvironment()
            ldp=sysenv.value('LD_PRELOAD')
            if ldp:
                del os.environ['LD_PRELOAD']
            openUrl(QtCore.QUrl(external_url))
            if ldp:
                os.environ['LD_PRELOAD']=ldp

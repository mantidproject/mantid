# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QApplication
from qtpy import QtCore, QtGui
import matplotlib
import sys
import os

try:
    from mantid import __version__ as __mtd_version
    from mantid import _bindir as __mtd_bin_dir

    # convert to major.minor
    __mtd_version = ".".join(__mtd_version.split(".")[:2])
except ImportError:  # mantid not found
    __mtd_version = ""
    __mtd_bin_dir = ""


def set_matplotlib_backend():
    """MUST be called before anything tries to use matplotlib

    This will set the backend if it hasn't been already. It also returns
    the name of the backend to be the name to be used for importing the
    correct matplotlib widgets."""
    backend = matplotlib.get_backend()
    if backend.startswith("module://"):
        for suffix in ["workbench", "qt5agg", "backend_interagg"]:
            if backend.endswith(suffix):
                backend = "Qt5Agg"
                break
    else:
        from qtpy import PYQT5

        if PYQT5:
            backend = "Qt5Agg"
        else:
            raise RuntimeError("Do not know which matplotlib backend to set")
        matplotlib.use(backend)
    return backend


def get_qapplication():
    """Example usage:

    app, within_mantid = get_qapplication()
    reducer = eventFilterGUI.MainWindow()  # the main ui class in this file
    reducer.show()
    if not within_mantid:
        sys.exit(app.exec_())"""
    app = QApplication.instance()
    if app:
        return app, app.applicationName().lower().startswith("mantid")
    else:
        return QApplication(sys.argv), False


def __to_external_url(interface_name: str, section: str, external_url: str) -> QtCore.QUrl:
    if not external_url:
        template = "http://docs.mantidproject.org/nightly/interfaces/{}/{}.html"
        external_url = template.format(section, interface_name)

    return QtCore.QUrl(external_url)


def __to_qthelp_url(interface_name: str, section: str, qt_url: str) -> str:
    if qt_url:
        return qt_url
    else:
        template = "qthelp://org.sphinx.mantidproject.{}/doc/interfaces/{}/{}.html"
        return template.format(__mtd_version, section, interface_name)


def __get_collection_file(collection_file: str) -> str:
    if not collection_file:
        if not __mtd_bin_dir:
            return "HELP COLLECTION FILE NOT FOUND"
        else:
            collection_file = os.path.join(__mtd_bin_dir, "../docs/qthelp/MantidProject.qhc")

    return os.path.abspath(collection_file)


def show_interface_help(
    mantidplot_name, assistant_process, area: str = "", collection_file: str = "", qt_url: str = "", external_url: str = ""
):
    """Shows the help page for a custom interface
    @param mantidplot_name: used by showCustomInterfaceHelp
    @param assistant_process: needs to be started/closed from outside (see example below)
    @param collection_file: qth file containing the help in format used by qtassistant. The default is
    ``mantid._bindir + '../docs/qthelp/MantidProject.qhc'``
    @param qt_url: location of the help in the qth file. The default value is
    ``qthelp://org.sphinx.mantidproject.{mtdversion}/doc/interfaces/{mantidplot_name}.html``.
    @param external_url: location of external page to be displayed in the default browser. The default value is
    ``http://docs.mantidproject.org/nightly/interfaces/framework/{mantidplot_name}.html``

    Example using defaults:

    #in the __init__ function of the GUI add:
    self.assistant_process = QtCore.QProcess(self)
    self.mantidplot_name='DGS Planner'

    #add a help function in the GUI
    def help(self):
       show_interface_help(self.mantidplot_name,
                           self.assistant_process)

    #make sure you close the qtassistant when the GUI is closed
    def closeEvent(self, event):
        self.assistant_process.close()
        self.assistant_process.waitForFinished()
        event.accept()
    """
    try:
        # try using built-in help in mantid
        import mantidqt

        mantidqt.interfacemanager.InterfaceManager().showCustomInterfaceHelp(mantidplot_name, area)
    except:  # (ImportError, ModuleNotFoundError) raises the wrong type of error
        # built-in help failed, try external qtassistant then give up and launch a browser

        # cleanup previous version
        assistant_process.close()
        assistant_process.waitForFinished()

        # where to expect qtassistant
        helpapp = QtCore.QLibraryInfo.location(QtCore.QLibraryInfo.BinariesPath) + QtCore.QDir.separator()
        helpapp += "assistant"

        collection_file = __get_collection_file(collection_file)
        if os.path.isfile(helpapp) and os.path.isfile(collection_file):
            # try to find the collection file and launch qtassistant
            args = ["-enableRemoteControl", "-collectionFile", collection_file, "-showUrl", __to_qthelp_url(mantidplot_name, area, qt_url)]

            assistant_process.close()
            assistant_process.waitForFinished()
            assistant_process.start(helpapp, args)
        else:
            # give up and upen a URL in default browser
            openUrl = QtGui.QDesktopServices.openUrl
            sysenv = QtCore.QProcessEnvironment.systemEnvironment()
            ldp = sysenv.value("LD_PRELOAD")
            if ldp:
                del os.environ["LD_PRELOAD"]

            # create a url to the help in the default location
            openUrl(__to_external_url(mantidplot_name, area, external_url))

            if ldp:
                os.environ["LD_PRELOAD"] = ldp

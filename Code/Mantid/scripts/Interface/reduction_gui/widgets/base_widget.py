from PyQt4 import QtGui, uic, QtCore
import os
import types
from reduction_gui.settings.application_settings import GeneralSettings

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from mantid.api import AnalysisDataService
    import mantid.simpleapi as api
    IS_IN_MANTIDPLOT = True
except:
    pass

def process_file_parameter(f):
    """
        Decorator that allows a function parameter to either be
        a string or a function returning a string
    """
    def processed_function(self, file_name=None, **kwargs):
        if file_name is None:
            return f(self, **kwargs)
        if isinstance(file_name, types.StringType):
            return f(self, file_name, **kwargs)
        else:
            return f(self, str(file_name()), **kwargs)
    return processed_function

class BaseWidget(QtGui.QWidget):
    """
        Base widget for reduction UI
    """
    ## Widget name
    name = ""

    def __init__(self, parent=None, state=None, settings=None, data_type=None, ui_class=None, data_proxy=None):
        QtGui.QWidget.__init__(self, parent)

        self._layout = QtGui.QHBoxLayout()
        self.setLayout(self._layout)
        if ui_class is not None:
            self._content = ui_class(self)
            self._layout.addWidget(self._content)

        # Data filter for file dialog
        self._data_type="Data files (*.xml)"
        if data_type is not None:
            self._data_type = data_type

        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

        if ui_class is not None:
            self.setLayout(self._layout)
            self.initialize_content()

        self._instrument_view = None
        self._data_set_viewed = ''

        self._data_proxy = data_proxy
        self._in_mantidplot = IS_IN_MANTIDPLOT and self._data_proxy is not None

        self._is_running = True

    def is_running(self, is_running):
        """
            Change running state
        """
        self._is_running = is_running

    def initialize_content(self):
        """
            Declare the validators and event connections for the
            widgets loaded through the .ui file.
        """
        return NotImplemented

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: InstrumentDescription object
        """
        return NotImplemented

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        return NotImplemented

    def live_button_widget(self):
        """
            Returns a reference to a widget that is or contains a live data button
        """
        return None

    def live_button_toggled_actions(self,checked):
        """
            Actions to take on the widget (e.g. setting or disabling certain items) if the
            live button has been turned on or off.
            Default is to do nothing - override this method if you need something to happen.
            @param checked: True if the button has been checked, false if unchecked
        """
        pass

    def dir_browse_dialog(self):
        """
            Pop up a directory dialog box.
            @param data_type: string used to filter the files
            @param title: string to use as title
            @param multi: multiselection is enabled if True
        """
        dirname = str(QtGui.QFileDialog.getExistingDirectory(self, "Select Directory"))

        return dirname

    def data_browse_dialog(self, data_type=None, title=None, multi=False):
        """
            Pop up a file dialog box.
            @param data_type: string used to filter the files
            @param title: string to use as title
            @param multi: multiselection is enabled if True
        """
        if data_type is None:
            data_type = self._data_type
        if title is None:
            title = "Data file - Choose a data file"
        if multi:
            qflist = QtGui.QFileDialog.getOpenFileNames(self, title,
                                                              self._settings.data_path,
                                                              data_type)
            if len(qflist)>0:
                flist = []
                for i in range(len(qflist)):
                    flist.append(str(QtCore.QFileInfo(qflist[i]).filePath()))
                # Store the location of the loaded file
                self._settings.data_path = str(QtCore.QFileInfo(qflist[i]).path())
                return flist
            else:
                return None
        else:
            fname = QtCore.QFileInfo(QtGui.QFileDialog.getOpenFileName(self, title,
                                                              self._settings.data_path,
                                                              data_type)).filePath()
            if fname:
                # Store the location of the loaded file
                self._settings.data_path = str(QtCore.QFileInfo(fname).path())
            return str(fname)

    def data_save_dialog(self, data_type=None, title=None):
        """
            Pop up a save file dialog box.
            @param data_type: string used to filter the files
            @param title: string to use as title
        """
        if data_type is None:
            data_type = self._data_type
        if title is None:
            title = "Save file - Set a location and name"
        fname = QtCore.QFileInfo(QtGui.QFileDialog.getSaveFileName(self, title,
                                                                   self._settings.data_path,
                                                                   data_type)).filePath()
        return str(fname)

    @process_file_parameter
    def show_instrument(self, file_name=None, workspace=None, tab=-1, reload=False, mask=None, data_proxy=None):
        """
            Show instrument for the given data file.
            If both file_name and workspace are given, the file will be loaded in
            a workspace with the given name.

            @param file_name: Data file path
            @param workspace: Workspace to create
            @param tab: Tab to open the instrument window in
        """
        file_name = str(file_name)

        def _show_ws_instrument(ws):
            if not AnalysisDataService.doesExist(ws):
                return

            # Do nothing if the instrument view is already displayed
            #FIXME: this doesn't seem to work 100% yet
            if False and self._instrument_view is not None and \
                self._data_set_viewed == file_name \
                and self._instrument_view.isVisible():

                # If we want a reload, close the instrument window currently shown
                if reload:
                    self._instrument_view.close()
                else:
                    return True

            self._instrument_view = mantidplot.getInstrumentView(ws, tab)
            if self._instrument_view is not None:
                self._instrument_view.show()
                self._data_set_viewed = file_name
                return True

            return False

        # Sanity check
        if not IS_IN_MANTIDPLOT:
            return

        # Set up workspace name
        if workspace is None:
            workspace = '__'+os.path.basename(file_name)

        # See if the file is already loaded
        if not reload and _show_ws_instrument(workspace):
            return

        if data_proxy is None:
            data_proxy = self._data_proxy

        if data_proxy is not None:
            proxy = data_proxy(file_name, workspace)
            if proxy.data_ws is not None:
                if mask is not None:
                    api.MaskDetectors(Workspace=proxy.data_ws, DetectorList=mask)
                _show_ws_instrument(proxy.data_ws)
            else:
                if hasattr(proxy, 'errors'):
                    if type(proxy.errors)==list:
                        for e in proxy.errors:
                            print e
                    else:
                        print proxy.errors
                QtGui.QMessageBox.warning(self, "Data Error", "Mantid doesn't know how to load this file")
        else:
            QtGui.QMessageBox.warning(self, "Data Error", "Mantid doesn't know how to load this file")



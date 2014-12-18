from PyQt4 import QtGui, uic, QtCore
import os
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.ui_data_catalog

from reduction_gui.reduction.scripter import BaseScriptElement
class Catalog(BaseScriptElement):
    def __init__(self):
        pass

class SANSCatalogWidget(BaseWidget):
    """
        Widget that present a data catalog to the user
    """
    ## Widget name
    name = "Data Catalog"
    _current_run = None

    def __init__(self, parent=None, state=None, settings=None, catalog_cls=None):
        super(SANSCatalogWidget, self).__init__(parent, state, settings)

        self._catalog_cls = catalog_cls

        class DataFrame(QtGui.QFrame, ui.ui_data_catalog.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = DataFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._content)

        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

       # Connect do UI data update
        self._settings.data_updated.connect(self._data_updated)

    def initialize_content(self):
        self.copyAction = QtGui.QAction("Copy",  self)
        self.copyAction.setShortcut("Ctrl+C")
        self.addAction(self.copyAction)

        self.connect(self.copyAction, QtCore.SIGNAL("triggered()"), self.copyCells)
        self._content.data_set_table.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.connect(self._content.data_set_table, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.tableWidgetContext)
        self.connect(self._content.refresh_button, QtCore.SIGNAL("clicked()"), self._update_content)
        self.connect(self._content.browse_button, QtCore.SIGNAL("clicked()"), self._browse_directory)
        self.connect(self._content.directory_edit, QtCore.SIGNAL("returnPressed()"), self._update_content)
        self._content.directory_edit.setText(self._settings.data_path)
        self._update_content(False)

    def tableWidgetContext(self, point):
        '''Create a menu for the tableWidget and associated actions'''
        tw_menu = QtGui.QMenu("Menu", self)
        tw_menu.addAction(self.copyAction)
        tw_menu.exec_(self.mapToGlobal(point))

    def is_running(self, is_running):
        """
            Enable/disable controls depending on whether a reduction is running or not
            @param is_running: True if a reduction is running
        """
        super(SANSCatalogWidget, self).is_running(is_running)
        self._content.refresh_button.setEnabled(not is_running)
        self._content.browse_button.setEnabled(not is_running)
        self._content.directory_edit.setEnabled(not is_running)

    def _data_updated(self, key, value):
        """
            Respond to application-level key/value pair updates.
            @param key: key string
            @param value: value string
        """
        try:
            if key == "sample_run":
                self._current_run = self._catalog_cls.data_set_cls.handle(str(value))
            elif key in self._catalog_cls.data_set_cls.data_type_cls.DATA_TYPES.keys():
                run = self._catalog_cls.data_set_cls.handle(str(value))
                if self._catalog_cls is not None and run is not None:
                    #TODO: At some point we want to tie the type to a given sample run too
                    self._catalog_cls().add_type(run, key)
        except:
            print "SANSCatalogWidget: Could not access local data catalog"

    def copyCells(self):
        indices = self._content.data_set_table.selectedIndexes()
        if len(indices)==0:
            return

        col_count = self._content.data_set_table.columnCount()
        rows = []
        for r in indices:
            if r.row() not in rows:
                rows.append(r.row())

        selected_text = ""
        for row in rows:
            for i in range(col_count):
                data = self._content.data_set_table.item(row,i)
                if data is not None:
                    selected_text += str(data.text())
                if i<col_count-1:
                    selected_text += '\t'
            selected_text += '\n'

        QtGui.QApplication.clipboard().setText(selected_text)

    def _update_content(self, process_files=True):
        self._settings.data_path = str(self._content.directory_edit.text())
        self._content.data_set_table.clear()
        self._content.data_set_table.setSortingEnabled(False)
        self._content.data_set_table.setRowCount(0)
        headers = ["Run", "Title", "Start", "Time[s]", "SDD[mm]", "Comment"]
        self._content.data_set_table.setColumnCount(len(headers))
        self._content.data_set_table.setHorizontalHeaderLabels(headers)
        # Stretch the columns evenly
        h = self._content.data_set_table.horizontalHeader()
        h.setResizeMode(1)
        h.setResizeMode(1,0)

        if self._catalog_cls is not None:
            dc = self._catalog_cls()
            def _add_item(data):
                row = dc.size()
                self._content.data_set_table.insertRow(row)
                for i in range(len(data)):
                    if data[i] is not None:
                        item = QtGui.QTableWidgetItem(str(data[i]))
                        item.setFlags( QtCore.Qt.ItemIsSelectable |  QtCore.Qt.ItemIsEnabled )
                        self._content.data_set_table.setItem(row, i, item)

            dc.list_data_sets(self._settings.data_path, call_back=_add_item, process_files=process_files)

        self._content.data_set_table.setSortingEnabled(True)
        self._content.data_set_table.resizeColumnsToContents()

    def _get_catalog(self):
        rows = []
        if self._catalog_cls is not None:
            dc = self._catalog_cls()
            rows = dc.get_string_list(self._settings.data_path)

        self._content.data_set_table.clear()
        self._content.data_set_table.setSortingEnabled(False)
        self._content.data_set_table.setRowCount(len(rows))
        headers = ["Run", "Title", "Start", "Time[s]", "SDD[mm]", "Comment"]
        self._content.data_set_table.setColumnCount(len(headers))
        self._content.data_set_table.setHorizontalHeaderLabels(headers)

        for row, data in enumerate(rows):
            for i in range(len(data)):
                if data[i] is not None:
                    item = QtGui.QTableWidgetItem(str(data[i]))
                    item.setFlags( QtCore.Qt.ItemIsSelectable |  QtCore.Qt.ItemIsEnabled )
                    self._content.data_set_table.setItem(row, i, item)

        self._content.data_set_table.setSortingEnabled(True)
        self._content.data_set_table.resizeColumnsToContents()

    def _browse_directory(self):
            dir = QtGui.QFileDialog.getExistingDirectory(self, "Open Directory",
                                                    self._settings.data_path)
            if dir:
                # Store the location of the loaded file
                self._settings.data_path = str(dir)
                self._content.directory_edit.setText(dir)
                self._update_content()

    def set_state(self, state):
        """
            Update the catalog according to the new data path
        """
        if not self._settings.data_path == str(self._content.directory_edit.text())\
            and len(self._settings.data_path)>0:
            self._content.directory_edit.setText(self._settings.data_path)
            self._update_content(False)

    def get_state(self):
        return Catalog()
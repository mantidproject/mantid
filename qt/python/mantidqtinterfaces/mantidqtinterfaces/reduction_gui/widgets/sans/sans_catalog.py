# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QApplication, QFileDialog, QFrame, QMenu, QTableWidgetItem
from mantidqtinterfaces.reduction_gui.settings.application_settings import GeneralSettings
from mantidqtinterfaces.reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.scripter import BaseScriptElement

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantid.kernel import Logger

    Logger("SANSCatalogWidget").information("Using legacy ui importer")
    from mantidplot import load_ui


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

        class DataFrame(QFrame):
            def __init__(self, parent=None):
                QFrame.__init__(self, parent)
                self.ui = load_ui(__file__, "../../../ui/data_catalog.ui", baseinstance=self)

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
        self.copyAction = QAction("Copy", self)
        self.copyAction.setShortcut("Ctrl+C")
        self.addAction(self.copyAction)

        self.copyAction.triggered.connect(self.copyCells)
        self._content.data_set_table.setContextMenuPolicy(Qt.CustomContextMenu)
        self._content.data_set_table.customContextMenuRequested.connect(self.tableWidgetContext)
        self._content.refresh_button.clicked.connect(self._update_content)
        self._content.browse_button.clicked.connect(self._browse_directory)
        self._content.directory_edit.returnPressed.connect(self._update_content)
        self._content.directory_edit.setText(self._settings.catalog_data_path)
        self._content.directory_edit.setToolTip(
            "Use a path of the form: /SNS/<instrument>/IPTS-<number>/data\nE.g.: /SNS/EQSANS/IPTS-1234/data"
        )
        self._update_content(False)

    def tableWidgetContext(self, point):
        """Create a menu for the tableWidget and associated actions"""
        tw_menu = QMenu("Menu", self)
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
                    # TODO: At some point we want to tie the type to a given sample run too
                    self._catalog_cls().add_type(run, key)
        except:
            print("SANSCatalogWidget: Could not access local data catalog")

    def copyCells(self):
        indices = self._content.data_set_table.selectedIndexes()
        if len(indices) == 0:
            return

        col_count = self._content.data_set_table.columnCount()
        rows = []
        for r in indices:
            if r.row() not in rows:
                rows.append(r.row())

        selected_text = ""
        for row in rows:
            for i in range(col_count):
                data = self._content.data_set_table.item(row, i)
                if data is not None:
                    selected_text += str(data.text())
                if i < col_count - 1:
                    selected_text += "\t"
            selected_text += "\n"

        QApplication.clipboard().setText(selected_text)

    def _update_content(self, process_files=True):
        self._settings.catalog_data_path = str(self._content.directory_edit.text())
        self._content.data_set_table.clear()
        self._content.data_set_table.setSortingEnabled(False)
        self._content.data_set_table.setRowCount(0)
        headers = ["Run", "Title", "Start", "Time[s]", "SDD[mm]", "Comment"]
        self._content.data_set_table.setColumnCount(len(headers))
        self._content.data_set_table.setHorizontalHeaderLabels(headers)
        # Stretch the columns evenly
        h = self._content.data_set_table.horizontalHeader()
        h.setSectionResizeMode(1)
        h.setSectionResizeMode(1, 0)

        if self._catalog_cls is not None:
            dc = self._catalog_cls()

            def _add_item(data):
                row = dc.size()
                self._content.data_set_table.insertRow(row)
                for i in range(len(data)):
                    if data[i] is not None:
                        item = QTableWidgetItem(str(data[i]))
                        item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
                        self._content.data_set_table.setItem(row, i, item)

            dc.list_data_sets(self._settings.catalog_data_path, call_back=_add_item, process_files=process_files)

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
                    item = QTableWidgetItem(str(data[i]))
                    item.setFlags(Qt.ItemIsSelectable | Qt.ItemIsEnabled)
                    self._content.data_set_table.setItem(row, i, item)

        self._content.data_set_table.setSortingEnabled(True)
        self._content.data_set_table.resizeColumnsToContents()

    def _browse_directory(self):
        direc = QFileDialog.getExistingDirectory(self, "Open Directory", self._settings.data_path)
        if not direc:
            return
        if isinstance(direc, tuple):
            direc = direc[0]
        # Store the location of the loaded file
        self._settings.data_path = str(direc)
        self._content.directory_edit.setText(direc)
        self._update_content()

    def set_state(self, state):
        """
        Update the catalog according to the new data path
        """
        if not self._settings.catalog_data_path == str(self._content.directory_edit.text()) and len(self._settings.data_path) > 0:
            self._content.directory_edit.setText(self._settings.catalog_data_path)
            self._update_content(False)

    def get_state(self):
        return Catalog()

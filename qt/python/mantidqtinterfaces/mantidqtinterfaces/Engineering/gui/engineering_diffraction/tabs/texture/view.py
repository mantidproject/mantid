from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from mantid.api import AnalysisDataService as ADS

Ui_texture, _ = load_ui(__file__, "texture_tab.ui")


class TextureView(QtWidgets.QWidget, Ui_texture):
    sig_enable_controls = QtCore.Signal(bool)
    sig_view_requested = QtCore.Signal(str)

    def __init__(self, parent=None):
        super(TextureView, self).__init__(parent)
        self.setupUi(self)

        self.finder_texture_ws.setLabelText("Sample Run(s)")
        self.finder_texture_ws.setInstrumentOverride("ENGINX")
        self.finder_texture_ws.allowMultipleFiles(True)

        self.finder_texture_tables.setLabelText("Fit Parameters")
        self.finder_texture_tables.setInstrumentOverride("ENGINX")
        self.finder_texture_tables.allowMultipleFiles(True)

        self.populate_workspace_list()

    # ========== Signal Connectors ==========
    def set_on_load_ws_clicked(self, slot):
        self.btn_loadWSFiles.clicked.connect(slot)

    def set_on_load_param_clicked(self, slot):
        self.btn_loadParamFiles.clicked.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    def set_on_delete_clicked(self, slot):
        self.btn_deleteSelected.clicked.connect(slot)

    def set_on_select_all_clicked(self, slot):
        self.btn_selectAll.clicked.connect(slot)

    # ========== Table Handling ==========
    def populate_workspace_table(self, workspace_info_list):
        self.table_loaded_data.setColumnCount(4)
        self.table_loaded_data.setHorizontalHeaderLabels(["Run", "Fit Parameters", "Crystal Structure", "Select"])
        self.table_loaded_data.setRowCount(len(workspace_info_list))

        for row, (ws, metadata) in enumerate(workspace_info_list.items()):
            self.table_loaded_data.setItem(row, 0, QtWidgets.QTableWidgetItem(ws))
            self.table_loaded_data.setItem(row, 1, QtWidgets.QTableWidgetItem(metadata.get("fit_parameters", "Not set")))
            self.table_loaded_data.setItem(row, 2, QtWidgets.QTableWidgetItem(metadata.get("crystal", "Not set")))

            checkbox = QtWidgets.QCheckBox()
            checkbox.setChecked(metadata["select"])
            cell_widget = QtWidgets.QWidget()
            layout = QtWidgets.QHBoxLayout(cell_widget)
            layout.addWidget(checkbox)
            layout.setAlignment(QtCore.Qt.AlignCenter)
            layout.setContentsMargins(0, 0, 0, 0)
            self.table_loaded_data.setCellWidget(row, 3, cell_widget)

    def populate_workspace_list(self):
        workspace_names = list(ADS.getObjectNames())
        self.combo_workspaceList.clear()
        self.combo_workspaceList.addItems(sorted(workspace_names))

    def get_selected_workspaces(self):
        selected_wss = []
        selected_params = []
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 3)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox and checkbox.isChecked():
                    selected_wss.append(self.table_loaded_data.item(row, 0).text())
                    selected_params.append(self.table_loaded_data.item(row, 1).text())
        return selected_wss, selected_params

    def select_all_workspaces(self):
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 3)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox:
                    checkbox.setChecked(True)

    def get_file_paths(self):
        return self.finder_corr.getFilenames()

    def is_searching(self):
        return self.finder_corr.isSearching()

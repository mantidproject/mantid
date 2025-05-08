from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from mantid.api import AnalysisDataService as ADS

Ui_texture, _ = load_ui(__file__, "correction_tab.ui")


class TextureCorrectionView(QtWidgets.QWidget, Ui_texture):
    sig_enable_controls = QtCore.Signal(bool)
    sig_view_requested = QtCore.Signal(str)

    def __init__(self, parent=None):
        super(TextureCorrectionView, self).__init__(parent)
        self.setupUi(self)

        self.finder_corr.setLabelText("Sample Run(s)")
        self.finder_corr.setInstrumentOverride("ENGINX")
        self.finder_corr.allowMultipleFiles(True)

        self.finder_orientation_file.setLabelText("Orientation File")
        self.finder_orientation_file.setInstrumentOverride("ENGINX")
        self.finder_orientation_file.allowMultipleFiles(False)

        self.finder_gauge_vol.setLabelText("Custom Gauge Volume Shape")
        self.finder_gauge_vol.setInstrumentOverride("ENGINX")
        self.finder_gauge_vol.allowMultipleFiles(False)
        self.finder_gauge_vol.setFileExtensions([".xml"])

        self.set_include_absorption(True)
        self.set_include_divergence(True)

        self.populate_workspace_list()

    # ========== Signal Connectors ==========
    def set_on_apply_clicked(self, slot):
        self.btn_applyCorrections.clicked.connect(slot)

    def set_on_load_orientation_clicked(self, slot):
        self.btn_loadOrientation.clicked.connect(slot)

    def set_enable_controls_connection(self, slot):
        self.sig_enable_controls.connect(slot)

    def set_on_load_clicked(self, slot):
        self.btn_loadFiles.clicked.connect(slot)

    def set_on_delete_clicked(self, slot):
        self.btn_deleteSelected.clicked.connect(slot)

    def set_on_select_all_clicked(self, slot):
        self.btn_selectAll.clicked.connect(slot)

    def set_on_set_orientation_clicked(self, slot):
        self.btn_setOrientation.clicked.connect(slot)

    def set_on_load_sample_shape_clicked(self, slot):
        self.btn_loadSampleShape.clicked.connect(slot)

    def set_on_set_sample_shape_clicked(self, slot):
        self.btn_setSampleShape.clicked.connect(slot)

    def set_on_set_material_clicked(self, slot):
        self.btn_setSampleMaterial.clicked.connect(slot)

    def set_on_copy_sample_clicked(self, slot):
        self.btn_copySampleToAll.clicked.connect(slot)

    # ========== Table Handling ==========
    def populate_workspace_table(self, workspace_info_list):
        self.table_loaded_data.setColumnCount(4)
        self.table_loaded_data.setHorizontalHeaderLabels(["Run", "Env", "Orientation", "Select"])
        self.table_loaded_data.setRowCount(len(workspace_info_list))

        for row, (ws, metadata) in enumerate(workspace_info_list.items()):
            self.table_loaded_data.setItem(row, 0, QtWidgets.QTableWidgetItem(ws))
            self.table_loaded_data.setItem(row, 1, QtWidgets.QTableWidgetItem(metadata.get("env", "Not set")))
            self.table_loaded_data.setItem(row, 2, QtWidgets.QTableWidgetItem(metadata.get("orient", "default")))

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
        selected = []
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 3)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox and checkbox.isChecked():
                    selected.append(self.table_loaded_data.item(row, 0).text())
        return selected

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

    # ========== Component Getters ==========
    def get_orientation_file(self):
        fnames = self.finder_orientation_file.getFilenames()
        return fnames[0] if len(fnames) > 0 else None

    def get_custom_shape(self):
        fnames = self.finder_gauge_vol.getFilenames()
        return fnames[0] if len(fnames) > 0 else None

    def get_sample_reference_ws(self):
        return self.combo_workspaceList.currentText()

    def get_div_horz(self):
        return float(self.line_divHorz.text())

    def get_div_vert(self):
        return float(self.line_divVert.text())

    def get_div_det_horz(self):
        return float(self.line_detHorz.text())

    def get_shape_method(self):
        return self.combo_shapeMethod.currentText()

    def set_include_absorption(self, val):
        return self.check_absorption.setChecked(val)

    def set_include_divergence(self, val):
        return self.check_divergence.setChecked(val)

    def include_absorption(self):
        return self.check_absorption.isChecked()

    def include_divergence(self):
        return self.check_divergence.isChecked()

    def setup_tabbing_order(self):
        self.finder_corr.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setTabOrder(self.finder_corr, self.line_orientationFile)

    # ============ Visibility =============

    def toggle_absorption_section_enabled(self):
        self.combo_shapeMethod.setVisible(not self.combo_shapeMethod.isVisible())
        self.finder_gauge_vol.setVisible(not self.finder_gauge_vol.isVisible())

    def toggle_divergence_section_enabled(self):
        self.label_divHorz.setVisible(not self.label_divHorz.isVisible())
        self.line_divHorz.setVisible(not self.line_divHorz.isVisible())
        self.label_divVert.setVisible(not self.label_divVert.isVisible())
        self.line_divVert.setVisible(not self.line_divVert.isVisible())
        self.label_detHorz.setVisible(not self.label_detHorz.isVisible())
        self.line_detHorz.setVisible(not self.line_detHorz.isVisible())

    def set_divergence_section_off(self):
        self.label_divHorz.setVisible(False)
        self.line_divHorz.setVisible(False)
        self.label_divVert.setVisible(False)
        self.line_divVert.setVisible(False)
        self.label_detHorz.setVisible(False)
        self.line_detHorz.setVisible(False)

    def set_on_check_inc_abs_corr_state_changed(self, slot):
        self.check_absorption.stateChanged.connect(slot)

    def set_on_check_inc_div_corr_state_changed(self, slot):
        self.check_divergence.stateChanged.connect(slot)

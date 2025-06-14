from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import UnitFactory


Ui_texture, _ = load_ui(__file__, "correction_tab.ui")


class TextureCorrectionView(QtWidgets.QWidget, Ui_texture):
    sig_enable_controls = QtCore.Signal(bool)
    sig_view_requested = QtCore.Signal(str)
    sig_view_shape_requested = QtCore.Signal(str)

    def __init__(self, parent=None):
        super(TextureCorrectionView, self).__init__(parent)
        self.setupUi(self)

        self.finder_corr.setLabelText("Sample Run(s)")
        self.finder_corr.allowMultipleFiles(True)

        self.finder_reference.setLabelText("Reference Workspace")
        self.finder_reference.allowMultipleFiles(False)
        self.finder_reference.setFileExtensions([".nxs"])

        self.finder_orientation_file.setLabelText("Orientation File")
        self.finder_orientation_file.allowMultipleFiles(False)
        self.finder_orientation_file.setFileExtensions([".txt"])

        self.finder_gauge_vol.setLabelText("Custom Gauge Volume Shape")
        self.finder_gauge_vol.allowMultipleFiles(False)
        self.finder_gauge_vol.setFileExtensions([".xml"])

        self.set_include_absorption(True)
        self.set_include_divergence(True)

        self.populate_workspace_list()
        self.populate_unit_list()
        self.set_default_unit("dSpacing")

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

    def set_on_deselect_all_clicked(self, slot):
        self.btn_deselectAll.clicked.connect(slot)

    def set_on_view_reference_shape_clicked(self, slot):
        self.btn_viewRefShape.clicked.connect(slot)

    def set_on_create_ref_ws_clicked(self, slot):
        self.btn_createRefWS.clicked.connect(slot)

    def set_on_set_ref_ws_orientation_clicked(self, slot):
        self.btn_setRefOrientation.clicked.connect(slot)

    def set_on_save_ref_clicked(self, slot):
        self.btn_saveRefWS.clicked.connect(slot)

    def set_on_load_ref_clicked(self, slot):
        self.btn_loadRef.clicked.connect(slot)

    def set_on_set_orientation_clicked(self, slot):
        self.btn_setOrientation.clicked.connect(slot)

    def set_on_load_sample_shape_clicked(self, slot):
        self.btn_loadSampleShape.clicked.connect(slot)

    def set_on_set_sample_shape_clicked(self, slot):
        self.btn_setSampleShape.clicked.connect(slot)

    def set_on_set_material_clicked(self, slot):
        self.btn_setSampleMaterial.clicked.connect(slot)

    def set_on_copy_ref_sample_clicked(self, slot):
        self.btn_copyRefSample.clicked.connect(slot)

    def set_on_copy_sample_clicked(self, slot):
        self.btn_copySampleToAll.clicked.connect(slot)

    # ========== Table Handling ==========
    def populate_workspace_table(self, workspace_info_list):
        self.table_loaded_data.setColumnCount(5)
        self.table_loaded_data.setHorizontalHeaderLabels(["Run", "Shape", "Material", "Orientation", "Select"])
        self.table_loaded_data.setRowCount(len(workspace_info_list))

        header = self.table_loaded_data.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(4, QtWidgets.QHeaderView.ResizeToContents)

        for row, (ws, metadata) in enumerate(workspace_info_list.items()):
            self.table_loaded_data.setItem(row, 0, QtWidgets.QTableWidgetItem(ws))
            self.table_loaded_data.setItem(row, 2, QtWidgets.QTableWidgetItem(metadata.get("material", "Not set")))
            self.table_loaded_data.setItem(row, 3, QtWidgets.QTableWidgetItem(metadata.get("orient", "default")))

            if metadata.get("shape", "Not set") == "Not set":
                self.table_loaded_data.setItem(row, 1, QtWidgets.QTableWidgetItem("Not set"))
            else:
                self.table_loaded_data.setItem(row, 1, QtWidgets.QTableWidgetItem(""))
                view_btn = QtWidgets.QPushButton()
                view_btn.setProperty("text", "View Shape")
                view_btn.clicked.connect(lambda _, ws_name=ws: self.sig_view_shape_requested.emit(ws_name))
                cell_widget = QtWidgets.QWidget()
                layout = QtWidgets.QHBoxLayout(cell_widget)
                layout.addWidget(view_btn)
                layout.setAlignment(QtCore.Qt.AlignCenter)
                layout.setContentsMargins(0, 0, 0, 0)
                self.table_loaded_data.setCellWidget(row, 1, cell_widget)

            checkbox = QtWidgets.QCheckBox()
            checkbox.setChecked(metadata["select"])
            cell_widget = QtWidgets.QWidget()
            layout = QtWidgets.QHBoxLayout(cell_widget)
            layout.addWidget(checkbox)
            layout.setAlignment(QtCore.Qt.AlignCenter)
            layout.setContentsMargins(0, 0, 0, 0)
            self.table_loaded_data.setCellWidget(row, 4, cell_widget)

    def populate_workspace_list(self):
        workspace_names = list(ADS.getObjectNames())
        self.combo_workspaceList.clear()
        self.combo_workspaceList.addItems(sorted(workspace_names))

    def populate_unit_list(self):
        units = UnitFactory.getKeys()
        self.combo_Units.clear()
        self.combo_Units.addItems(units)

    def set_default_unit(self, default_val):
        units = [self.combo_Units.itemText(i) for i in range(self.combo_Units.count())]
        default_ind = units.index(default_val)
        self.combo_Units.setCurrentIndex(default_ind)

    def get_selected_workspaces(self):
        selected = []
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 4)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox and checkbox.isChecked():
                    selected.append(self.table_loaded_data.item(row, 0).text())
        return selected

    def set_all_workspaces_selected(self, selected):
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 4)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox:
                    checkbox.setChecked(selected)

    def update_reference_info_section(self, ws_name, shape_enabled, material):
        self.ref_frame_status.setText(ws_name)
        self.btn_viewRefShape.setEnabled(shape_enabled)
        self.ref_material_status.setText(material)

    def get_file_paths(self):
        return self.finder_corr.getFilenames()

    def is_searching(self):
        return self.finder_corr.isSearching()

    # ========== Component Getters ==========
    def get_reference_file(self):
        fnames = self.finder_reference.getFilenames()
        return fnames[0] if len(fnames) > 0 else None

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

    def get_evaluation_value(self):
        return float(self.line_evalVal.text())

    def get_evaluation_units(self):
        return self.combo_Units.currentText()

    # ========== Component Setters ==========

    def set_include_absorption(self, val):
        return self.check_absorption.setChecked(val)

    def set_include_divergence(self, val):
        return self.check_divergence.setChecked(val)

    def set_include_atten_tab(self, val):
        return self.check_attenTab.setChecked(val)

    def include_absorption(self):
        return self.check_absorption.isChecked()

    def include_divergence(self):
        return self.check_divergence.isChecked()

    def include_atten_tab(self):
        return self.check_attenTab.isChecked()

    def setup_tabbing_order(self):
        self.finder_corr.focusProxy().setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setTabOrder(self.finder_corr, self.line_orientationFile)

    # ============ Visibility =============

    def update_absorption_section_visibility(self):
        self.set_absorption_section_visibility(self.include_absorption())

    def update_divergence_section_visibility(self):
        self.set_divergence_section_visibility(self.include_divergence())

    def update_atten_tab_section_visibility(self):
        self.set_atten_tab_visibility(self.include_atten_tab())

    def set_absorption_section_visibility(self, vis):
        self.combo_shapeMethod.setVisible(vis)
        self.finder_gauge_vol.setVisible(vis)

    def set_divergence_section_visibility(self, vis):
        self.label_divHorz.setVisible(vis)
        self.line_divHorz.setVisible(vis)
        self.label_divVert.setVisible(vis)
        self.line_divVert.setVisible(vis)
        self.label_detHorz.setVisible(vis)
        self.line_detHorz.setVisible(vis)

    def set_atten_tab_visibility(self, vis):
        self.widget_attenuationTableContainer.setVisible(vis)

    def set_on_check_inc_abs_corr_state_changed(self, slot):
        self.check_absorption.stateChanged.connect(slot)

    def set_on_check_inc_div_corr_state_changed(self, slot):
        self.check_divergence.stateChanged.connect(slot)

    def set_on_check_att_tab_state_changed(self, slot):
        self.check_attenTab.stateChanged.connect(slot)

    def set_on_gauge_vol_state_changed(self, slot):
        self.combo_shapeMethod.currentIndexChanged.connect(slot)

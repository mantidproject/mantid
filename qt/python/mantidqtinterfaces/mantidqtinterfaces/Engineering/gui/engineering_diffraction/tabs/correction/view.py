# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import UnitFactory
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_view import ShowSampleView
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import create_workspace_table

Ui_texture, _ = load_ui(__file__, "correction_tab.ui")


class TextureCorrectionView(QtWidgets.QWidget, Ui_texture):
    sig_enable_controls = QtCore.Signal(bool)
    sig_view_requested = QtCore.Signal(str)
    sig_view_shape_requested = QtCore.Signal(str)
    alg_ui_finished = QtCore.Signal()

    def __init__(self, parent=None, instrument="ENGINX"):
        super(TextureCorrectionView, self).__init__(parent)
        self.setupUi(self)
        self.init_tool_tips()

        self.show_sample_view = ShowSampleView()

        self.finder_corr.setLabelText("Sample Run(s)")
        self.finder_corr.allowMultipleFiles(True)
        self.finder_corr.findFiles(True)
        self.finder_corr.setInstrumentOverride(instrument)

        self.finder_reference.setLabelText("Reference Workspace")
        self.finder_reference.allowMultipleFiles(False)
        self.finder_reference.setFileExtensions([".nxs"])

        self.finder_orientation_file.setLabelText("Orientation File")
        self.finder_orientation_file.allowMultipleFiles(False)
        self.finder_orientation_file.setFileExtensions([".txt"])

        self.finder_gauge_vol.setLabelText("Custom Gauge Volume File")
        self.finder_gauge_vol.allowMultipleFiles(False)
        self.finder_gauge_vol.setFileExtensions([".xml"])

        self.set_include_absorption(True)
        self.set_include_divergence(True)

        self.populate_workspace_list()
        self.populate_unit_list()
        self.set_default_unit("dSpacing")

        self.table_column_headers = ["Run", "Shape", "Material", "Orientation", "Select"]

        self.line_divHorz.setText("0.02")
        self.line_divVert.setText("0.02")
        self.line_detHorz.setText("0.012")

    # ========== Setup Tool Tips ==========

    def init_tool_tips(self):
        self.btn_loadFiles.setToolTip("Loads the selected sample runs into the table")
        self.btn_selectAll.setToolTip("Sets all of the loaded workspaces in the table to selected")
        self.btn_deselectAll.setToolTip("Sets all of the loaded workspaces in the table to unselected")
        self.btn_deleteSelected.setToolTip("Deletes all of the loaded files, which are set as selected, from the table")
        self.btn_viewRefShape.setToolTip(
            "Pops up a plot of the sample shape on a given workspace, "
            "along with the relative orientation of sample axes (defined in the settings)"
        )
        self.btn_createRefWS.setToolTip(
            "Creates an empty workspace that can hold the reference sample information "
            "that can then be copied onto each experimental workspace"
        )
        self.btn_setRefOrientation.setToolTip(
            "Allows an initial orientation to be applied to the reference shape to "
            "align it correctly with the experimental sample upon a "
            "positioner with neutral/default/homed motor values "
        )
        self.btn_saveRefWS.setToolTip("Saves the reference workspace into the experimental data folder")
        self.btn_loadRef.setToolTip("Loads a reference workspace from provided file path")
        self.btn_loadSampleShape.setToolTip(
            "Opens dialog for LoadSampleShape, allows loading of STL sample shape onto the reference/experimental workspaces"
        )
        self.btn_setSampleShape.setToolTip(
            "Opens dialog for CreateSampleShape, "
            "allows defining sample shape onto the reference/experimental workspaces using "
            "constructive solid geometry XML string"
        )
        self.btn_setSampleMaterial.setToolTip(
            "Opens dialog for SetSampleMaterial, allows definition of sample material properties onto the reference/experimental workspaces"
        )
        self.btn_setOrientation.setToolTip(
            "Opens dialog for SetGoniometer, allows definition of sample orientation onto individual experimental workspaces"
        )
        self.btn_loadOrientation.setToolTip(
            "Reads the provided orientation file and sets the orientation provided in each line onto the selected workspaces in turn"
        )
        self.btn_copyRefSample.setToolTip("Copies the sample information from the reference workspace onto all of the selected workspaces")
        self.combo_workspaceList.setToolTip("Workspace who's sample should be copied to all selected workspaces")
        self.btn_copySampleToAll.setToolTip(
            "Copies the sample information from the workspace in the dropdown menu onto all of the selected workspaces"
        )
        self.combo_shapeMethod.setToolTip(
            "Gauge Volume to use for correction, select either: the preset 4mm x 4mm x 4mm cube; "
            "No Gauge Volume; or Custom Shape (to be provided as an XML file)"
        )
        self.line_evalVal.setToolTip(
            "Position along the spectra x-axis where the attenuation value saved into the table should be evaluated"
        )
        self.combo_Units.setToolTip("Unit the x-axis should be in when getting the attenuation at the evaluation point")
        self.line_divHorz.setToolTip("The horizontal component of the divergence of the incident beam (in radians)")
        self.line_divVert.setToolTip("The vertical component of the divergence of the incident beam (in radians)")
        self.line_detHorz.setToolTip(
            "The horizontal component of the divergence of the scattered beam (in radians) - "
            "currently this correction assumes this is the same for all detector groups"
        )
        self.btn_applyCorrections.setToolTip(
            "Applies all the flagged corrections according to the provided parameters "
            "onto each selected workspace in turn, saving the resulting workspaces "
            "in the indicated save directory"
        )

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

    def set_on_view_shape_requested(self, slot):
        self.sig_view_shape_requested.connect(slot)

    # ========== Table Handling ==========
    def populate_workspace_table(self, workspace_info_list):
        create_workspace_table(self.table_column_headers, self.table_loaded_data, len(workspace_info_list))
        for row, (ws, metadata) in enumerate(workspace_info_list.items()):
            self._add_table_row(row, ws, metadata)

    def _add_table_row(self, row, ws, metadata):
        # run names:
        self.table_loaded_data.setItem(row, 0, QtWidgets.QTableWidgetItem(ws))

        # shapes:
        self.show_sample_view.add_show_button_to_table_if_shape(
            self.sig_view_shape_requested, self.table_loaded_data, ws, row, 1, metadata.get("shape", "Not set") != "Not set"
        )

        # materials:
        self.table_loaded_data.setItem(row, 2, QtWidgets.QTableWidgetItem(metadata.get("material", "Not set")))

        # orientations:
        self.table_loaded_data.setItem(row, 3, QtWidgets.QTableWidgetItem(metadata.get("orient", "default")))

        # selected:
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

    def signal_alg_finished(self):
        self.alg_ui_finished.emit()

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
        return self.line_divHorz.text()

    def get_div_vert(self):
        return self.line_divVert.text()

    def get_div_det_horz(self):
        return self.line_detHorz.text()

    def get_shape_method(self):
        return self.combo_shapeMethod.currentText()

    def get_evaluation_value(self):
        return self.line_evalVal.text()

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

    def set_instrument_override(self, instrument):
        self.finder_corr.setInstrumentOverride(instrument)

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

    def set_finder_gauge_vol_visible(self, vis):
        self.finder_gauge_vol.setVisible(vis)

    def set_on_check_inc_abs_corr_state_changed(self, slot):
        self.check_absorption.stateChanged.connect(slot)

    def set_on_check_inc_div_corr_state_changed(self, slot):
        self.check_divergence.stateChanged.connect(slot)

    def set_on_check_att_tab_state_changed(self, slot):
        self.check_attenTab.stateChanged.connect(slot)

    def set_on_gauge_vol_state_changed(self, slot):
        self.combo_shapeMethod.currentIndexChanged.connect(slot)

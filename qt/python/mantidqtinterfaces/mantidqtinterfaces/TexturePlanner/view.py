# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QMainWindow, QHeaderView, QTableWidgetItem, QCheckBox, QWidget, QHBoxLayout, QPushButton, QToolBar
from qtpy import QtCore
from mantidqt.utils.qt import load_ui
from mantidqt.icons import get_icon
from mantid.kernel import FeatureType
from mantid import UsageService
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from qtpy.QtWidgets import QVBoxLayout
from functools import partial


Ui_texplan, _ = load_ui(__file__, "texture_planner.ui")


class TexturePlannerView(QMainWindow, Ui_texplan):
    sig_select_state_changed = QtCore.Signal()
    sig_include_state_changed = QtCore.Signal()

    def __init__(self, parent=None, presenter=None):
        super().__init__(parent)
        self.setupUi(self)
        self.presenter = presenter

        self.finder_stl.setLabelText("STL File")
        self.finder_stl.allowMultipleFiles(False)
        self.finder_stl.setFileExtensions([".stl"])
        self.finder_stl.isOptional(True)

        self.finder_xml.setLabelText("CSG File")
        self.finder_xml.allowMultipleFiles(False)
        self.finder_xml.setFileExtensions([".xml"])
        self.finder_xml.isOptional(True)

        self.finder_orient.setLabelText("Orientation File")
        self.finder_orient.allowMultipleFiles(False)
        self.finder_orient.setFileExtensions([".txt"])
        self.finder_orient.isOptional(True)

        self.finder_save_dir.setLabelText("Save Directory")
        self.finder_save_dir.isForDirectory(True)
        self.finder_save_dir.setFileExtensions([".txt"])

        self.finder_gauge_vol.setLabelText("Custom Gauge Volume File")
        self.finder_gauge_vol.allowMultipleFiles(False)
        self.finder_gauge_vol.setFileExtensions([".xml"])

        self.gonio_axes = (self.axis0, self.axis1, self.axis2, self.axis3, self.axis4, self.axis5)
        self.gonio_angles = (self.spnAngle0, self.spnAngle1, self.spnAngle2, self.spnAngle3, self.spnAngle4, self.spnAngle5)
        self.gonio_senses = (self.cmbSense0, self.cmbSense1, self.cmbSense2, self.cmbSense3, self.cmbSense4, self.cmbSense5)
        self.gonio_vecs = (self.edtVec0, self.edtVec1, self.edtVec2, self.edtVec3, self.edtVec4, self.edtVec5)
        self.init_angles = (self.spnInitX, self.spnInitY, self.spnInitZ)

        self._setup_pf_plot()
        self._setup_lab_plot()
        self.set_load_stl_enabled(False)
        self.set_load_xml_enabled(False)
        self.set_load_orientation_enabled(False)
        self.set_outputs_enabled(False)
        self.set_show_mu(False)
        self.set_material_visible(False)

        self.set_angle_limits()
        self.set_translation_step(0.001)  # setup 1mm translational steps

        self.create_workspace_table()
        self.hide_axis_columns()

        self.make_box_toggleable(self.initOrientation, self.set_init_rotations_visible)
        self.make_box_toggleable(self.initPosition, self.set_init_position_visible)
        self.make_box_toggleable(self.grpDirectionWidgets, self.set_sample_directions_visible)
        self.make_box_toggleable(self.grpOrientationFile)  # finder widget has some hidden features that toggling messes with

        self._setup_settings_toolbar()

        # register startup
        UsageService.registerFeatureUsage(FeatureType.Interface, "TexturePlanner", False)

    def _setup_settings_toolbar(self):
        toolbar = QToolBar("Main Toolbar", self)
        self.btn_settings = QPushButton()
        self.btn_settings.setIcon(get_icon("mdi.settings", "black", 1.2))
        self.btn_settings.setToolTip("Settings")
        toolbar.addWidget(self.btn_settings)
        self.addToolBar(QtCore.Qt.BottomToolBarArea, toolbar)

    def set_on_settings_clicked(self, slot):
        self.btn_settings.clicked.connect(slot)

    def init_tool_tips(self):
        self.lineedit_RD.setToolTip("Label for the first (PF: in-plane) intrinsic sample direction")
        self.lineedit_RD0.setToolTip("X component of the first intrinsic sample direction")
        self.lineedit_RD1.setToolTip("Y component of the first intrinsic sample direction")
        self.lineedit_RD2.setToolTip("Z component of the first intrinsic sample direction")
        self.lineedit_ND.setToolTip("Label for the second (PF: normal) intrinsic sample direction")
        self.lineedit_ND0.setToolTip("X component of the second intrinsic sample direction")
        self.lineedit_ND1.setToolTip("Y component of the second intrinsic sample direction")
        self.lineedit_ND2.setToolTip("Z component of the second intrinsic sample direction")
        self.lineedit_TD.setToolTip("Label for the third (PF: in-plane) intrinsic sample direction")
        self.lineedit_TD0.setToolTip("X component of the third intrinsic sample direction")
        self.lineedit_TD1.setToolTip("Y component of the third intrinsic sample direction")
        self.lineedit_TD2.setToolTip("Z component of the third intrinsic sample direction")

    def set_on_num_gonio_updated(self, slot):
        self.spnNumAxes.valueChanged.connect(slot)

    def set_on_step_updated(self, slot):
        self.spnStepSize.valueChanged.connect(slot)

    def set_on_update_dirs(self, slot):
        self.updateDirs.clicked.connect(slot)

    def set_on_gonio_vec_updated(self, slot):
        for i, gonio_vec in enumerate(self.gonio_vecs):
            gonio_vec.editingFinished.connect(partial(slot, i))

    def set_on_gonio_sense_updated(self, slot):
        for i, gonio_sense in enumerate(self.gonio_senses):
            gonio_sense.currentTextChanged.connect(partial(slot, i))

    def set_on_gonio_angle_updated(self, slot):
        for i, gonio_angle in enumerate(self.gonio_angles):
            gonio_angle.valueChanged.connect(partial(slot, i))

    def set_on_group_changed(self, slot):
        self.cmbGroup.currentTextChanged.connect(slot)

    def set_on_add_orientation_clicked(self, slot):
        self.addOrientation.clicked.connect(slot)

    def set_on_current_index_changed(self, slot):
        self.spnIndex.valueChanged.connect(slot)

    def set_on_stl_file_changed(self, slot):
        self.finder_stl.fileFindingFinished.connect(slot)

    def set_on_xml_file_changed(self, slot):
        self.finder_xml.fileFindingFinished.connect(slot)

    def set_on_orient_file_changed(self, slot):
        self.finder_orient.fileFindingFinished.connect(slot)

    def set_on_save_dir_changed(self, slot):
        self.finder_save_dir.fileFindingFinished.connect(slot)

    def set_on_save_file_changed(self, slot):
        self.saveFileLine.textEdited.connect(slot)

    def set_on_load_stl_clicked(self, slot):
        self.btnSTL.clicked.connect(slot)

    def set_on_load_xml_clicked(self, slot):
        self.btnXML.clicked.connect(slot)

    def set_on_load_orient_clicked(self, slot):
        self.btnOrient.clicked.connect(slot)

    def set_on_set_gauge_volume_clicked(self, slot):
        self.setGV.clicked.connect(slot)

    def set_on_select_all_clicked(self, slot):
        self.selectAll.clicked.connect(slot)

    def set_on_deselect_all_clicked(self, slot):
        self.deselectAll.clicked.connect(slot)

    def set_on_delete_selected_clicked(self, slot):
        self.deleteSelected.clicked.connect(slot)

    def set_on_output_sscanss_clicked(self, slot):
        self.toSscanss.clicked.connect(slot)

    def set_on_output_euler_clicked(self, slot):
        self.toEuler.clicked.connect(slot)

    def set_on_output_matrix_clicked(self, slot):
        self.toMatrix.clicked.connect(slot)

    def set_on_show_mu_toggled(self, slot):
        self.chkMu.toggled.connect(slot)

    def set_on_material_changed(self, slot):
        self.edtMaterial.editingFinished.connect(slot)

    def set_on_init_x_changed(self, slot):
        self.spnInitX.valueChanged.connect(slot)

    def set_on_init_y_changed(self, slot):
        self.spnInitY.valueChanged.connect(slot)

    def set_on_init_z_changed(self, slot):
        self.spnInitZ.valueChanged.connect(slot)

    def set_on_init_px_changed(self, slot):
        self.spnInitPX.valueChanged.connect(slot)

    def set_on_init_py_changed(self, slot):
        self.spnInitPY.valueChanged.connect(slot)

    def set_on_init_pz_changed(self, slot):
        self.spnInitPZ.valueChanged.connect(slot)

    def set_on_instrument_changed(self, slot):
        self.cmbInstr.currentTextChanged.connect(slot)

    @QtCore.Slot(bool)
    def _on_any_include_toggled(self):
        self.sig_include_state_changed.emit()

    @QtCore.Slot(bool)
    def _on_any_select_toggled(self):
        self.sig_select_state_changed.emit()

    # getters
    def get_stl_string(self):
        fnames = self.finder_stl.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_xml_string(self):
        fnames = self.finder_xml.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_orientation_file(self):
        fnames = self.finder_orient.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_save_dir(self):
        fnames = self.finder_save_dir.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_save_filename(self):
        return self.saveFileLine.text()

    def get_rd_name(self):
        return self.lineedit_RD.text()

    def get_nd_name(self):
        return self.lineedit_ND.text()

    def get_td_name(self):
        return self.lineedit_TD.text()

    def get_rd_dir(self):
        return ",".join([self.lineedit_RD0.text(), self.lineedit_RD1.text(), self.lineedit_RD2.text()])

    def get_td_dir(self):
        return ",".join([self.lineedit_TD0.text(), self.lineedit_TD1.text(), self.lineedit_TD2.text()])

    def get_nd_dir(self):
        return ",".join([self.lineedit_ND0.text(), self.lineedit_ND1.text(), self.lineedit_ND2.text()])

    def get_num_gonios(self):
        return int(self.spnNumAxes.value())

    def get_step_size(self):
        return self.spnStepSize.value()

    def get_lab_figure(self):
        return self.lab_figure

    def get_lab_ax(self):
        return self.lab_ax

    def get_pf_ax(self):
        return self.pf_ax

    def get_vecs(self):
        return [x.text() for x in self.gonio_vecs]

    def get_senses(self):
        return [x.currentText() for x in self.gonio_senses]

    def get_angles(self):
        return [x.text() for x in self.gonio_angles]

    def get_group(self):
        return self.cmbGroup.currentText()

    def get_instrument(self):
        return self.cmbInstr.currentText()

    def get_current_index(self):
        return int(self.spnIndex.value()) - 1

    def get_include_indices(self):
        return self._read_checkbox_column_states(6)

    def get_select_indices(self):
        return self._read_checkbox_column_states(7)

    def get_show_mu(self):
        return self.chkMu.isChecked()

    def get_material(self):
        return self.edtMaterial.text()

    def get_init_x(self):
        return self.spnInitX.value()

    def get_init_y(self):
        return self.spnInitY.value()

    def get_init_z(self):
        return self.spnInitZ.value()

    def get_init_px(self):
        return self.spnInitPX.value()

    def get_init_py(self):
        return self.spnInitPY.value()

    def get_init_pz(self):
        return self.spnInitPZ.value()

    def get_shape_method(self):
        return self.combo_shapeMethod.currentText()

    def get_custom_shape(self):
        fnames = self.finder_gauge_vol.getFilenames()
        return fnames[0] if len(fnames) > 0 else None

    # setters

    def set_rd_name(self, text):
        self.lineedit_RD.setText(text)

    def set_nd_name(self, text):
        self.lineedit_ND.setText(text)

    def set_td_name(self, text):
        self.lineedit_TD.setText(text)

    def set_rd_dir(self, vec):
        self.lineedit_RD0.setText(str(vec[0]))
        self.lineedit_RD1.setText(str(vec[1]))
        self.lineedit_RD2.setText(str(vec[2]))

    def set_td_dir(self, vec):
        self.lineedit_TD0.setText(str(vec[0]))
        self.lineedit_TD1.setText(str(vec[1]))
        self.lineedit_TD2.setText(str(vec[2]))

    def set_nd_dir(self, vec):
        self.lineedit_ND0.setText(str(vec[0]))
        self.lineedit_ND1.setText(str(vec[1]))
        self.lineedit_ND2.setText(str(vec[2]))

    def set_max_ind(self, ind):
        self.spnIndex.setMaximum(ind)

    def set_angle_steps(self):
        step_size = self.get_step_size()
        for ang in self.gonio_angles:
            ang.setSingleStep(step_size)

    def set_num_gonios(self, val):
        self.spnNumAxes.setValue(val)

    def set_current_index(self, val):
        self.spnIndex.setValue(val + 1)

    def set_vecs(self, vecs):
        for i in range(6):
            self.gonio_vecs[i].setText(vecs[i])

    def set_senses(self, senses):
        for i in range(6):
            self.gonio_senses[i].setCurrentText(senses[i])

    def set_angles(self, angles):
        for i in range(6):
            self.gonio_angles[i].setValue(angles[i])

    def set_show_mu(self, val):
        return self.chkMu.setChecked(val)

    def set_material(self):
        return self.edtMaterial.setText()

    def _setup_pf_plot(self):
        self.pf_figure = Figure(layout="constrained")
        self.pf_canvas = FigureCanvas(self.pf_figure)
        self.pf_canvas.setMinimumHeight(400)
        self.pf_ax = self.pf_figure.add_subplot()

        self.pf_toolbar = NavigationToolbar(self.pf_canvas, self)

        layout = QVBoxLayout()
        self.projCanvas.setLayout(layout)
        layout.addWidget(self.pf_toolbar)
        layout.addWidget(self.pf_canvas)

    def _setup_lab_plot(self):
        self.lab_figure = Figure(layout="constrained")
        self.lab_canvas = FigureCanvas(self.lab_figure)
        self.lab_canvas.setMinimumHeight(400)
        self.lab_ax = self.lab_figure.add_subplot(projection="3d")
        self.lab_ax.view_init(vertical_axis="y")

        layout = QVBoxLayout()
        self.labCanvas.setLayout(layout)
        layout.addWidget(self.lab_canvas)

    def set_instrument_options(self, instrs):
        self.cmbInstr.insertItems(0, instrs)

    @staticmethod
    def set_gonio_axis_enabled(gonio, val):
        gonio.setEnabled(val)

    def hide_axis_columns(self):
        for i in range(6):
            self.tableWidget.setColumnHidden(i, not i < self.get_num_gonios())

    def set_angle_limits(self):
        for angs in self.gonio_angles:
            angs.setMinimum(-360)
            angs.setMaximum(360)
        for angs in self.init_angles:
            angs.setMinimum(-360)
            angs.setMaximum(360)

    def set_translation_step(self, step_size):
        self.spnInitPX.setSingleStep(step_size)
        self.spnInitPY.setSingleStep(step_size)
        self.spnInitPZ.setSingleStep(step_size)

    def setup_group_options(self, groups):
        self.cmbGroup.blockSignals(True)
        try:
            self.cmbGroup.clear()
            self.cmbGroup.addItems(groups)
        finally:
            self.cmbGroup.blockSignals(False)

    def create_workspace_table(self):
        table_column_headers = ("Axis0", "Axis1", "Axis2", "Axis3", "Axis4", "Axis5", "Include", "Select")
        n_col = len(table_column_headers)
        self.tableWidget.setColumnCount(n_col)
        self.tableWidget.setHorizontalHeaderLabels(table_column_headers)
        self.tableWidget.setRowCount(1)

        header = self.tableWidget.horizontalHeader()
        [header.setSectionResizeMode(ind, QHeaderView.Stretch) for ind in range(n_col - 2)]
        header.setSectionResizeMode(n_col - 1, QHeaderView.ResizeToContents)

    def add_table_row(self, row, axes_strings, include, select):
        # axes:
        for i in range(6):
            self.tableWidget.setItem(row, i, QTableWidgetItem(axes_strings[i]))

        # include box
        inc_checkbox = QCheckBox()
        self.add_checkbox(inc_checkbox, include, self._on_any_include_toggled, row, 6)

        # select box
        select_checkbox = QCheckBox()
        self.add_checkbox(select_checkbox, select, self._on_any_select_toggled, row, 7)

    def add_checkbox(self, checkbox, val, slot, row, col):
        checkbox.setChecked(val)
        checkbox.toggled.connect(slot)
        cell_widget = QWidget()
        layout = QHBoxLayout(cell_widget)
        layout.addWidget(checkbox)
        layout.setAlignment(QtCore.Qt.AlignCenter)
        layout.setContentsMargins(0, 0, 0, 0)
        self.tableWidget.setCellWidget(row, col, cell_widget)

    def set_load_stl_enabled(self, enabled):
        self.btnSTL.setEnabled(enabled)

    def set_load_xml_enabled(self, enabled):
        self.btnXML.setEnabled(enabled)

    def set_load_orientation_enabled(self, enabled):
        self.btnOrient.setEnabled(enabled)

    def set_outputs_enabled(self, enabled):
        self.toSscanss.setEnabled(enabled)
        self.toEuler.setEnabled(enabled)
        self.toMatrix.setEnabled(enabled)

    def _read_checkbox_column_states(self, col):
        checked = []
        for row in range(self.tableWidget.rowCount()):
            cell_widget = self.tableWidget.cellWidget(row, col)
            if cell_widget:
                checkbox = cell_widget.findChild(QCheckBox)
                if checkbox and checkbox.isChecked():
                    checked.append(row)
        return checked

    @staticmethod
    def make_box_toggleable(box, slot=None, initial_state=False):
        box.setCheckable(True)
        box.setChecked(initial_state)
        if slot:
            slot(initial_state)
            box.toggled.connect(slot)

    def set_init_rotations_visible(self, vis):
        self.set_box_children_visible(self.initOrientation, vis)

    def set_init_position_visible(self, vis):
        self.set_box_children_visible(self.initPosition, vis)

    def set_sample_directions_visible(self, vis):
        self.set_box_children_visible(self.grpDirectionWidgets, vis)

    @staticmethod
    def set_box_children_visible(box, vis):
        children = box.findChildren(QWidget)
        for widget in children:
            widget.setVisible(vis)

    def set_material_visible(self, vis):
        self.edtMaterial.setVisible(vis)
        self.lblMaterial.setVisible(vis)
        self.combo_shapeMethod.setVisible(vis)
        self.finder_gauge_vol.setVisible(vis)
        self.setGV.setVisible(vis)

    def set_finder_gauge_vol_enabled(self, enabled):
        self.finder_gauge_vol.setEnabled(enabled)

    def set_set_gauge_vol_enabled(self, enabled):
        self.setGV.setEnabled(enabled)

    def set_on_gauge_vol_state_changed(self, slot):
        self.combo_shapeMethod.currentIndexChanged.connect(slot)

    def set_on_gauge_vol_file_changed(self, slot):
        self.finder_gauge_vol.filesFoundChanged.connect(slot)

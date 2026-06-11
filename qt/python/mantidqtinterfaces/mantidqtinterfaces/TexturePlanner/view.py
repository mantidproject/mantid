# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QMainWindow, QHeaderView, QTableWidgetItem, QCheckBox, QWidget, QHBoxLayout, QPushButton, QToolBar, QGroupBox
from qtpy import QtCore
from qtpy.QtGui import QCloseEvent
from mantidqt.utils.qt import load_ui
from mantidqt.icons import get_icon
from mantid.kernel import FeatureType
from mantid import UsageService
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from qtpy.QtWidgets import QVBoxLayout
from functools import partial
from typing import Callable, List, Tuple

Ui_texplan, _ = load_ui(__file__, "texture_planner.ui")

# Label shown in the instrument combo for the user-defined instrument option.
CUSTOM_INSTRUMENT = "Custom"

# Export-format labels shown in the output combo (the presenter dispatches on these strings).
EXPORT_SSCANSS = "Sscanss2 Angles"
EXPORT_EULER = "Euler Orientation File"
EXPORT_MATRIX = "Matrix Orientation File"
EXPORT_REFERENCE_WS = "Reference Workspace"
EXPORT_TRANSMISSION_WEIGHTING = "Transmission Weighting"
# Always-available formats; the transmission weighting option is added/removed dynamically
# depending on whether transmission values have been estimated.
BASE_EXPORT_FORMATS = (EXPORT_SSCANSS, EXPORT_EULER, EXPORT_MATRIX, EXPORT_REFERENCE_WS)
_REFERENCE_WS_TOOLTIP = (
    "Save the sample (shape, position, initial orientation and material) as a NeXus reference "
    'workspace compatible with the EngDiff "Load Reference Workspace" option.'
)
_TRANSMISSION_WEIGHTING_TOOLTIP = (
    "One weighting per included orientation: the smallest transmission factor of each orientation "
    "normalised against the largest across all orientations (least absorbing = 1.0, more absorbing > 1.0)."
)
_SSCANSS_TOOLTIP = "Write the included orientations to a .angles file that can be loaded into SScanSS-2."
_EULER_TOOLTIP = "Write the included orientations to an Euler angle .txt file (using the axes defined in the settings)."
_MATRIX_TOOLTIP = "Write the included orientations to a flattened rotation matrix .txt file."


class TexturePlannerView(QMainWindow, Ui_texplan):
    sig_select_state_changed = QtCore.Signal()
    sig_include_state_changed = QtCore.Signal()
    # emitted from the SetSampleMaterial dialog's algorithm-finished callback (which runs on the
    # algorithm worker thread) to hop back onto the GUI thread before touching workspaces/plots
    sig_material_set = QtCore.Signal()

    def __init__(self, parent=None, presenter=None):
        super().__init__(parent)
        self.setupUi(self)
        self.presenter = presenter
        # invoked from closeEvent so the presenter can tear down (e.g. remove this instance's
        # workspaces); set via set_on_close
        self._on_close = None

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
        self.finder_gauge_vol.isOptional(True)

        self.finder_grouping.setLabelText("Grouping File")
        self.finder_grouping.allowMultipleFiles(False)
        self.finder_grouping.setFileExtensions([".xml"])
        self.finder_grouping.isOptional(True)

        # custom instrument / grouping controls are only shown when the relevant
        # custom option is selected (see presenter.update_custom_widgets_visibility)
        self.set_custom_instrument_name_visible(False)
        self.set_grouping_finder_visible(False)

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
        self.populate_export_formats()
        self.set_outputs_enabled(False)
        self.set_show_transmission(False)

        self.set_angle_limits()
        self.set_translation_step(0.001)  # setup 1mm translational steps
        self.set_translation_limits(-1e4, 1e4)

        self.create_workspace_table()
        self.hide_axis_columns()

        self.init_tool_tips()

        self.make_box_toggleable(self.grpLoadShape, self.set_load_shape_visible, initial_state=True)
        self.make_box_toggleable(self.grpSetMaterial, self.set_set_material_visible)
        self.make_box_toggleable(self.initOrientation, self.set_init_rotations_visible)
        self.make_box_toggleable(self.initPosition, self.set_init_position_visible)
        self.make_box_toggleable(self.grpDirectionWidgets, self.set_sample_directions_visible)
        self.make_box_toggleable(self.grpOrientationFile)  # finder widget has some hidden features that toggling messes with
        self.make_box_toggleable(self.grpGaugeVol, self.set_gauge_vol_visible)

        self._setup_settings_toolbar()

        # register startup
        UsageService.registerFeatureUsage(FeatureType.Interface, "TexturePlanner", False)

    def _setup_settings_toolbar(self) -> None:
        toolbar = QToolBar("Main Toolbar", self)
        self.btn_settings = QPushButton()
        self.btn_settings.setIcon(get_icon("mdi.settings", "black", 1.2))
        self.btn_settings.setToolTip("Settings")
        toolbar.addWidget(self.btn_settings)
        self.addToolBar(QtCore.Qt.BottomToolBarArea, toolbar)

    def set_on_settings_clicked(self, slot: Callable) -> None:
        self.btn_settings.clicked.connect(slot)

    def set_on_close(self, slot: Callable) -> None:
        self._on_close = slot

    def closeEvent(self, event: QCloseEvent):
        if self._on_close is not None:
            self._on_close()
        super().closeEvent(event)

    def init_tool_tips(self) -> None:
        # Sample setup
        self.btnSTL.setToolTip("Load a sample shape from a mesh (.stl) file.")
        self.btnXML.setToolTip("Load a sample shape from a Constructive Solid Geometry (CSG) definition (.xml file).")
        self.btnSetMaterial.setToolTip("Set the sample material using the SetSampleMaterial algorithm.")
        for spn, axis in ((self.spnInitX, "X"), (self.spnInitY, "Y"), (self.spnInitZ, "Z")):
            spn.setToolTip(
                f"Rotation about the instrument {axis} axis defining the shape's initial orientation, "
                "before the sample positioner repositions it (degrees)."
            )
        for spn, axis in ((self.spnInitPX, "X"), (self.spnInitPY, "Y"), (self.spnInitPZ, "Z")):
            spn.setToolTip(
                f"Starting {axis} position of the shape relative to the instrument components, before the sample positioner repositions it."
            )

        # Sample directions
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
        self.updateDirs.setToolTip("Apply the entered sample directions. Must be clicked for direction changes to take effect.")

        # Experimental setup
        self.cmbInstr.setToolTip(
            "Select the instrument: a fully supported instrument (ENGINX or IMAT), or Custom for another Mantid instrument."
        )
        self.cmbGroup.setToolTip(
            "Select the virtual detector grouping. Pre-set options exist for ENGINX and IMAT; custom XML definitions can also be loaded."
        )
        self.edt_custom_instr.setToolTip(
            "Name of the Mantid instrument to use. The box turns red and Update Instrument stays disabled "
            "until the text matches a known instrument."
        )
        self.finder_grouping.setToolTip(
            "Detector grouping file to use. If it is incompatible with the selected instrument, Update Instrument stays disabled."
        )
        self.btnUpdateInstr.setToolTip("Update the experiment with the current instrument and grouping options.")
        self.combo_shapeMethod.setToolTip("Gauge volume definition: choose a pre-set, a custom CSG definition, or No Gauge Volume.")
        self.finder_gauge_vol.setToolTip("Custom gauge volume CSG (.xml) definition.")
        self.setGV.setToolTip("Apply the current gauge volume definition to the experiment.")
        self.btnOrient.setToolTip(
            "Load orientations from a .txt file of Euler angles or flattened rotation matrices; "
            "the type is detected from the number of entries per row."
        )

        # Goniometers
        self.spnNumAxes.setToolTip("Number of active goniometer axes for the sample positioner.")
        self.spnStepSize.setToolTip("Degrees incremented or decremented by a single arrow click on the Angle fields.")
        self.spnIndex.setToolTip("Row of the Orientation Table shown in the Axes below and highlighted in the Pole Figure Display.")
        self.addOrientation.setToolTip("Add a duplicate of the current orientation to the end of the Orientation Table.")
        for i in range(6):
            getattr(self, f"edtVec{i}").setToolTip(f"Rotation axis vector for goniometer axis {i} (axis 0 is the outermost).")
            getattr(self, f"cmbSense{i}").setToolTip(f"Sense of rotation for goniometer axis {i}.")
            getattr(self, f"spnAngle{i}").setToolTip(f"Rotation angle for goniometer axis {i} (degrees).")

        # Displays
        self.grpSampleFigure.setToolTip(
            "Lab view: the sample oriented on the chosen instrument. Visibility of overlays is set in the settings."
        )
        self.grpPoleFigure.setToolTip(
            "Pole figure coverage for the included orientations (or estimated transmission values when that option is enabled)."
        )
        self.chkTransmission.setToolTip(
            "Show estimated transmission values per virtual detector (Monte Carlo) instead of pole figure coverage."
        )

        # Orientation table
        self.tableWidget.setToolTip(
            "Axis information for every orientation. Use Include to choose orientations for the pole figure and outputs, "
            "and Select to mark rows for deletion."
        )
        self.selectAll.setToolTip("Tick the Select box on every orientation.")
        self.deselectAll.setToolTip("Clear the Select box on every orientation.")
        self.deleteSelected.setToolTip("Remove all orientations whose Select box is ticked.")

        # Outputs
        self.finder_save_dir.setToolTip("Directory to save the output files into.")
        self.saveFileLine.setToolTip("File name for the output file.")
        self.cmbExportFormat.setToolTip("Type of file to write.")
        self.btnExport.setToolTip("Write the selected export file to the save directory.")

    def set_on_num_gonio_updated(self, slot: Callable) -> None:
        self.spnNumAxes.valueChanged.connect(slot)

    def set_on_step_updated(self, slot: Callable) -> None:
        self.spnStepSize.valueChanged.connect(slot)

    def set_on_update_dirs(self, slot: Callable) -> None:
        self.updateDirs.clicked.connect(slot)

    def set_on_gonio_vec_updated(self, slot: Callable) -> None:
        for i, gonio_vec in enumerate(self.gonio_vecs):
            gonio_vec.editingFinished.connect(partial(slot, i))

    def set_on_gonio_sense_updated(self, slot: Callable) -> None:
        for i, gonio_sense in enumerate(self.gonio_senses):
            gonio_sense.currentTextChanged.connect(partial(slot, i))

    def set_on_gonio_angle_updated(self, slot: Callable) -> None:
        for i, gonio_angle in enumerate(self.gonio_angles):
            gonio_angle.valueChanged.connect(partial(slot, i))

    def set_on_group_changed(self, slot: Callable) -> None:
        self.cmbGroup.currentTextChanged.connect(slot)

    def set_on_add_orientation_clicked(self, slot: Callable) -> None:
        self.addOrientation.clicked.connect(slot)

    def set_on_current_index_changed(self, slot: Callable) -> None:
        self.spnIndex.valueChanged.connect(slot)

    def set_on_stl_file_changed(self, slot: Callable) -> None:
        self.finder_stl.fileFindingFinished.connect(slot)

    def set_on_xml_file_changed(self, slot: Callable) -> None:
        self.finder_xml.fileFindingFinished.connect(slot)

    def set_on_orient_file_changed(self, slot: Callable) -> None:
        self.finder_orient.fileFindingFinished.connect(slot)

    def set_on_save_dir_changed(self, slot: Callable) -> None:
        self.finder_save_dir.fileFindingFinished.connect(slot)

    def set_on_save_file_changed(self, slot: Callable) -> None:
        self.saveFileLine.textEdited.connect(slot)

    def set_on_load_stl_clicked(self, slot: Callable) -> None:
        self.btnSTL.clicked.connect(slot)

    def set_on_load_xml_clicked(self, slot: Callable) -> None:
        self.btnXML.clicked.connect(slot)

    def set_on_set_material_clicked(self, slot: Callable) -> None:
        self.btnSetMaterial.clicked.connect(slot)

    def set_on_material_set(self, slot: Callable) -> None:
        self.sig_material_set.connect(slot)

    def signal_material_set(self) -> None:
        self.sig_material_set.emit()

    def set_current_material(self, material: str) -> None:
        # material is the chemical formula / name currently set on the sample, or "" if unset
        self.lblCurrentMaterialValue.setText(material or "Not set")

    def set_on_load_orient_clicked(self, slot: Callable) -> None:
        self.btnOrient.clicked.connect(slot)

    def set_on_set_gauge_volume_clicked(self, slot: Callable) -> None:
        self.setGV.clicked.connect(slot)

    def set_on_select_all_clicked(self, slot: Callable) -> None:
        self.selectAll.clicked.connect(slot)

    def set_on_deselect_all_clicked(self, slot: Callable) -> None:
        self.deselectAll.clicked.connect(slot)

    def set_on_delete_selected_clicked(self, slot: Callable) -> None:
        self.deleteSelected.clicked.connect(slot)

    def populate_export_formats(self) -> None:
        self.cmbExportFormat.addItems(BASE_EXPORT_FORMATS)
        # per-item hints so the documented behaviour of each format is discoverable in the combo
        item_tooltips = {
            EXPORT_SSCANSS: _SSCANSS_TOOLTIP,
            EXPORT_EULER: _EULER_TOOLTIP,
            EXPORT_MATRIX: _MATRIX_TOOLTIP,
            EXPORT_REFERENCE_WS: _REFERENCE_WS_TOOLTIP,
        }
        for text, tooltip in item_tooltips.items():
            idx = self.cmbExportFormat.findText(text)
            if idx != -1:
                self.cmbExportFormat.setItemData(idx, tooltip, QtCore.Qt.ToolTipRole)

    def get_export_format(self) -> str:
        return self.cmbExportFormat.currentText()

    def set_transmission_weighting_available(self, available: bool) -> None:
        # the transmission weighting export only makes sense once transmission has been estimated,
        # so its entry is added to / removed from the combo as the estimate is toggled
        idx = self.cmbExportFormat.findText(EXPORT_TRANSMISSION_WEIGHTING)
        if available and idx == -1:
            self.cmbExportFormat.addItem(EXPORT_TRANSMISSION_WEIGHTING)
            new_idx = self.cmbExportFormat.findText(EXPORT_TRANSMISSION_WEIGHTING)
            self.cmbExportFormat.setItemData(new_idx, _TRANSMISSION_WEIGHTING_TOOLTIP, QtCore.Qt.ToolTipRole)
        elif not available and idx != -1:
            self.cmbExportFormat.removeItem(idx)

    def set_on_export_clicked(self, slot: Callable) -> None:
        self.btnExport.clicked.connect(slot)

    def set_on_show_transmission_toggled(self, slot: Callable) -> None:
        self.chkTransmission.toggled.connect(slot)

    def set_on_init_x_changed(self, slot: Callable) -> None:
        self.spnInitX.valueChanged.connect(slot)

    def set_on_init_y_changed(self, slot: Callable) -> None:
        self.spnInitY.valueChanged.connect(slot)

    def set_on_init_z_changed(self, slot: Callable) -> None:
        self.spnInitZ.valueChanged.connect(slot)

    def set_on_init_px_changed(self, slot: Callable) -> None:
        self.spnInitPX.valueChanged.connect(slot)

    def set_on_init_py_changed(self, slot: Callable) -> None:
        self.spnInitPY.valueChanged.connect(slot)

    def set_on_init_pz_changed(self, slot: Callable) -> None:
        self.spnInitPZ.valueChanged.connect(slot)

    def set_on_instrument_changed(self, slot: Callable) -> None:
        self.cmbInstr.currentTextChanged.connect(slot)

    def set_on_custom_instrument_name_changed(self, slot: Callable) -> None:
        self.edt_custom_instr.textChanged.connect(slot)

    def set_on_custom_instrument_name_committed(self, slot: Callable) -> None:
        # fires when editing finishes (focus lost / Enter), gating the expensive grouping check
        self.edt_custom_instr.editingFinished.connect(slot)

    def set_on_grouping_file_changed(self, slot: Callable) -> None:
        self.finder_grouping.fileFindingFinished.connect(slot)

    def set_on_update_instrument_clicked(self, slot: Callable) -> None:
        self.btnUpdateInstr.clicked.connect(slot)

    @QtCore.Slot(bool)
    def _on_any_include_toggled(self) -> None:
        self.sig_include_state_changed.emit()

    @QtCore.Slot(bool)
    def _on_any_select_toggled(self) -> None:
        self.sig_select_state_changed.emit()

    # getters
    def get_stl_string(self) -> str:
        fnames = self.finder_stl.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_xml_string(self) -> str:
        fnames = self.finder_xml.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_orientation_file(self) -> str:
        fnames = self.finder_orient.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_save_dir(self) -> str:
        fnames = self.finder_save_dir.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_save_filename(self) -> str:
        return self.saveFileLine.text()

    def get_rd_name(self) -> str:
        return self.lineedit_RD.text()

    def get_nd_name(self) -> str:
        return self.lineedit_ND.text()

    def get_td_name(self) -> str:
        return self.lineedit_TD.text()

    def get_rd_dir(self) -> str:
        return ",".join([self.lineedit_RD0.text(), self.lineedit_RD1.text(), self.lineedit_RD2.text()])

    def get_td_dir(self) -> str:
        return ",".join([self.lineedit_TD0.text(), self.lineedit_TD1.text(), self.lineedit_TD2.text()])

    def get_nd_dir(self) -> str:
        return ",".join([self.lineedit_ND0.text(), self.lineedit_ND1.text(), self.lineedit_ND2.text()])

    def get_num_gonios(self) -> int:
        return int(self.spnNumAxes.value())

    def get_step_size(self) -> float:
        return self.spnStepSize.value()

    def get_lab_figure(self) -> Figure:
        return self.lab_figure

    def get_lab_ax(self) -> Axes:
        return self.lab_ax

    def get_pf_ax(self) -> Axes:
        return self.pf_ax

    def get_vecs(self) -> List[str]:
        return [x.text() for x in self.gonio_vecs]

    def get_senses(self) -> List[str]:
        return [x.currentText() for x in self.gonio_senses]

    def get_angles(self) -> List[str]:
        return [x.text() for x in self.gonio_angles]

    def get_group(self) -> str:
        return self.cmbGroup.currentText()

    def get_instrument(self) -> str:
        return self.cmbInstr.currentText()

    def get_custom_instrument_name(self) -> str:
        return self.edt_custom_instr.text().strip()

    def get_grouping_file(self) -> str:
        fnames = self.finder_grouping.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_current_index(self) -> int:
        return int(self.spnIndex.value()) - 1

    def get_include_indices(self) -> List[int]:
        return self._read_checkbox_column_states(6)

    def get_select_indices(self) -> List[int]:
        return self._read_checkbox_column_states(7)

    def get_show_transmission(self) -> List[int]:
        return self.chkTransmission.isChecked()

    def get_init_x(self) -> float:
        return self.spnInitX.value()

    def get_init_y(self) -> float:
        return self.spnInitY.value()

    def get_init_z(self) -> float:
        return self.spnInitZ.value()

    def get_init_px(self) -> float:
        return self.spnInitPX.value()

    def get_init_py(self) -> float:
        return self.spnInitPY.value()

    def get_init_pz(self) -> float:
        return self.spnInitPZ.value()

    def get_shape_method(self) -> str:
        return self.combo_shapeMethod.currentText()

    def get_custom_shape(self) -> str | None:
        fnames = self.finder_gauge_vol.getFilenames()
        return fnames[0] if len(fnames) > 0 else None

    # setters

    def set_rd_name(self, text: str) -> None:
        self.lineedit_RD.setText(text)

    def set_nd_name(self, text: str) -> None:
        self.lineedit_ND.setText(text)

    def set_td_name(self, text: str) -> None:
        self.lineedit_TD.setText(text)

    def set_rd_dir(self, vec: Tuple[int | float, int | float, int | float]) -> None:
        self.lineedit_RD0.setText(str(vec[0]))
        self.lineedit_RD1.setText(str(vec[1]))
        self.lineedit_RD2.setText(str(vec[2]))

    def set_td_dir(self, vec: Tuple[int | float, int | float, int | float]) -> None:
        self.lineedit_TD0.setText(str(vec[0]))
        self.lineedit_TD1.setText(str(vec[1]))
        self.lineedit_TD2.setText(str(vec[2]))

    def set_nd_dir(self, vec: Tuple[int | float, int | float, int | float]) -> None:
        self.lineedit_ND0.setText(str(vec[0]))
        self.lineedit_ND1.setText(str(vec[1]))
        self.lineedit_ND2.setText(str(vec[2]))

    def set_max_ind(self, ind: int) -> None:
        self.spnIndex.setMaximum(ind)

    def set_step_size(self, val: float) -> None:
        self.spnStepSize.setValue(val)
        self.set_angle_steps()

    def set_angle_steps(self) -> None:
        step_size = self.get_step_size()
        for ang in self.gonio_angles:
            ang.setSingleStep(step_size)

    def set_num_gonios(self, val: int) -> None:
        self.spnNumAxes.setValue(val)

    def set_current_index(self, val: int) -> None:
        self.spnIndex.setValue(val + 1)

    def set_vecs(self, vecs: List[str]) -> None:
        for i in range(6):
            self.gonio_vecs[i].setText(vecs[i])

    def set_senses(self, senses: List[str]) -> None:
        for i in range(6):
            self.gonio_senses[i].setCurrentText(senses[i])

    def set_angles(self, angles: List[str]) -> None:
        for i in range(6):
            self.gonio_angles[i].setValue(angles[i])

    def set_show_transmission(self, check: bool) -> None:
        self.chkTransmission.setChecked(check)

    def _setup_pf_plot(self) -> None:
        self.pf_figure = Figure(layout="constrained")
        self.pf_canvas = FigureCanvas(self.pf_figure)
        self.pf_canvas.setMinimumHeight(400)
        self.pf_ax = self.pf_figure.add_subplot()

        self.pf_toolbar = NavigationToolbar(self.pf_canvas, self)

        layout = QVBoxLayout()
        self.projCanvas.setLayout(layout)
        layout.addWidget(self.pf_toolbar)
        layout.addWidget(self.pf_canvas)

    def _setup_lab_plot(self) -> None:
        self.lab_figure = Figure(layout="constrained")
        self.lab_canvas = FigureCanvas(self.lab_figure)
        self.lab_canvas.setMinimumHeight(400)
        self.lab_ax = self.lab_figure.add_subplot(projection="3d")
        self.lab_ax.view_init(vertical_axis="y")

        layout = QVBoxLayout()
        self.labCanvas.setLayout(layout)
        layout.addWidget(self.lab_canvas)

    def set_instrument_options(self, instrs: List[str]) -> None:
        self.cmbInstr.insertItems(0, instrs)
        self.cmbInstr.addItem(CUSTOM_INSTRUMENT)

    def is_custom_instrument(self) -> bool:
        return self.cmbInstr.currentText() == CUSTOM_INSTRUMENT

    @staticmethod
    def set_gonio_axis_enabled(gonio, check: bool) -> None:
        gonio.setEnabled(check)

    def hide_axis_columns(self) -> None:
        for i in range(6):
            self.tableWidget.setColumnHidden(i, not i < self.get_num_gonios())

    def set_angle_limits(self) -> None:
        for angs in self.gonio_angles:
            angs.setMinimum(-360)
            angs.setMaximum(360)
        for angs in self.init_angles:
            angs.setMinimum(-360)
            angs.setMaximum(360)

    def set_translation_step(self, step_size: float) -> None:
        self.spnInitPX.setSingleStep(step_size)
        self.spnInitPY.setSingleStep(step_size)
        self.spnInitPZ.setSingleStep(step_size)

    def set_translation_limits(self, min: float, max: float) -> None:
        self.spnInitPX.setMinimum(min)
        self.spnInitPX.setMaximum(max)
        self.spnInitPY.setMinimum(min)
        self.spnInitPY.setMaximum(max)
        self.spnInitPZ.setMinimum(min)
        self.spnInitPZ.setMaximum(max)

    def setup_group_options(self, groups: List[str]) -> None:
        self.cmbGroup.blockSignals(True)
        try:
            self.cmbGroup.clear()
            self.cmbGroup.addItems(groups)
        finally:
            self.cmbGroup.blockSignals(False)

    def set_group_enabled(self, enabled: bool) -> None:
        # disabled when a custom instrument locks the group to its only (custom) option
        self.cmbGroup.setEnabled(enabled)

    def set_custom_instrument_name_visible(self, visible: bool) -> None:
        self.lblCustomInstr.setVisible(visible)
        self.edt_custom_instr.setVisible(visible)

    def set_grouping_finder_visible(self, visible: bool) -> None:
        self.finder_grouping.setVisible(visible)

    def clear_grouping_file(self) -> None:
        self.finder_grouping.clear()

    def set_grouping_file_problem(self, message: str) -> None:
        # empty message clears the problem indicator on the finder
        self.finder_grouping.setFileProblem(message)

    def set_custom_instrument_valid(self, valid: bool) -> None:
        # red border while the typed name does not match a known IDF
        self.edt_custom_instr.setStyleSheet("" if valid else "QLineEdit { border: 1px solid red; }")

    def set_update_instrument_enabled(self, enabled: bool) -> None:
        self.btnUpdateInstr.setEnabled(enabled)

    def create_workspace_table(self) -> None:
        table_column_headers = ("Axis0", "Axis1", "Axis2", "Axis3", "Axis4", "Axis5", "Include", "Select")
        n_col = len(table_column_headers)
        self.tableWidget.setColumnCount(n_col)
        self.tableWidget.setHorizontalHeaderLabels(table_column_headers)
        self.tableWidget.setRowCount(1)

        header = self.tableWidget.horizontalHeader()
        [header.setSectionResizeMode(ind, QHeaderView.Stretch) for ind in range(n_col - 2)]
        header.setSectionResizeMode(n_col - 1, QHeaderView.ResizeToContents)

    def add_table_row(self, row: int, axes_strings: List[str], include: bool, select: bool) -> None:
        # axes:
        for i in range(6):
            self.tableWidget.setItem(row, i, QTableWidgetItem(axes_strings[i]))

        # include box
        inc_checkbox = QCheckBox()
        self.add_checkbox(inc_checkbox, include, self._on_any_include_toggled, row, 6)

        # select box
        select_checkbox = QCheckBox()
        self.add_checkbox(select_checkbox, select, self._on_any_select_toggled, row, 7)

    def add_checkbox(self, checkbox: QCheckBox, val: bool, slot: Callable | None, row: int, col: int) -> None:
        checkbox.setChecked(val)
        checkbox.toggled.connect(slot)
        cell_widget = QWidget()
        layout = QHBoxLayout(cell_widget)
        layout.addWidget(checkbox)
        layout.setAlignment(QtCore.Qt.AlignCenter)
        layout.setContentsMargins(0, 0, 0, 0)
        self.tableWidget.setCellWidget(row, col, cell_widget)

    def set_load_stl_enabled(self, enabled: bool) -> None:
        self.btnSTL.setEnabled(enabled)

    def set_load_xml_enabled(self, enabled: bool) -> None:
        self.btnXML.setEnabled(enabled)

    def set_load_orientation_enabled(self, enabled: bool) -> None:
        self.btnOrient.setEnabled(enabled)

    def set_outputs_enabled(self, enabled: bool) -> None:
        self.btnExport.setEnabled(enabled)

    def _read_checkbox_column_states(self, col: int) -> bool:
        checked = []
        for row in range(self.tableWidget.rowCount()):
            cell_widget = self.tableWidget.cellWidget(row, col)
            if cell_widget:
                checkbox = cell_widget.findChild(QCheckBox)
                if checkbox and checkbox.isChecked():
                    checked.append(row)
        return checked

    @staticmethod
    def make_box_toggleable(box: QGroupBox, slot: Callable | None = None, initial_state: bool = False) -> None:
        box.setCheckable(True)
        box.setChecked(initial_state)
        if slot:
            slot(initial_state)
            box.toggled.connect(slot)

    def set_load_shape_visible(self, vis: bool) -> None:
        # only toggle our own widgets; FileFinderWidget has internally-hidden children
        for w in (self.finder_stl, self.btnSTL, self.label_or, self.finder_xml, self.btnXML):
            w.setVisible(vis)

    def set_set_material_visible(self, vis: bool) -> None:
        self.set_box_children_visible(self.grpSetMaterial, vis)

    def set_init_rotations_visible(self, vis: bool) -> None:
        self.set_box_children_visible(self.initOrientation, vis)

    def set_init_position_visible(self, vis: bool) -> None:
        self.set_box_children_visible(self.initPosition, vis)

    def set_sample_directions_visible(self, vis: bool) -> None:
        self.set_box_children_visible(self.grpDirectionWidgets, vis)

    @staticmethod
    def set_box_children_visible(box: QGroupBox, vis: bool) -> None:
        children = box.findChildren(QWidget)
        for widget in children:
            widget.setVisible(vis)

    def set_gauge_vol_visible(self, vis: bool) -> None:
        # only toggle the gauge-volume controls we own; recursing into FileFinderWidget
        # would override its internally-hidden widgets (live button, multi-entry box, etc.)
        self.combo_shapeMethod.setVisible(vis)
        self.setGV.setVisible(vis)
        self.clearGV.setVisible(vis)
        self.finder_gauge_vol.setVisible(vis and self.get_shape_method() == "Custom Shape")

    def set_finder_gauge_vol_visible(self, visible: bool) -> None:
        self.finder_gauge_vol.setVisible(visible and self.grpGaugeVol.isChecked())

    def set_set_gauge_vol_enabled(self, enabled: bool) -> None:
        self.setGV.setEnabled(enabled)

    def set_on_clear_gauge_volume_clicked(self, slot: Callable) -> None:
        self.clearGV.clicked.connect(slot)

    def set_on_gauge_vol_group_toggled(self, slot: Callable) -> None:
        self.grpGaugeVol.toggled.connect(slot)

    def set_on_gauge_vol_state_changed(self, slot: Callable) -> None:
        self.combo_shapeMethod.currentIndexChanged.connect(slot)

    def set_on_gauge_vol_file_changed(self, slot: Callable) -> None:
        self.finder_gauge_vol.filesFoundChanged.connect(slot)

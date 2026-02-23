# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtCore
from qtpy.QtWidgets import (
    QDialog,
    QDialogButtonBox,
    QDoubleSpinBox,
    QFormLayout,
    QGroupBox,
    QCheckBox,
    QComboBox,
    QLineEdit,
    QScrollArea,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)


class _WheelIgnoreFilter(QtCore.QObject):
    """Event filter that discards wheel events so spinboxes and comboboxes
    do not change value when the user scrolls without first clicking."""

    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.Wheel:
            event.ignore()
            return True
        return super().eventFilter(obj, event)


class TexturePlannerSettingsView(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Texture Planner Settings")
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowContextHelpButtonHint)
        self.setModal(True)
        self.setMinimumWidth(520)

        main_layout = QVBoxLayout(self)

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll_widget = QWidget()
        scroll_layout = QVBoxLayout(scroll_widget)
        scroll_layout.setSpacing(8)

        scroll_layout.addWidget(self._build_vis_group())
        scroll_layout.addWidget(self._build_stl_group())
        scroll_layout.addWidget(self._build_orientation_group())
        scroll_layout.addWidget(self._build_mc_group())
        scroll_layout.addWidget(self._build_attenuation_group())
        scroll_layout.addStretch()

        scroll_widget.setLayout(scroll_layout)
        scroll.setWidget(scroll_widget)
        main_layout.addWidget(scroll)

        self.button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Apply | QDialogButtonBox.Cancel)
        main_layout.addWidget(self.button_box)

        self.init_tool_tips()
        self._install_wheel_ignore_filter()

    # ================
    # Group Builders
    # ================

    def _build_vis_group(self):
        group = QGroupBox("Visualisation Settings")
        form = QFormLayout()

        self.show_directions = QCheckBox()
        form.addRow("Show Sample Directions:", self.show_directions)

        self.show_goniometers = QCheckBox()
        form.addRow("Show Goniometers:", self.show_goniometers)

        self.show_incident_beam = QCheckBox()
        form.addRow("Show Incident Beam:", self.show_incident_beam)

        self.show_ks = QCheckBox()
        form.addRow("Show Ks:", self.show_ks)

        self.show_scattered_beam = QCheckBox()
        form.addRow("Show Scattered Beams:", self.show_scattered_beam)

        group.setLayout(form)
        return group

    def _build_stl_group(self):
        group = QGroupBox("STL Loading Settings")
        form = QFormLayout()

        self.stl_scale_combo = QComboBox()
        self.stl_scale_combo.addItems(["mm", "cm", "m", "in"])
        form.addRow("Scale:", self.stl_scale_combo)

        self.stl_x_deg = QDoubleSpinBox()
        self.stl_x_deg.setRange(-360.0, 360.0)
        self.stl_x_deg.setDecimals(3)
        form.addRow("X Degrees:", self.stl_x_deg)

        self.stl_y_deg = QDoubleSpinBox()
        self.stl_y_deg.setRange(-360.0, 360.0)
        self.stl_y_deg.setDecimals(3)
        form.addRow("Y Degrees:", self.stl_y_deg)

        self.stl_z_deg = QDoubleSpinBox()
        self.stl_z_deg.setRange(-360.0, 360.0)
        self.stl_z_deg.setDecimals(3)
        form.addRow("Z Degrees:", self.stl_z_deg)

        self.stl_translation = QLineEdit()
        form.addRow("Translation Vector:", self.stl_translation)

        group.setLayout(form)
        return group

    def _build_orientation_group(self):
        group = QGroupBox("Orientation File Settings")
        form = QFormLayout()

        self.orient_axes = QLineEdit()
        form.addRow("Axes:", self.orient_axes)

        self.orient_senses = QLineEdit()
        form.addRow("Senses:", self.orient_senses)

        group.setLayout(form)
        return group

    def _build_mc_group(self):
        group = QGroupBox("Monte Carlo Absorption Settings")
        form = QFormLayout()

        self.mc_events = QSpinBox()
        self.mc_events.setRange(1, 10_000_000)
        form.addRow("Events Per Point:", self.mc_events)

        self.mc_max_scatter = QSpinBox()
        self.mc_max_scatter.setRange(1, 10_000_000)
        form.addRow("Max Scatter Point Attempts:", self.mc_max_scatter)

        self.mc_simulate_in = QComboBox()
        self.mc_simulate_in.addItems(["SampleOnly", "EnvironmentOnly", "SampleAndEnvironment"])
        form.addRow("Simulate Scattering Point In:", self.mc_simulate_in)

        self.mc_resimulate = QCheckBox()
        form.addRow("Resimulate Tracks For Different Wavelengths:", self.mc_resimulate)

        group.setLayout(form)
        return group

    def _build_attenuation_group(self):
        group = QGroupBox("Attenuation Settings")
        form = QFormLayout()

        self.att_point = QDoubleSpinBox()
        self.att_point.setRange(0.001, 100.0)
        self.att_point.setDecimals(4)
        form.addRow("Point:", self.att_point)

        self.att_unit = QComboBox()
        self.att_unit.addItems(["dSpacing", "Wavelength", "TOF"])
        form.addRow("Unit:", self.att_unit)

        self.att_material = QLineEdit()
        form.addRow("Material:", self.att_material)

        group.setLayout(form)
        return group

    # ================
    # Wheel Filter
    # ================

    def _install_wheel_ignore_filter(self):
        self._wheel_filter = _WheelIgnoreFilter(self)
        scroll_widgets = (
            self.stl_scale_combo,
            self.stl_x_deg,
            self.stl_y_deg,
            self.stl_z_deg,
            self.mc_events,
            self.mc_max_scatter,
            self.mc_simulate_in,
            self.att_point,
            self.att_unit,
        )
        for widget in scroll_widgets:
            widget.installEventFilter(self._wheel_filter)

    # ================
    # Tool Tips
    # ================

    def init_tool_tips(self):
        self.stl_scale_combo.setToolTip("Units of the STL file's coordinates")
        self.stl_x_deg.setToolTip("Rotation around the X axis applied when loading the STL file (degrees)")
        self.stl_y_deg.setToolTip("Rotation around the Y axis applied when loading the STL file (degrees)")
        self.stl_z_deg.setToolTip("Rotation around the Z axis applied when loading the STL file (degrees)")
        self.stl_translation.setToolTip("Comma-separated translation vector applied when loading the STL file, e.g. '0,0,0'")

        self.orient_axes.setToolTip("Lab-frame axes that the Euler angles in the orientation file are defined along, e.g. 'YXY'")
        self.orient_senses.setToolTip(
            "Sense of rotation around each Euler axis: 1=counterclockwise, -1=clockwise. "
            "Comma-separated, one value per axis, e.g. '-1,-1,-1'"
        )

        self.mc_events.setToolTip("Number of Monte Carlo events per detector point (higher = more accurate but slower)")
        self.mc_max_scatter.setToolTip("Maximum number of attempts to find a valid scattering point inside the sample")
        self.mc_simulate_in.setToolTip("Region in which the scattering point is simulated")
        self.mc_resimulate.setToolTip(
            "Whether to resimulate scattering tracks for different wavelengths (more accurate but significantly slower)"
        )

        self.att_point.setToolTip("Value at which to read the attenuation coefficient from the absorption workspace")
        self.att_unit.setToolTip("Unit of the attenuation point value")
        self.att_material.setToolTip("Chemical formula of the sample material (see SetSampleMaterial documentation), e.g. 'Fe'")

    # ================
    # Slot Connectors
    # ================

    def set_on_ok_clicked(self, slot):
        self.button_box.accepted.connect(slot)

    def set_on_cancel_clicked(self, slot):
        self.button_box.rejected.connect(slot)

    def set_on_apply_clicked(self, slot):
        self.button_box.button(QDialogButtonBox.Apply).clicked.connect(slot)

    # ================
    # Getters
    # ================

    def get_show_directions(self):
        return self.show_directions.isChecked()

    def get_show_goniometers(self):
        return self.show_goniometers.isChecked()

    def get_show_incident_beam(self):
        return self.show_incident_beam.isChecked()

    def get_show_ks(self):
        return self.show_ks.isChecked()

    def get_show_scattered_beam(self):
        return self.show_scattered_beam.isChecked()

    def get_stl_scale(self):
        return self.stl_scale_combo.currentText()

    def get_stl_x_deg(self):
        return self.stl_x_deg.value()

    def get_stl_y_deg(self):
        return self.stl_y_deg.value()

    def get_stl_z_deg(self):
        return self.stl_z_deg.value()

    def get_stl_translation(self):
        return self.stl_translation.text()

    def get_orient_axes(self):
        return self.orient_axes.text()

    def get_orient_senses(self):
        return self.orient_senses.text()

    def get_mc_events(self):
        return self.mc_events.value()

    def get_mc_max_scatter(self):
        return self.mc_max_scatter.value()

    def get_mc_simulate_in(self):
        return self.mc_simulate_in.currentText()

    def get_mc_resimulate(self):
        return self.mc_resimulate.isChecked()

    def get_att_point(self):
        return self.att_point.value()

    def get_att_unit(self):
        return self.att_unit.currentText()

    def get_att_material(self):
        return self.att_material.text()

    # ================
    # Setters
    # ================

    def set_show_directions(self, val):
        self.show_directions.setChecked(bool(val))

    def set_show_goniometers(self, val):
        return self.show_goniometers.setChecked(bool(val))

    def set_show_incident_beam(self, val):
        return self.show_incident_beam.setChecked(bool(val))

    def set_show_ks(self, val):
        return self.show_ks.setChecked(bool(val))

    def set_show_scattered_beam(self, val):
        return self.show_scattered_beam.setChecked(bool(val))

    def set_stl_scale(self, text):
        self.stl_scale_combo.setCurrentText(str(text))

    def set_stl_x_deg(self, val):
        self.stl_x_deg.setValue(float(val))

    def set_stl_y_deg(self, val):
        self.stl_y_deg.setValue(float(val))

    def set_stl_z_deg(self, val):
        self.stl_z_deg.setValue(float(val))

    def set_stl_translation(self, text):
        self.stl_translation.setText(str(text))

    def set_orient_axes(self, text):
        self.orient_axes.setText(str(text))

    def set_orient_senses(self, text):
        self.orient_senses.setText(str(text))

    def set_mc_events(self, val):
        self.mc_events.setValue(int(val))

    def set_mc_max_scatter(self, val):
        self.mc_max_scatter.setValue(int(val))

    def set_mc_simulate_in(self, text):
        self.mc_simulate_in.setCurrentText(str(text))

    def set_mc_resimulate(self, val):
        self.mc_resimulate.setChecked(bool(val))

    def set_att_point(self, val):
        self.att_point.setValue(float(val))

    def set_att_unit(self, text):
        self.att_unit.setCurrentText(str(text))

    def set_att_material(self, text):
        self.att_material.setText(str(text))

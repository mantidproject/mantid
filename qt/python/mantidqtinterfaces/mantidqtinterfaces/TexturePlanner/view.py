# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QMainWindow, QHeaderView, QTableWidgetItem, QCheckBox, QWidget, QHBoxLayout
from qtpy import QtCore
from mantidqt.utils.qt import load_ui
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

        self.gonio_axes = (self.axis0, self.axis1, self.axis2, self.axis3, self.axis4, self.axis5)
        self.gonio_angles = (self.spnAngle0, self.spnAngle1, self.spnAngle2, self.spnAngle3, self.spnAngle4, self.spnAngle5)
        self.gonio_senses = (self.cmbSense0, self.cmbSense1, self.cmbSense2, self.cmbSense3, self.cmbSense4, self.cmbSense5)
        self.gonio_vecs = (self.edtVec0, self.edtVec1, self.edtVec2, self.edtVec3, self.edtVec4, self.edtVec5)

        self._setup_pf_plot()
        self._setup_lab_plot()

        self.set_angle_limits()

        self.create_workspace_table()
        self.hide_axis_columns()

        # register startup
        UsageService.registerFeatureUsage(FeatureType.Interface, "TexturePlanner", False)

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

    @QtCore.Slot(bool)
    def _on_any_include_toggled(self):
        self.sig_include_state_changed.emit()

    @QtCore.Slot(bool)
    def _on_any_select_toggled(self):
        self.sig_select_state_changed.emit()

    # getters

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

    def get_current_index(self):
        return int(self.spnIndex.value()) - 1

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

    def set_vecs(self, vecs):
        for i in range(6):
            self.gonio_vecs[i].setText(vecs[i])

    def set_senses(self, senses):
        for i in range(6):
            self.gonio_senses[i].setCurrentText(senses[i])

    def set_angles(self, angles):
        for i in range(6):
            self.gonio_angles[i].setValue(angles[i])

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

        layout = QVBoxLayout()
        self.labCanvas.setLayout(layout)
        layout.addWidget(self.lab_canvas)

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

    def setup_group_options(self, groups):
        self.cmbGroup.addItems(groups)

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

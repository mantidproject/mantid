# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from qtpy.QtWidgets import QVBoxLayout
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_view import ShowSampleView
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import create_workspace_table
from typing import Callable, List, Dict, Tuple, Iterable

from workbench.plotting.mantidfigurecanvas import MantidFigureCanvas

Ui_texture, _ = load_ui(__file__, "texture_tab.ui")


class TextureView(QtWidgets.QWidget, Ui_texture):
    sig_enable_controls = QtCore.Signal(bool)
    sig_view_requested = QtCore.Signal(str)
    sig_view_shape_requested = QtCore.Signal(str)
    sig_selection_state_changed = QtCore.Signal()

    def __init__(self, parent: QtWidgets.QWidget | None = None, instrument: str = "ENGINX"):
        super(TextureView, self).__init__(parent)
        self.setupUi(self)
        self.init_tool_tips()

        self.show_sample_view = ShowSampleView()

        self.finder_texture_ws.setLabelText("Sample Run(s)")
        self.finder_texture_ws.allowMultipleFiles(True)
        self.finder_texture_ws.setFileExtensions([".nxs"])
        self.finder_texture_ws.setInstrumentOverride(instrument)

        self.finder_texture_tables.setLabelText("Fit Parameters")
        self.finder_texture_tables.allowMultipleFiles(True)
        self.finder_texture_tables.setFileExtensions([".nxs"])
        self.finder_texture_tables.isOptional(True)

        self.finder_cif_file.setLabelText("CIF File")
        self.finder_cif_file.allowMultipleFiles(False)
        self.finder_cif_file.setFileExtensions([".cif"])
        self.finder_cif_file.isOptional(True)

        self._setup_plot()

        self.table_column_headers = ["Run", "Fit Parameters", "Crystal Structure", "Sample", "Select"]

    # ========== Setup Tool Tips ==========

    def init_tool_tips(self) -> None:
        self.btn_loadWSFiles.setToolTip("Loads the selected sample runs into the table")
        self.btn_loadParamFiles.setToolTip(
            "Loads the selected table workspaces into the table, assigning them to each row currently without an assigned table, in turn"
        )
        self.btn_selectAll.setToolTip("Sets all the rows of the table as selected")
        self.btn_deselectAll.setToolTip("Sets all the rows of the table as unselected")
        self.btn_deleteSelected.setToolTip("Deletes all rows of the table which are set as selected")
        self.btn_deleteSelectedParams.setToolTip("Removes all the set parameter files for the rows of the table that have been selected")
        self.lattice_lineedit.setToolTip(
            "Tab separated string representation of the crystal lattice (eg. 'a   b   c    alpha   beta    gamma')"
        )
        self.spacegroup_lineedit.setToolTip("String representation of the crystal space group (eg. 'I m -3 m')")
        self.basis_lineedit.setToolTip("String representation of the crystal basis (eg. 'Fe 0 0 0 1.0 0.05; Fe 0.5 0.5 0.5 1.0 0.05')")
        self.combo_workspaceListProp.setToolTip("Workspace to set crystal structure on using 'Set Crystal' button")
        self.btn_setCrystal.setToolTip(
            "Takes the crystal structure defined either in the lattice/space group/basis inputs or CIF file and "
            "applies it to the workspace in the drop down menu"
        )
        self.btn_setAllCrystal.setToolTip(
            "Takes the crystal structure defined either in the lattice/space group/basis inputs or CIF file"
            " and applies it to all the selected workspaces"
        )
        self.h_lineedit.setToolTip("H index")
        self.k_lineedit.setToolTip("K index")
        self.l_lineedit.setToolTip("L index")
        self.combo_projMethod.setToolTip("The 2D projection method of displaying the Pole Figure Table data")
        self.combo_param.setToolTip(
            "Which column of the parameter table workspaces to read to find the values which will be included in the pole figure table"
        )
        self.btn_calc_pf.setToolTip("Creates the pole figure tables and plots the corresponding pole figure")

    # ========== Signal Connectors ==========
    def set_on_load_ws_clicked(self, slot: Callable) -> None:
        self.btn_loadWSFiles.clicked.connect(slot)

    def set_on_load_param_clicked(self, slot: Callable) -> None:
        self.btn_loadParamFiles.clicked.connect(slot)

    def set_enable_controls_connection(self, slot: Callable) -> None:
        self.sig_enable_controls.connect(slot)

    def set_on_delete_clicked(self, slot: Callable) -> None:
        self.btn_deleteSelected.clicked.connect(slot)

    def set_on_delete_param_clicked(self, slot: Callable) -> None:
        self.btn_deleteSelectedParams.clicked.connect(slot)

    def set_on_select_all_clicked(self, slot: Callable) -> None:
        self.btn_selectAll.clicked.connect(slot)

    def set_on_deselect_all_clicked(self, slot: Callable) -> None:
        self.btn_deselectAll.clicked.connect(slot)

    def set_on_calc_pf_clicked(self, slot: Callable) -> None:
        self.btn_calc_pf.clicked.connect(slot)

    def set_include_scatter_corr(self, val: bool) -> None:
        self.check_scatt.setChecked(val)

    def set_on_check_inc_scatt_corr_state_changed(self, slot: Callable) -> None:
        self.check_scatt.stateChanged.connect(slot)

    def set_on_set_crystal_clicked(self, slot: Callable) -> None:
        self.btn_setCrystal.clicked.connect(slot)

    def set_on_set_all_crystal_clicked(self, slot: Callable) -> None:
        self.btn_setAllCrystal.clicked.connect(slot)

    def set_on_view_shape_requested(self, slot: Callable) -> None:
        self.sig_view_shape_requested.connect(slot)

    def set_on_selection_state_changed(self, slot: Callable) -> None:
        self.sig_selection_state_changed.connect(slot)

    @QtCore.Slot(bool)
    def _on_any_checkbox_toggled(self) -> None:
        self.sig_selection_state_changed.emit()

    def on_lattice_changed(self, slot: Callable) -> None:
        self.lattice_lineedit.textChanged.connect(slot)

    def on_spacegroup_changed(self, slot: Callable) -> None:
        self.spacegroup_lineedit.textChanged.connect(slot)

    def on_basis_changed(self, slot: Callable) -> None:
        self.basis_lineedit.textChanged.connect(slot)

    def on_cif_changed(self, slot: Callable) -> None:
        self.finder_cif_file.fileFindingFinished.connect(slot)

    # ========== Table Handling ==========

    def populate_workspace_table(self, workspace_info_list: Dict[str, Dict]) -> None:
        create_workspace_table(self.table_column_headers, self.table_loaded_data, len(workspace_info_list))
        for row, (ws, metadata) in enumerate(workspace_info_list.items()):
            self._add_table_row(row, ws, metadata)

    def _add_table_row(self, row: int, ws: str, metadata: Dict[str, str]) -> None:
        # run:
        self.table_loaded_data.setItem(row, 0, QtWidgets.QTableWidgetItem(ws))

        # table ws:
        self.table_loaded_data.setItem(row, 1, QtWidgets.QTableWidgetItem(metadata.get("fit_parameters", "Not set")))

        # xtal structure:
        self.table_loaded_data.setItem(row, 2, QtWidgets.QTableWidgetItem(metadata.get("crystal", "Not set")))

        # shape_view
        self.show_sample_view.add_show_button_to_table_if_shape(
            self.sig_view_shape_requested, self.table_loaded_data, ws, row, 3, metadata.get("shape", "Not set") != "Not set"
        )

        # selection box
        checkbox = QtWidgets.QCheckBox()
        checkbox.setChecked(metadata["select"])
        checkbox.toggled.connect(self._on_any_checkbox_toggled)
        cell_widget = QtWidgets.QWidget()
        layout = QtWidgets.QHBoxLayout(cell_widget)
        layout.addWidget(checkbox)
        layout.setAlignment(QtCore.Qt.AlignCenter)
        layout.setContentsMargins(0, 0, 0, 0)
        self.table_loaded_data.setCellWidget(row, 4, cell_widget)

    def get_projection_method(self) -> str:
        return self.combo_projMethod.currentText()

    def get_hkl(self) -> Tuple[str, str, str]:
        return self.h_lineedit.text(), self.k_lineedit.text(), self.l_lineedit.text()

    def get_inc_scatt_power(self) -> bool:
        return self.check_scatt.isChecked()

    def get_lattice(self) -> str:
        return self.lattice_lineedit.text()

    def get_spacegroup(self) -> str:
        return self.spacegroup_lineedit.text()

    def get_basis(self) -> str:
        return self.basis_lineedit.text()

    def get_cif(self) -> str:
        fnames = self.finder_cif_file.getFilenames()
        return fnames[0] if len(fnames) > 0 else ""

    def get_crystal_ws_prop(self) -> str:
        return self.combo_workspaceListProp.currentText()

    def get_readout_column(self) -> str:
        return self.combo_param.currentText() if self.combo_param.isVisible else ""

    def populate_workspace_list(self, workspace_names: List[str]) -> None:
        self.combo_workspaceListProp.clear()
        self.combo_workspaceListProp.addItems(sorted(workspace_names))

    def populate_readout_column_list(self, cols: List[str], starting_index: int) -> None:
        self.combo_param.clear()
        self.combo_param.addItems(cols)
        if starting_index:
            self.combo_param.setCurrentIndex(starting_index)

    def get_selected_workspaces(self) -> Tuple[List[str], List[str]]:
        selected_wss = []
        selected_params = []
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 4)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox and checkbox.isChecked():
                    selected_wss.append(self.table_loaded_data.item(row, 0).text())
                    selected_params.append(self.table_loaded_data.item(row, 1).text())
        return selected_wss, selected_params

    def set_all_workspaces_selected(self, selected: bool) -> None:
        for row in range(self.table_loaded_data.rowCount()):
            cell_widget = self.table_loaded_data.cellWidget(row, 4)
            if cell_widget:
                checkbox = cell_widget.findChild(QtWidgets.QCheckBox)
                if checkbox:
                    checkbox.setChecked(selected)

    def set_default_files(self, filepaths: Iterable[str], directory: str | None) -> None:
        if not filepaths:
            return
        self.finder_texture_ws.setUserInput(",".join(filepaths))
        if directory:
            self.finder_texture_ws.setLastDirectory(directory)

    def _setup_plot(self) -> None:
        self.figure = Figure(layout="constrained")
        self.canvas = FigureCanvas(self.figure)
        self.canvas.setMinimumHeight(400)

        self.toolbar = NavigationToolbar(self.canvas, self)

        self.canvas.setFocusPolicy(QtCore.Qt.StrongFocus)

        layout = QVBoxLayout()
        self.plot_canvas.setLayout(layout)
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)

    def get_plot_axis(self) -> Tuple[Figure, MantidFigureCanvas]:
        return self.figure, self.canvas

    def update_crystal_section_visibility(self) -> None:
        self.set_crystal_section_visibility(self.check_scatt.isChecked())

    def set_crystal_section_visibility(self, vis: bool) -> None:
        self.widget_scattPowerContainer.setVisible(vis)

    def update_col_select_visibility(self, vis: bool) -> None:
        self.label_param.setVisible(vis)
        self.combo_param.setVisible(vis)

    def set_instrument_override(self, instrument: str) -> None:
        self.finder_texture_ws.setInstrumentOverride(instrument)

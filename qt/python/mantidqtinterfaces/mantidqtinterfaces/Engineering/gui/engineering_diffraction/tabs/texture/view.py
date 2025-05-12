from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt import load_ui
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from qtpy.QtWidgets import QVBoxLayout

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

        self._setup_plot()

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

    def set_on_calc_pf_clicked(self, slot):
        self.btn_calc_pf.clicked.connect(slot)

    def set_include_scatter_corr(self, val):
        return self.check_scatt.setChecked(val)

    def set_on_check_inc_scatt_corr_state_changed(self, slot):
        self.check_scatt.stateChanged.connect(slot)

    def set_on_set_crystal_clicked(self, slot):
        self.btn_setCrystal.clicked.connect(slot)

    def set_on_set_all_crystal_clicked(self, slot):
        self.btn_setAllCrystal.clicked.connect(slot)

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

    def get_projection_method(self):
        return self.combo_projMethod.currentText()

    def get_hkl(self):
        return self.h_lineedit.text(), self.k_lineedit.text(), self.l_lineedit.text()

    def get_inc_scatt_power(self):
        return self.check_scatt.isChecked()

    def get_lattice(self):
        return self.lattice_lineedit.text()

    def get_spacegroup(self):
        return self.spacegroup_lineedit.text()

    def get_basis(self):
        return self.basis_lineedit.text()

    def get_crystal_ws(self):
        return self.combo_workspaceList.currentText()

    def populate_workspace_list(self, workspace_names):
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

    def _setup_plot(self):
        self.figure = Figure()
        self.canvas = FigureCanvas(self.figure)
        self.canvas.setMinimumHeight(400)

        self.toolbar = NavigationToolbar(self.canvas, self)

        layout = QVBoxLayout()
        self.plot_canvas.setLayout(layout)
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)

    def get_plot_axis(self):
        return self.figure, self.canvas

    def update_crystal_section_visibility(self):
        self.set_crystal_section_visibility(self.check_scatt.isChecked())

    def set_crystal_section_visibility(self, vis):
        self.label_lattice.setVisible(vis)
        self.lattice_lineedit.setVisible(vis)
        self.label_spacegroup.setVisible(vis)
        self.spacegroup_lineedit.setVisible(vis)
        self.label_basis.setVisible(vis)
        self.basis_lineedit.setVisible(vis)
        self.combo_workspaceList.setVisible(vis)
        self.btn_setCrystal.setVisible(vis)
        self.btn_setAllCrystal.setVisible(vis)
        self.label_hkl.setVisible(vis)
        self.h_lineedit.setVisible(vis)
        self.label_comma1.setVisible(vis)
        self.k_lineedit.setVisible(vis)
        self.label_comma2.setVisible(vis)
        self.l_lineedit.setVisible(vis)
        self.label_bracket.setVisible(vis)

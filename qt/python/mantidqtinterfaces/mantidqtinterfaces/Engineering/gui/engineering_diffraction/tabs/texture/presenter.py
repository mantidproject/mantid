from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import Load
from qtpy.QtCore import QTimer
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqt.interfacemanager import InterfaceManager
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    INSTRUMENT_DICT,
    CalibrationObserver,
)
import os
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter import ShowSamplePresenter


class TexturePresenter:
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.show_sample_presenter = ShowSamplePresenter(model, view, False)

        self.ws_names = []
        self.ws_assignments = {}
        self.param_assignments = {}
        self.unassigned_params = []
        self.unassigned_wss = []
        self.ws_info = {}

        self.correction_notifier = GenericObservable()

        self.calibration_observer = CalibrationObserver(self)
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        self.view.set_on_load_ws_clicked(self.load_ws_files)
        self.view.set_on_load_param_clicked(self.load_param_files)

        self.view.set_on_select_all_clicked(self.select_all)
        self.view.set_on_deselect_all_clicked(self.deselect_all)
        self.view.set_on_delete_clicked(self.delete_selected_files)
        self.view.set_on_delete_param_clicked(self.delete_selected_param_files)

        self.view.set_on_check_inc_scatt_corr_state_changed(self.view.update_crystal_section_visibility)
        self.view.set_include_scatter_corr(False)
        self.view.set_crystal_section_visibility(False)
        self.view.set_on_load_cif_clicked(self._open_load_cif_dialog)
        self.view.set_on_copy_all_xtal_clicked(self.on_copy_all_crystal_clicked)
        self.view.set_on_set_crystal_clicked(self.on_set_crystal_clicked)
        self.view.set_on_set_all_crystal_clicked(self.on_set_all_crystal_clicked)

        self.view.set_on_calc_pf_clicked(self.on_calc_pf_clicked)

        self.view.sig_selection_state_changed.connect(self.update_readout_column_list)
        self.update_readout_column_list()

    def load_ws_files(self):
        filenames = self.view.finder_texture_ws.getFilenames()

        for path in filenames:
            ws_name = os.path.splitext(os.path.basename(path))[0]
            if ADS.doesExist(ws_name):
                logger.notice(f'A workspace "{ws_name}" already exists, loading {path} has been skipped')
            else:
                try:
                    Load(Filename=path, OutputWorkspace=ws_name)
                except Exception as e:
                    logger.warning(f"Failed to load {path}: {e}")
                    continue
            if ws_name not in self.ws_names:
                self.ws_names.append(ws_name)
            if not self.ws_has_param(ws_name):
                self.unassigned_wss.append(ws_name)

        self.redraw_table()

    def ws_has_param(self, ws):
        return ws in self.ws_assignments.keys()

    def param_has_ws(self, param):
        return param in self.param_assignments.keys()

    def get_assigned_params(self):
        return list(self.param_assignments.keys())

    def load_param_files(self):
        filenames = self.view.finder_texture_tables.getFilenames()

        for path in filenames:
            param = os.path.splitext(os.path.basename(path))[0]
            try:
                Load(Filename=path, OutputWorkspace=param)
            except Exception as e:
                logger.warning(f"Failed to load {path}: {e}")
                continue
            if not self.param_has_ws(param):
                self.unassigned_params.append(param)

        self.redraw_table()

    def delete_selected_files(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        for ws in selected_wss:
            # remove from ws_names
            self.ws_names.pop(self.ws_names.index(ws))
            if self.ws_has_param(ws):
                # remove assignment between ws and param
                param = self.ws_assignments[ws]
                self.ws_assignments.pop(ws)
                self.param_assignments.pop(param)
                # do not add either back to the unassigned piles
            else:
                self.unassigned_wss.pop(self.unassigned_wss.index(ws))
        self.redraw_table()

    def delete_selected_param_files(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        self.unassign_params(selected_params)
        self.redraw_table()

    def unassign_params(self, params):
        for param in params:
            if self.param_has_ws(param):
                # remove assignment between ws and param
                assigned_ws = self.param_assignments[param]
                self.param_assignments.pop(param)
                self.ws_assignments.pop(assigned_ws)
                # add ws to the unassigned pile
                self.unassigned_wss.append(assigned_ws)

    def assign_unpaired_wss_and_params(self):
        while (len(self.unassigned_params) > 0) and (len(self.unassigned_wss) > 0):
            ws = self.unassigned_wss.pop(0)
            param = self.unassigned_params.pop(0)
            self.ws_assignments[ws] = param
            self.param_assignments[param] = ws
        # don't want to keep an unseen stack of unassigned parameter files
        self.unassigned_params = []

    def select_all(self):
        self.view.set_all_workspaces_selected(True)

    def deselect_all(self):
        self.view.set_all_workspaces_selected(False)

    def redraw_table(self):
        self.assign_unpaired_wss_and_params()
        self.update_ws_info()
        self.view.populate_workspace_table(self.ws_info)
        self.view.populate_workspace_list(self.ws_names)
        self.update_readout_column_list()

    def update_ws_info(self):
        ws_info = {}
        selected_ws, _ = self.view.get_selected_workspaces()
        for pos_ind, ws_name in enumerate(self.ws_names):
            param = self.ws_assignments[ws_name] if self.ws_has_param(ws_name) else "Not set"
            ws_info[ws_name] = self.model.get_ws_info(ws_name, param, ws_name in selected_ws)  # maintain state of selected boxes
        self.ws_info = ws_info

    def on_set_crystal_clicked(self):
        self.model.set_ws_xtal(self.view.get_crystal_ws_prop(), self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis())
        self.redraw_table()

    def on_set_all_crystal_clicked(self):
        wss, _ = self.view.get_selected_workspaces()
        self.model.set_all_ws_xtal(wss, self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis())
        self.redraw_table()

    def on_copy_all_crystal_clicked(self):
        wss, _ = self.view.get_selected_workspaces()
        self.model.copy_xtal_to_all(self.view.get_crystal_ws_cif(), wss)
        self.redraw_table()

    def on_calc_pf_clicked(self):
        wss, params = self.view.get_selected_workspaces()
        # require at least one wss to be selected
        if len(wss) > 0:
            # remove any 'not set' parameters workspaces from the list
            params = [p for p in params if p != "Not set"]
            projection_method = self.view.get_projection_method()
            inc_scatt = self.view.get_inc_scatt_power()
            hkl = self.model.parse_hkl(*self.view.get_hkl()) if inc_scatt else None
            readout_col = self.view.get_readout_column()
            out_ws, grouping = self.model.get_pf_table_name(wss, params, hkl, readout_col)
            ax_transform, ax_labels = output_settings.get_texture_axes_transform()
            plot_exp = self._get_setting("plot_exp_pf", bool)
            contour_kernel = float(self._get_setting("contour_kernel", str))

            # default for now
            scat_vol_pos = (0.0, 0.0, 0.0)
            has_chi2_col, has_x0_col = False, False
            if self.all_wss_have_params() and self.at_least_one_param_assigned():
                has_chi2_col, has_x0_col = self.model.check_param_ws_for_columns(params)
            chi2_thresh = self._get_setting("cost_func_thresh") if has_chi2_col else 0.0
            peak_thresh = self._get_setting("peak_pos_thresh") if has_x0_col else 0.0

            self.set_worker(
                AsyncTask(
                    self.calc_pf,
                    (
                        wss,
                        params,
                        out_ws,
                        hkl,
                        projection_method,
                        inc_scatt,
                        scat_vol_pos,
                        chi2_thresh,
                        peak_thresh,
                        self.rb_num,
                        ax_transform,
                        ax_labels,
                        readout_col,
                        grouping,
                        plot_exp,
                        contour_kernel,
                    ),
                    error_cb=self._on_worker_error,
                    finished_cb=self._on_worker_success,
                )
            )
            self.worker = self.get_worker()
            self.worker.start()

    def set_worker(self, worker):
        self.worker = worker

    def get_worker(self):
        return self.worker

    def calc_pf(
        self,
        wss,
        params,
        out_ws,
        hkl,
        projection_method,
        inc_scatt,
        scat_vol_pos,
        chi2_thresh,
        peak_thresh,
        rb_num,
        ax_transform,
        ax_labels,
        readout_col,
        grouping,
        plot_exp,
        contour_kernel,
    ):
        root_dir = output_settings.get_output_path()
        save_dirs = self.model.get_save_dirs("PoleFigureTables", root_dir, rb_num, grouping)
        self.model.make_pole_figure_tables(
            wss, params, out_ws, hkl, inc_scatt, scat_vol_pos, chi2_thresh, peak_thresh, save_dirs, ax_transform, readout_col
        )
        self.plot_pf(out_ws, projection_method, readout_col, save_dirs, plot_exp, ax_labels, contour_kernel)

    def _on_worker_success(self):
        self.correction_notifier.notify_subscribers("Corrections Applied")

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))

    def _redraw_on_alg_exec(self):
        QTimer.singleShot(200, self.redraw_table)

    def _open_load_cif_dialog(self, alg_str):
        manager = InterfaceManager()
        dialog = manager.createDialogFromName("LoadCIF", -1)
        if dialog is not None:
            dialog.finished.connect(self._redraw_on_alg_exec)
            dialog.open()

    def plot_pf(self, pf_ws, proj, readout_col, save_dirs, plot_exp, ax_labels, contour_kernel):
        # Get figure and canvas from view
        fig, canvas = self.view.get_plot_axis()

        # Clear existing figure
        fig.clf()

        # if no column specified, should default to I
        readout_col = "I" if readout_col == "" else readout_col

        self.model.plot_pole_figure(
            pf_ws,
            proj,
            fig=fig,
            readout_col=readout_col,
            save_dirs=save_dirs,
            plot_exp=plot_exp,
            ax_labels=ax_labels,
            contour_kernel=contour_kernel,
        )

        canvas.draw()

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def update_calibration(self, calibration):
        """
        Update the current calibration following a call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def update_readout_column_list(self):
        params = self.get_assigned_params()
        show_col = False
        # only if all wss have param files should plotting with a column readout be an option
        if self.all_wss_have_params() and self.at_least_one_param_assigned():
            col_list, starting_index = self.model.read_param_cols(params[0])
            self.view.populate_readout_column_list(col_list, starting_index)
            # only have the option to pick a column if columns are present
            show_col = len(col_list) > 0
        self.view.update_col_select_visibility(show_col)

    def all_wss_have_params(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        valid_params = [p for p in selected_params if p != "Not set"]
        return len(selected_wss) == len(valid_params) and len(selected_wss) > 0

    def at_least_one_param_assigned(self):
        return len(self.get_assigned_params()) > 0

    def _get_setting(self, setting_name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, setting_name, return_type)

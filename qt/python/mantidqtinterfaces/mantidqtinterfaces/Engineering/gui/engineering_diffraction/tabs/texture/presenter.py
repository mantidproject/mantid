# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserverWithArgPassing
from mantid.kernel import logger
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    INSTRUMENT_DICT,
    CalibrationObserver,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.show_sample.show_sample_presenter import ShowSamplePresenter


def redraws_table(func):
    def wrapper(self):
        func(self)
        self.redraw_table()
        return

    return wrapper


class TexturePresenter:
    def __init__(self, model, view):
        # set up mvp components
        self.model = model
        self.view = view
        self.worker = None
        self.show_sample_presenter = ShowSamplePresenter(model, view, False)

        # some internal data structures
        self.ws_names = []
        self.ws_assignments = {}
        self.param_assignments = {}
        self.unassigned_params = []
        self.unassigned_wss = []
        self.ws_info = {}

        # set up some observers
        self.correction_notifier = GenericObservable()
        self.calibration_observer = CalibrationObserver(self)
        # Observers
        self.focus_run_observer = GenericObserverWithArgPassing(self.set_default_files)

        # set some metadata
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        # connect view slots
        # loader slots
        self.view.set_on_load_ws_clicked(self.load_ws_files)
        self.view.set_on_load_param_clicked(self.load_param_files)
        # table slots
        self.view.set_on_select_all_clicked(self.select_all)
        self.view.set_on_deselect_all_clicked(self.deselect_all)
        self.view.set_on_delete_clicked(self.delete_selected_files)
        self.view.set_on_delete_param_clicked(self.delete_selected_param_files)
        # xtal slots
        self.view.set_on_check_inc_scatt_corr_state_changed(self.view.update_crystal_section_visibility)
        self.view.set_include_scatter_corr(False)
        self.view.set_crystal_section_visibility(False)
        self.view.set_on_set_crystal_clicked(self.on_set_crystal_clicked)
        self.view.set_on_set_all_crystal_clicked(self.on_set_all_crystal_clicked)
        # pf slots
        self.view.set_on_calc_pf_clicked(self.on_calc_pf_clicked)
        # enable/disable options
        self.view.on_lattice_changed(self.set_crystal_inputs_enabled)
        self.view.on_spacegroup_changed(self.set_crystal_inputs_enabled)
        self.view.on_basis_changed(self.set_crystal_inputs_enabled)
        self.view.on_cif_changed(self.set_crystal_inputs_enabled)
        # handle changes to UI when updating selected workspace
        self.view.set_on_selection_state_changed(self.on_selection_change)

        # ensure initial state is correct
        self.set_crystal_inputs_enabled()
        self.update_readout_column_list()
        self.set_default_directories()

    # ------- File Loaders ------------------

    @redraws_table
    def load_ws_files(self):
        filenames = self.view.finder_texture_ws.getFilenames()
        ws_names = self.model.load_files(filenames)
        for ws_name in ws_names:
            if ws_name not in self.ws_names:
                self.ws_names.append(ws_name)
            if not self.ws_has_param(ws_name):
                self.unassigned_wss.append(ws_name)

    @redraws_table
    def load_param_files(self):
        filenames = self.view.finder_texture_tables.getFilenames()
        params = self.model.load_files(filenames)
        for param in params:
            if not self.param_has_ws(param):
                self.unassigned_params.append(param)

    @redraws_table
    def delete_selected_files(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        for ws in selected_wss:
            # remove from ws_names
            self.ws_names.pop(self.ws_names.index(ws))
            if self.ws_has_param(ws):
                # remove assignment between ws and param
                param = self.ws_assignments.pop(ws)
                self.param_assignments.pop(param)
                # do not add either back to the unassigned piles
            else:
                self.unassigned_wss.pop(self.unassigned_wss.index(ws))

    def set_default_files(self, filepaths):
        directory = self.model.get_last_directory(filepaths)
        self.view.set_default_files(filepaths, directory)

    # ---- Internal data structure logic for tracking parameter table assignments -------

    def ws_has_param(self, ws):
        return ws in self.ws_assignments.keys()

    def param_has_ws(self, param):
        return param in self.param_assignments.keys()

    def get_assigned_params(self):
        return list(self.param_assignments.keys())

    def unassign_params(self, params):
        for param in params:
            if self.param_has_ws(param):
                # remove assignment between ws and param
                assigned_ws = self.param_assignments.pop(param)
                self.ws_assignments.pop(assigned_ws)
                # add ws to the unassigned pile
                self.unassigned_wss.append(assigned_ws)

    def assign_unpaired_wss_and_params(self):
        (self.unassigned_wss, self.unassigned_params, self.ws_assignments, self.param_assignments) = (
            self.model.assign_unpaired_wss_and_params(
                self.unassigned_wss, self.unassigned_params, self.ws_assignments, self.param_assignments
            )
        )

    def all_wss_have_params(self):
        return self.model.all_wss_have_params(*self.view.get_selected_workspaces())

    def at_least_one_param_assigned(self):
        return self.model.at_least_one_param_assigned(self.get_assigned_params())

    def _has_selected_wss(self):
        selected_wss, _ = self.view.get_selected_workspaces()
        return self.model.has_selected_wss(selected_wss)

    # ----- Table logic -----------------

    @redraws_table
    def delete_selected_param_files(self):
        selected_wss, selected_params = self.view.get_selected_workspaces()
        self.unassign_params(selected_params)

    def select_all(self):
        self.view.set_all_workspaces_selected(True)
        self.set_crystal_inputs_enabled()

    def deselect_all(self):
        self.view.set_all_workspaces_selected(False)
        self.set_crystal_inputs_enabled()

    # --------- Xtal Logic -------------------------------

    @redraws_table
    def on_set_crystal_clicked(self):
        self.model.set_ws_xtal(
            self.view.get_crystal_ws_prop(), self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis(), self.view.get_cif()
        )

    @redraws_table
    def on_set_all_crystal_clicked(self):
        wss, _ = self.view.get_selected_workspaces()
        self.model.set_all_ws_xtal(wss, self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis(), self.view.get_cif())

    # --------- Pole Figure Logic --------------------------

    def on_calc_pf_clicked(self):
        wss, params = self.view.get_selected_workspaces()
        # require at least one wss to be selected
        if len(wss) == 0:
            return
        # remove any 'not set' parameters workspaces from the list
        params = [p for p in params if p != "Not set"]

        # set all the parameters from the view onto the model
        self.model.set_projection_method(self.view.get_projection_method())
        self.model.set_inc_scatt(self.view.get_inc_scatt_power())
        self.model.set_hkl(self.view.get_hkl())
        self.model.set_readout_col(self.view.get_readout_column())
        self.model.set_out_ws_and_grouping(wss, params)
        ax_transform, ax_labels = self._get_ax_data()
        self.model.set_ax_trans(ax_transform)
        self.model.set_ax_labels(ax_labels)
        self.model.set_plot_exp(self._get_setting("plot_exp_pf", bool))
        self.model.set_contour_kernel(float(self._get_setting("contour_kernel", str)))

        # default scattering pos for now
        self.model.set_scat_vol_pos((0.0, 0.0, 0.0))

        # get the threshold values from the view
        has_chi2_col, has_x0_col = False, False
        if self.all_wss_have_params() and self.at_least_one_param_assigned():
            has_chi2_col, has_x0_col = self.model.check_param_ws_for_columns(params)
        chi2_thresh = self._get_setting("cost_func_thresh") if has_chi2_col else 0.0
        peak_thresh = self._get_setting("peak_pos_thresh") if has_x0_col else 0.0

        self.model.set_chi2_thresh(chi2_thresh)
        self.model.set_peak_thresh(peak_thresh)

        self.set_worker(
            AsyncTask(
                self.calc_pf,
                (
                    wss,
                    params,
                ),
                error_cb=self._on_worker_error,
                finished_cb=self._on_worker_success,
            )
        )
        self.worker = self.get_worker()
        self.worker.start()

    def _get_ax_data(self):
        return output_settings.get_texture_axes_transform()

    def set_worker(self, worker):
        self.worker = worker

    def get_worker(self):
        return self.worker

    def calc_pf(
        self,
        wss,
        params,
    ):
        root_dir = output_settings.get_output_path()
        save_dirs = self.model.get_save_dirs(root_dir, "PoleFigureTables", self.model.get_rb_num(), self.model.get_grouping())
        self.model.exec_make_pf_tables(wss, params, save_dirs)
        self.plot_pf(save_dirs)

    def _on_worker_success(self):
        self.correction_notifier.notify_subscribers("Pole Figure Created")

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))

    def plot_pf(self, save_dirs):
        # Get figure and canvas from view
        fig, canvas = self.view.get_plot_axis()

        # Clear existing figure
        fig.clf()

        self.model.exec_plot_pf(fig, save_dirs)

        canvas.draw()

    # --------- Metadata handling --------------------

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

    def set_default_directories(self):
        save_dir = output_settings.get_output_path()
        self.view.finder_texture_ws.setLastDirectory(save_dir)
        self.view.finder_texture_tables.setLastDirectory(save_dir)

    def _get_setting(self, setting_name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, setting_name, return_type)

    # ------- UI state tracking ----------------------

    def on_selection_change(self):
        self.update_readout_column_list()
        self.set_crystal_inputs_enabled()

    def redraw_table(self):
        self.assign_unpaired_wss_and_params()
        self.update_ws_info()
        self.view.populate_workspace_table(self.ws_info)
        self.view.populate_workspace_list(self.ws_names)
        self.update_readout_column_list()
        self.set_crystal_inputs_enabled()

    def update_ws_info(self):
        ws_info = {}
        selected_ws, _ = self.view.get_selected_workspaces()
        for pos_ind, ws_name in enumerate(self.ws_names):
            param = self.model.get_param_from_ws(ws_name, self.ws_assignments)
            ws_info[ws_name] = self.model.get_ws_info(ws_name, param, ws_name in selected_ws)  # maintain state of selected boxes
        self.ws_info = ws_info

    def update_readout_column_list(self):
        params = self.get_assigned_params()
        show_col = False
        # only if all wss have param files plotting with a column readout should be an option
        if self.all_wss_have_params() and self.at_least_one_param_assigned():
            col_list, starting_index = self.model.read_param_cols(params[0])
            self.view.populate_readout_column_list(col_list, starting_index)
            # only have the option to pick a column if columns are present
            show_col = self.model.has_at_least_one_col(col_list)
        self.view.update_col_select_visibility(show_col)

    def set_crystal_inputs_enabled(self):
        # inputs:
        has_cif = self.model.has_cif(self.view.get_cif())
        has_any_latt = self.model.has_any_latt(self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis())

        # if any input in the lattice parameters field, disable the cif file search
        self.view.finder_cif_file.setEnabled(not has_any_latt)

        # if there is a cif file, disable the lattice inputs
        self.view.lattice_lineedit.setEnabled(not has_cif)
        self.view.spacegroup_lineedit.setEnabled(not has_cif)
        self.view.basis_lineedit.setEnabled(not has_cif)

        # buttons:
        # to set crystal need either a cif OR lattice params AND a ws to apply it to
        enabled = self.model.has_xtal_and_ws(
            self.view.get_lattice(), self.view.get_spacegroup(), self.view.get_basis(), self.view.get_cif(), self.view.get_crystal_ws_prop()
        )
        self.view.btn_setCrystal.setEnabled(enabled)
        self.view.btn_setAllCrystal.setEnabled(self.model.can_set_all_crystal(enabled, self._has_selected_wss()))

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

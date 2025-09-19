# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    create_error_message,
    wsname_in_instr_run_ceria_group_ispec_unit_format,
)
from mantid.simpleapi import logger
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserverWithArgPassing
from mantid.api import AnalysisDataService as ADS


class FittingDataPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.iplot = []
        self.row_numbers = TwoWayRowDict()  # {ws_name: table_row} and {table_row: ws_name}
        self.plotted = set()  # List of plotted workspace names

        # Connect view signals to local methods
        self.view.set_on_load_clicked(self.on_load_clicked)
        self.view.set_enable_load_button_connection(self._enable_load_button)
        self.view.set_enable_inspect_bg_button_connection(self._enable_inspect_bg_button)
        self.view.set_on_remove_selected_clicked(self._remove_selected_tracked_workspaces)
        self.view.set_on_remove_all_clicked(self._remove_all_tracked_workspaces)
        self.view.set_on_plotBG_clicked(self._plotBG)
        self.view.set_on_table_cell_changed(self._handle_table_cell_changed)
        self.view.set_on_region_changed(self._update_file_filter)
        self.view.set_on_xunit_changed(self._update_file_filter)
        self.view.set_table_selection_changed(self._handle_selection_changed)

        # Observable Setup
        self.plot_added_notifier = GenericObservable()
        self.plot_removed_notifier = GenericObservable()
        self.all_plots_removed_notifier = GenericObservable()
        # Observers
        self.focus_run_observer = GenericObserverWithArgPassing(self.set_default_files_tof)
        self.focus_combined_observer = GenericObserverWithArgPassing(self.set_default_files_texture)

    def set_default_files_tof(self, filepaths):
        if not self.model.texture_auto_populate():
            self._set_default_files(filepaths)

    def set_default_files_texture(self, filepaths):
        if self.model.texture_auto_populate():
            index = self.view.combo_xunit.findText("dSpacing")
            self.view.combo_xunit.setCurrentIndex(index)
            self._set_default_files(filepaths)

    def _set_default_files(self, filepaths):
        directory = self.model.get_last_directory(filepaths)
        self.view.set_default_files(filepaths, directory)

    def get_sorted_active_ws_list(self):
        return self.model.get_active_ws_sorted_by_primary_log()

    def get_active_ws_list(self):
        return self.model.get_active_ws_name_list()

    def get_loaded_ws_list(self):
        return self.model.get_loaded_ws_list()

    def get_log_ws_group_name(self):
        return self.model.get_log_workspace_group_name()

    def _update_file_filter(self, region, xunit):
        self.view.update_file_filter(region, xunit)

    def on_load_clicked(self):
        if self._validate():
            filenames = self._get_filenames()
            self._start_load_worker(filenames)

    def remove_workspace(self, ws_name):
        self.plotted.discard(ws_name)
        if ws_name in self.model.get_all_workspace_names():
            if ws_name in self.model.get_loaded_workspaces():
                self.row_numbers.pop(ws_name)
            self.model.remove_workspace(ws_name)
            # plot_presenter will already be notified of ws being removed via its own ADS observer
            self._repopulate_table(False)
        elif ws_name in self.model.get_all_log_workspaces_names():
            self.model.update_sample_log_workspace_group()

    def rename_workspace(self, old_name, new_name):
        # Note - ws.name() not updated yet so need to rely on new_name parameter
        # Also Note - ADS rename is always associated with a ADS replace so
        # rely on the replace to _repopulate_table. Prefer not to call twice
        # to avoid issue with legend entries doubling up
        if old_name in self.model.get_all_workspace_names():
            self.model.update_workspace_name(old_name, new_name)
            if old_name in self.plotted:
                self.plotted.remove(old_name)
                self.plotted.add(new_name)
            if old_name in self.row_numbers:  # bgsub not in row_numbers
                row_no = self.row_numbers.pop(old_name)
                self.row_numbers[new_name] = row_no

    # handle ADS clear
    def clear_workspaces(self):
        self.model.clear_workspaces()
        self.plotted.clear()
        self.row_numbers.clear()
        self._repopulate_table()

    def replace_workspace(self, name, workspace):
        if name in self.model.get_all_workspace_names():
            self.model.replace_workspace(name, workspace)
            self._repopulate_table()

    def restore_table(self, clear_plotted=True):  # used when the interface is being restored from a save or crash
        self._repopulate_table(clear_plotted)

    def _start_load_worker(self, filenames):
        """
        Load one to many files into mantid that are tracked by the interface.
        :param filenames: Comma separated list of filenames to load
        """
        self.worker = AsyncTask(
            self.model.load_files,
            (filenames,),
            error_cb=self._on_worker_error,
            finished_cb=self._emit_enable_load_button_signal,
            success_cb=self._on_worker_success,
        )
        self.worker.start()

    def _on_worker_error(self, _):
        logger.error("Error occurred when loading files.")
        self._emit_enable_load_button_signal()

    def _on_worker_success(self, _):
        wsnames = self.model.get_last_added()
        if self.view.get_add_to_plot():
            self.plotted.update(wsnames)

        self._repopulate_table()

        # subtract background - has to be done post repopulation, can't change default in _add_row_to_table
        [self.view.set_item_checkstate(self.row_numbers[wsname], 3, True) for wsname in wsnames]

    def _repopulate_table(self, clear_plotted=True):
        """
        Populate the table with the information from the loaded workspaces.
        Will also handle any workspaces that need to be plotted.
        """
        workspaces_to_be_plotted = self.plotted.copy()
        if clear_plotted:
            self.plotted.clear()
        self._remove_all_table_rows()
        self.row_numbers.clear()
        self.all_plots_removed_notifier.notify_subscribers()
        workspaces = self.model.get_loaded_workspaces()
        for i, name in enumerate(workspaces):
            try:
                run_no = self.model.get_sample_log_from_ws(name, "run_number")
                bank = self.model.get_sample_log_from_ws(name, "bankid")
                if bank == 0:
                    bank = "cropped"
                active_ws_name = self.model.get_active_ws_name(name)
                plotted = active_ws_name in workspaces_to_be_plotted
                self._add_row_to_table(name, i, run_no, bank, plotted, *self.model.get_bg_params()[name])
            except RuntimeError:
                self._add_row_to_table(name, i)
            # update row_numbers at end so _handle_table_cell_changed only acts once row is ready
            self.row_numbers[name] = i

    def _remove_selected_tracked_workspaces(self):
        row_numbers = self._remove_selected_table_rows()
        for row_no in row_numbers:
            ws_name = self.row_numbers.pop(row_no)
            removed_ws_list = self.model.delete_workspace(ws_name)
            for ws in removed_ws_list:
                # plot_removed_notifier will be done in _repopulate_table
                self.plotted.discard(ws.name())
        self._repopulate_table()

    def _remove_all_tracked_workspaces(self):
        self.model.delete_workspaces()
        self.all_plots_removed_notifier.notify_subscribers()
        self.plotted.clear()
        self.row_numbers.clear()
        self._remove_all_table_rows()

    def _plotBG(self):
        # make external figure
        row_numbers = self.view.get_selected_rows()
        for row in row_numbers:
            ws_name = self.row_numbers[row]
            self.model.plot_background_figure(ws_name)

    def _handle_table_cell_changed(self, row, col):
        if row in self.row_numbers:
            # this is written on assumption that all columns are present.
            # events fired by view.add_table_row are queued so handler called after add_table_row completes
            loaded_ws_name = self.row_numbers[row]
            is_plotted = self.view.get_item_checked(row, 2)
            is_sub = self.view.get_item_checked(row, 3)
            if col == 2:
                # Plot check box
                ws = self.model.get_active_ws(loaded_ws_name)
                ws_name = self.model.get_active_ws_name(loaded_ws_name)
                if is_plotted:
                    if len(self.plotted) > 0:
                        if ADS.retrieve(next(iter(self.plotted))).getXDimension().name == ws.getXDimension().name:
                            self.plot_added_notifier.notify_subscribers(ws)
                            self.plotted.add(ws_name)
                        else:
                            self.view.set_item_checkstate(row, col, False)
                    else:
                        self.plot_added_notifier.notify_subscribers(ws)
                        self.plotted.add(ws_name)
                else:
                    self.plot_removed_notifier.notify_subscribers(ws)
                    self.plotted.discard(ws_name)
            elif col == 3:
                # subtract bg col
                self.model.update_bgsub_status(loaded_ws_name, is_sub)
                if is_sub:
                    bg_params = self.view.read_bg_params_from_table(row)
                    if not self.model.create_or_update_bgsub_ws(loaded_ws_name, bg_params):
                        self._revert_bg_sub_table_values(loaded_ws_name, row)
                if is_plotted:
                    self._update_plotted_ws_with_sub_state(loaded_ws_name, is_sub)
            elif col > 3:
                if is_sub:
                    # bg params changed - revaluate background
                    bg_params = self.view.read_bg_params_from_table(row)
                    if not self.model.create_or_update_bgsub_ws(loaded_ws_name, bg_params):
                        self._revert_bg_sub_table_values(loaded_ws_name, row)

    def _revert_bg_sub_table_values(self, loaded_ws_name, row):
        original_bg_params = self.model.get_bg_params()[loaded_ws_name]
        self.view.set_table_column(row, 4, original_bg_params[1])
        self.view.set_table_column(row, 5, original_bg_params[2])

    def _update_plotted_ws_with_sub_state(self, ws_name, is_sub):
        ws = self.model.get_loaded_workspaces()[ws_name]
        ws_bgsub = self.model.get_bgsub_workspaces()[ws_name]
        ws_bgsub_name = self.model.get_bgsub_workspace_names()[ws_name]
        if ws_name in self.plotted and is_sub:
            self.plot_removed_notifier.notify_subscribers(ws)
            self.plotted.discard(ws_name)
            self.plot_added_notifier.notify_subscribers(ws_bgsub)
            self.plotted.add(ws_bgsub_name)
        elif ws_bgsub_name in self.plotted and not is_sub:
            self.plot_removed_notifier.notify_subscribers(ws_bgsub)
            self.plotted.discard(ws_bgsub_name)
            self.plot_added_notifier.notify_subscribers(ws)
            self.plotted.add(ws_name)

    def _handle_selection_changed(self):
        enable = True
        if not self.view.get_selected_rows():
            enable = False
        self._enable_inspect_bg_button(enable)

    def _enable_load_button(self, enabled):
        self.view.set_load_button_enabled(enabled)

    def _emit_enable_load_button_signal(self):
        self.view.sig_enable_load_button.emit(True)

    def _enable_inspect_bg_button(self, enabled):
        self.view.set_inspect_bg_button_enabled(enabled)

    def _get_filenames(self):
        return self.view.get_filenames_to_load()

    def _is_searching(self):
        return self.view.is_searching()

    def _files_are_valid(self):
        return self.view.get_filenames_valid()

    def _validate(self):
        if self._is_searching():
            create_error_message(self.view, "Mantid is searching for files. Please wait.")
            return False
        elif not self._files_are_valid():
            create_error_message(self.view, "Entered files are not valid.")
            return False
        return True

    def _add_row_to_table(self, ws_name, row, run_no=None, bank=None, plotted=False, bgsub=False, niter=50, xwindow=None, SG=True):
        words = ws_name.split("_")
        # find xwindow from ws xunit if not specified
        if not xwindow:
            ws = self.model.get_loaded_workspaces()[ws_name]
            if ws.getAxis(0).getUnit().unitID() == "TOF":
                xwindow = 600
            else:
                xwindow = 0.02
        if run_no is not None and bank is not None:
            self.view.add_table_row(run_no, bank, plotted, bgsub, niter, xwindow, SG)
        elif len(words) == 4 and words[2] == "bank":
            # this seems to now be obsolete - common file name format is now:
            # INSTR_RUNNUM_CERIANUM_GROUP_ispec_UNIT?
            # will maintain this path for legacy data?
            logger.notice("No sample logs present, determining information from workspace name.")
            self.view.add_table_row(words[1], words[3], plotted, bgsub, niter, xwindow, SG)
        elif wsname_in_instr_run_ceria_group_ispec_unit_format(ws_name):
            logger.notice("No sample logs present, determining information from workspace name.")
            self.view.add_table_row(words[1], f"{words[3]} {words[4]}", plotted, bgsub, niter, xwindow, SG)
        else:
            logger.warning(
                "The workspace '{}' was not in the correct naming format. Files should be named in either of these ways: "
                "INSTRUMENT_RUNNUMBER_bank_BANK or INSTR_CERIANUM_RUNNUM_GROUP_ispec_UNIT."
                "Using workspace name as identifier.".format(ws_name)
            )
            self.view.add_table_row(ws_name, "N/A", plotted, bgsub, niter, xwindow, SG)

    def _remove_table_row(self, row_no):
        self.view.remove_table_row(row_no)

    def _remove_selected_table_rows(self):
        return self.view.remove_selected()

    def _remove_all_table_rows(self):
        self.view.remove_all()


class TwoWayRowDict(dict):
    """
    Two way dictionary used to map rows to workspaces and vice versa.
    """

    def pop(self, key):
        value = self[key]
        self.__delitem__(key)
        return value

    def __setitem__(self, key, value):
        if key in self:
            del self[key]
        if value in self:
            del self[value]
        dict.__setitem__(self, key, value)
        dict.__setitem__(self, value, key)

    def __delitem__(self, key):
        dict.__delitem__(self, self[key])
        dict.__delitem__(self, key)

    def __bool__(self):
        return bool(self.keys())

    def __len__(self):
        return dict.__len__(self) / 2

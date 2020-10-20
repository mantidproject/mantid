# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Engineering.gui.engineering_diffraction.tabs.common import create_error_message
from mantid.simpleapi import logger
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable


class FittingDataPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None

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
        self.view.set_on_xunit_changed(self._log_xunit_change)
        self.view.set_table_selection_changed(self._handle_selection_changed)

        # Observable Setup
        self.plot_added_notifier = GenericObservable()
        self.plot_removed_notifier = GenericObservable()
        self.all_plots_removed_notifier = GenericObservable()

    def _log_xunit_change(self, xunit):
        logger.notice("Subsequent files will be loaded with the x-axis unit:\t{}".format(xunit))

    def on_load_clicked(self, xunit):
        if self._validate():
            filenames = self._get_filenames()
            self._start_load_worker(filenames, xunit)

    def remove_workspace(self, ws_name):
        if ws_name in self.get_loaded_workspaces():
            removed = self.get_loaded_workspaces().pop(ws_name)
            self.plot_removed_notifier.notify_subscribers(removed)
            self.plotted.discard(ws_name)
            self.model.remove_log_rows([self.row_numbers[ws_name]])
            self.model.update_log_workspace_group()
            self._repopulate_table()
        elif ws_name in self.model.get_log_workspaces_name():
            self.model.update_log_workspace_group()

    def rename_workspace(self, old_name, new_name):
        if old_name in self.get_loaded_workspaces():
            self.model.update_workspace_name(old_name, new_name)
            if old_name in self.plotted:
                self.plotted.remove(old_name)
                self.plotted.add(new_name)
            self._repopulate_table()
            self.model.update_log_workspace_group()  # so matches new table

    def clear_workspaces(self):
        self.get_loaded_workspaces().clear()
        self.plotted.clear()
        self.row_numbers.clear()
        self._repopulate_table()

    def replace_workspace(self, name, workspace):
        if name in self.get_loaded_workspaces():
            self.get_loaded_workspaces()[name] = workspace
            if name in self.plotted:
                self.all_plots_removed_notifier.notify_subscribers()
            self._repopulate_table()

    def get_loaded_workspaces(self):
        return self.model.get_loaded_workspaces()

    def _start_load_worker(self, filenames, xunit):
        """
        Load one to many files into mantid that are tracked by the interface.
        :param filenames: Comma separated list of filenames to load
        """
        self.worker = AsyncTask(self.model.load_files, (filenames, xunit),
                                error_cb=self._on_worker_error,
                                finished_cb=self._emit_enable_load_button_signal,
                                success_cb=self._on_worker_success)
        self.worker.start()

    def _on_worker_error(self, _):
        logger.error("Error occurred when loading files.")
        self._emit_enable_load_button_signal()

    def _on_worker_success(self, _):
        if self.view.get_add_to_plot():
            self.plotted.update(self.model.get_last_added())
        self._repopulate_table()

    def _repopulate_table(self):
        """
        Populate the table with the information from the loaded workspaces.
        Will also handle any workspaces that need to be plotted.
        """
        self._remove_all_table_rows()
        self.row_numbers.clear()
        self.all_plots_removed_notifier.notify_subscribers()
        workspaces = self.get_loaded_workspaces()
        for i, name in enumerate(workspaces):
            try:
                run_no = self.model.get_sample_log_from_ws(name, "run_number")
                bank = self.model.get_sample_log_from_ws(name, "bankid")
                if bank == 0:
                    bank = "cropped"
                checked = name in self.plotted
                if name in self.model.get_bg_params():
                    self._add_row_to_table(name, i, run_no, bank, checked, *self.model.get_bg_params()[name])
                else:
                    self._add_row_to_table(name, i, run_no, bank, checked)
            except RuntimeError:
                self._add_row_to_table(name, i)
            self._handle_table_cell_changed(i, 2)

    def _remove_selected_tracked_workspaces(self):
        row_numbers = self._remove_selected_table_rows()
        self.model.remove_log_rows(row_numbers)
        for row_no in row_numbers:
            ws_name = self.row_numbers.pop(row_no)
            removed = self.get_loaded_workspaces().pop(ws_name)
            self.plot_removed_notifier.notify_subscribers(removed)
            self.plotted.discard(ws_name)
        self._repopulate_table()

    def _remove_all_tracked_workspaces(self):
        self.clear_workspaces()
        self.model.clear_logs()
        self._remove_all_table_rows()

    def _plotBG(self):
        # make external figure
        row_numbers = self.view.get_selected_rows()
        for row in row_numbers:
            if self.view.get_item_checked(row, 3):
                # background has been subtracted from workspace
                ws_name = self.row_numbers[row]
                self.model.plot_background_figure(ws_name)

    def _handle_table_cell_changed(self, row, col):
        if row in self.row_numbers:
            ws_name = self.row_numbers[row]
            if col == 2:
                # Plot check box
                ws = self.model.get_loaded_workspaces()[ws_name]
                if self.view.get_item_checked(row, col):  # Plot Box is checked
                    self.plot_added_notifier.notify_subscribers(ws)
                    self.plotted.add(ws_name)
                else:  # Plot box is unchecked
                    self.plot_removed_notifier.notify_subscribers(ws)
                    self.plotted.discard(ws_name)
            elif col == 3:
                # subtract bg
                if self.view.get_item_checked(row, col):
                    # subtract bg box checked
                    bg_params = self.view.read_bg_params_from_table(row)
                    self.model.do_background_subtraction(ws_name, bg_params)
                elif self.model.get_background_workspaces()[ws_name]:
                    # box unchecked and bg exists:
                    self.model.undo_background_subtraction(ws_name)
            elif col > 3:
                if self.view.get_item_checked(row, 3):
                    # bg params changed - revaluate background
                    bg_params = self.view.read_bg_params_from_table(row)
                    self.model.do_background_subtraction(ws_name, bg_params)

    def _handle_selection_changed(self):
        rows = self.view.get_selected_rows()
        enabled = False
        for row in rows:
            if self.view.get_item_checked(row, 3):
                enabled = True
        self._enable_inspect_bg_button(enabled)

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

    def _add_row_to_table(self, ws_name, row, run_no=None, bank=None, checked=False, bgsub=False, niter=100,
                          xwindow=None, SG=True):
        words = ws_name.split("_")
        # find xwindow from ws xunit if not specified
        if not xwindow:
            ws = self.model.get_loaded_workspaces()[ws_name]
            if ws.getAxis(0).getUnit().unitID() == "TOF":
                xwindow = 1000
            else:
                xwindow = 0.05
        if run_no is not None and bank is not None:
            self.view.add_table_row(run_no, bank, checked, bgsub, niter, xwindow, SG)
            self.row_numbers[ws_name] = row
        elif len(words) == 4 and words[2] == "bank":
            logger.notice("No sample logs present, determining information from workspace name.")
            self.view.add_table_row(words[1], words[3], checked, bgsub, niter, xwindow, SG)
            self.row_numbers[ws_name] = row
        else:
            logger.warning(
                "The workspace '{}' was not in the correct naming format. Files should be named in the following way: "
                "INSTRUMENT_RUNNUMBER_bank_BANK. Using workspace name as identifier.".format(ws_name)
            )
            self.view.add_table_row(ws_name, "N/A", checked, bgsub, niter, xwindow, SG)
            self.row_numbers[ws_name] = row

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

    def pop(self, key):
        value = self[key]
        self.__delitem__(key)
        return value

    def __len__(self):
        return dict.__len__(self) / 2

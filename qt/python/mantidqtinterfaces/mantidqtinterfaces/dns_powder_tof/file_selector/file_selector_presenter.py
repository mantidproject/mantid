# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS file selector tab presenter of DNS Reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver


class DNSFileSelectorPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None, watcher=None):
        # pylint: disable=too-many-arguments
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.watcher = watcher
        self.view = view
        self._old_data_set = set()
        self._standard_data_counter = 0
        self._sample_data_path = ""

        # connect signals
        self._attach_signal_slots()

    # loading
    def _read_all(self, filtered=False, watcher=False, start=None, end=None):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified by start and end.
        """
        if self.param_dict:
            data_path = self.param_dict["paths"]["data_dir"]
            if "polarisation_table" in self.param_dict["paths"]:
                pol_table = self.param_dict["paths"]["polarisation_table"]
            else:
                pol_table = []
            file_number_range = [start, end]
            number_of_files, loaded, datafiles, file_number_range_filtered = self.model.set_datafiles_to_load(
                data_path, file_number_range, filtered, watcher
            )
            self.view.open_progress_dialog(number_of_files)
            self.model.read_all(datafiles, data_path, loaded, pol_table, watcher)

    def _read_standard(self, self_call=False):
        """
        Reading of standard files.
        """
        data_path = self.param_dict["paths"]["data_dir"]
        standard_path = self.param_dict["paths"]["standards_dir"]
        if standard_path:
            if "polarisation_table" in self.param_dict["paths"]:
                pol_table = self.param_dict["paths"]["polarisation_table"]
            else:
                pol_table = []
            standard_found = self.model.read_standard(standard_path, pol_table)
            self._filter_standard()
            if not standard_found and not self_call:
                if self.model.try_unzip(data_path, standard_path):
                    self._read_standard(self_call=True)

    def _cancel_loading(self):
        self.model.set_loading_canceled(True)

    def _set_start_end(self, file_number_range):
        start, end = file_number_range
        self.view.set_start_end_file_numbers_from_arguments(start, end)

    def _files_changed_by_watcher(self):
        """
        Triggered by view if new files are found and timer of 5 s is run
        down.
        """
        self._read_all(watcher=True)

    def _autoload_new(self, state):
        data_dir = self.param_dict["paths"]["data_dir"]

        if not data_dir:
            self.raise_error("No data directory selected", critical=True)

        if state == 2 and data_dir:
            self.watcher.start_watcher(data_dir)
            if not self._old_data_set:
                self._read_all()
        else:
            self.watcher.stop_watcher()
        # re-adjust view to column width
        self._format_view()

    def _auto_select_standard(self, state):
        if state == 2:
            self._check_all_visible_scans()

    def _check_all_visible_scans(self):
        self.model.check_scans_by_rows(self._get_non_hidden_rows())

    def _check_last_scans(self, sender_name):
        number_of_scans_to_check = self.view.get_number_scans_to_check()
        complete = "complete" in sender_name
        not_hidden_rows = self._get_non_hidden_rows()
        self.model.check_last_scans(number_of_scans_to_check, complete, not_hidden_rows)

    def _check_selected_scans(self):
        indexes = self.view.get_selected_indexes()
        self.model.check_scans_by_indexes(indexes)

    def _uncheck_all_scans(self):
        self.model.uncheck_all_scans()

    def _show_all_scans(self):
        scan_range = self.model.get_scan_range()
        for row in scan_range:
            self.view.show_scan(row)

    def _get_non_hidden_rows(self):
        hidden = self.view.is_scan_hidden
        scan_range = self.model.get_scan_range()
        non_hidden_rows = [row for row in scan_range if not hidden(row)]
        return non_hidden_rows

    def _filter_scans(self):
        """
        Hide and uncheck the scans in the treeview which do not
        match filters from get_filters.
        """
        self._show_all_scans()
        filters = self.view.get_filters().items()
        hide_scans = self.model.filter_scans_for_boxes(filters, self._is_modus_tof())
        for row in hide_scans:
            self.view.hide_scan(row)

    def _filter_standard(self):
        """
        Hide and uncheck the standard files which do not match filters.
        """
        self._show_all_scans()
        filters = self.view.get_standard_filters()
        active = filters["vanadium"] or filters["empty"] or filters["nicr"]
        hide_scans = self.model.filter_standard_types(filters, active, self._is_modus_tof())
        for row in hide_scans:
            self.view.hide_scan(row)

    # change of datasets
    def _changed_to_standard(self):
        if not self.model.get_number_of_scans():
            self._read_standard()
        self._filter_standard()

    def _changed_to_sample(self):
        if not self.model.get_number_of_scans():
            self._read_all()
        self._filter_scans()

    # change of modi
    def _is_modus_tof(self):
        return "_tof" in self.modus

    def _modus_changed(self):
        self._filter_scans()
        self._filter_standard()
        self.view.hide_tof(self._is_modus_tof())

    # model can access this function
    # have no idea how to do this otherwise
    def update_progress(self, iteration, iteration_max):
        self.view.set_progress(iteration, iteration_max)

    def _right_click(self, index):
        """
        Open files in the treeview with right click.
        """
        data_path = self.param_dict["paths"]["data_dir"]
        standard_path = self.param_dict["paths"]["standards_dir"]
        self.model.open_datafile(index, data_path, standard_path)

    # normal observer function
    def get_option_dict(self):
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        self.own_dict["full_data"] = self.model.get_data()
        self.own_dict["standard_data_tree_model"] = self.model.get_data(standard=True)
        self.own_dict["selected_file_numbers"] = self.model.get_data(full_info=False)
        return self.own_dict

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options["auto_select_standard"] and not own_options["standard_data_tree_model"]:
            self._auto_select_standard(state=2)

    def set_view_from_param(self):
        """
        Setting of the fields defined in mapping from the parameter dictionary
        and checks scans checked in the dict.
        """
        if self.param_dict:
            file_numbers = self.param_dict[self.name].pop("selected_file_numbers", [])
            self.view.set_state(self.param_dict.get(self.name, {}))
            not_found = self.model.check_by_file_numbers(file_numbers)
            if not_found:
                print(f"Of {len(file_numbers)} loaded checked " f"file numbers {not_found} were not found " "in list of datafiles")

    def _clear_data_trees(self):
        self.model.sample_data_tree_model.clear_scans()
        self.model.standard_data_tree_model.clear_scans()

    def tab_got_focus(self):
        sample_data_dir = self.param_dict["paths"]["data_dir"]
        if self._sample_data_path != sample_data_dir:
            self._clear_data_trees()
            self._standard_data_counter = 0
            self._sample_data_path = sample_data_dir

        standard_path = self.param_dict["paths"]["standards_dir"]
        # The first time that the standard data path is provided
        # and the user clicks on the file selector tab, then the
        # file selector presenter's dictionary needs to be filled
        # with standard data info under the 'standard_data_tree_model'
        # key (default setting). To implement this, standard data
        # click is realized. After that, a sample data view is
        # provided to the user.
        if standard_path and self._standard_data_counter == 0:
            self._standard_data_clicked()
            self._sample_data_clicked()
            self._standard_data_counter += 1
        self.view.hide_tof(hidden="_tof" not in self.modus)

    def process_commandline_request(self, command_dict):
        start = int(command_dict["files"][0]["start"])
        end = int(command_dict["files"][0]["end"])
        self._read_all(filtered=True, start=start, end=end)
        self.model.check_file_number_range(start, end)

    def _expanded(self):
        self.num_columns = self.model.get_active_model_column_count()
        self.view.adjust_treeview_columns_width(self.num_columns)

    def _format_view(self):
        self.num_columns = self.model.get_active_model_column_count()
        self.view.set_first_column_spanned(self.model.get_scan_range())
        # expand all only in the case when the total number of files
        # to display is less than 151 (more files to expand takes
        # some time to execute)
        file_count = self.model.get_number_of_files_in_treeview()
        if file_count <= 150:
            self.view.expand_all()
        self.view.adjust_treeview_columns_width(self.num_columns)

    def _sample_data_clicked(self):
        self.view.set_sample_data_tree_model(self.model.get_sample_data_model())
        self.model.set_active_model(standard=False)
        self._changed_to_sample()
        # re-adjust view to column width
        self._format_view()

    def _standard_data_clicked(self):
        self.view.set_standard_data_tree_model(self.model.get_standard_data_model())
        self.model.set_active_model(standard=True)
        self._changed_to_standard()
        own_options = self.get_option_dict()
        if own_options["auto_select_standard"]:
            self._check_all_visible_scans()
        # re-adjust view to column width
        self._format_view()

    def _attach_signal_slots(self):
        self.view.sig_expanded.connect(self._expanded)
        self.view.sig_filters_clicked.connect(self._filter_scans)
        self.view.sig_standard_filters_clicked.connect(self._filter_standard)
        self.view.sig_check_all.connect(self._check_all_visible_scans)
        self.view.sig_uncheck_all.connect(self._uncheck_all_scans)
        self.view.sig_check_last.connect(self._check_last_scans)
        self.view.sig_check_selected.connect(self._check_selected_scans)
        self.view.sig_right_click.connect(self._right_click)
        self.view.sig_progress_canceled.connect(self._cancel_loading)
        self.view.sig_autoload_new_clicked.connect(self._autoload_new)
        self.view.sig_auto_select_standard_clicked.connect(self._auto_select_standard)
        self.view.sig_standard_data_clicked.connect(self._standard_data_clicked)
        self.view.sig_sample_data_clicked.connect(self._sample_data_clicked)

        self.watcher.sig_files_changed.connect(self._files_changed_by_watcher)

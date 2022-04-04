# Mantid Repository : https://github.com/mantidproject/mantid
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS File selector Presenter - Tab of DNS Reduction GUI
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver


class DNSFileSelectorPresenter(DNSObserver):

    def __init__(self,
                 name=None,
                 parent=None,
                 view=None,
                 model=None,
                 watcher=None):
        # pylint: disable=too-many-arguments
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.watcher = watcher
        self.view.set_tree_model(self.model.get_model())
        self.view.set_tree_model(self.model.get_model(standard=True),
                                 standard=True)
        self._old_data_set = set()

        # connect signals
        self.view.sig_read_all.connect(self._read_all)
        self.view.sig_read_filtered.connect(self._read_filtered)
        self.view.sig_filters_clicked.connect(self._filter_scans)
        self.view.sig_standard_filters_clicked.connect(self._filter_standard)
        self.view.sig_check_all.connect(self._check_all_vissible_scans)
        self.view.sig_uncheck_all.connect(self._uncheck_all_scans)
        self.view.sig_check_last.connect(self._check_last_scans)
        self.view.sig_check_selected.connect(self._check_selected_scans)
        self.view.sig_right_click.connect(self._right_click)
        self.view.sig_progress_canceled.connect(self._cancel_loading)
        self.view.sig_autoload_clicked.connect(self._autoload)
        self.view.sig_dataset_changed.connect(self._dataset_changed)
        self.watcher.sig_files_changed.connect(self._files_changed_by_watcher)

    # loading

    def _read_all(self, filtered=False, watcher=False, start=None, end=None):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified by start and end
        """
        fn_range = [start, end]
        datapath = self.param_dict['paths']['data_dir']
        nbfiles, loaded, datafiles, fn_range = \
            self.model.set_datafiles_to_load(datapath, fn_range, filtered,
                                             watcher)

        self.view.open_progress_dialog(nbfiles)
        self.model.read_all(datafiles, datapath, loaded, watcher)
        self.view.set_first_column_spanned(self.model.get_scan_range())
        self._filter_scans()
        if self.model.get_number_of_scans() == 1:
            self.view.expand_all()
        if not filtered:
            self._set_start_end(fn_range)

    def _read_filtered(self):
        """ reads only the files in the range given by start and stop fields
            in the view """
        start, end = self.view.get_start_end_filenumbers()
        self._read_all(filtered=True, start=start, end=end)

    def _read_standard(self, selfcall=False):
        """
        Reading of standard files
        """
        datapath = self.param_dict['paths']['data_dir']
        standardpath = self.param_dict['paths']['standards_dir']
        if not standardpath:
            self.raise_error('No path set for standard data')
        standard_found = self.model.read_standard(standardpath)
        self.view.set_first_column_spanned(self.model.get_scan_range())
        self._filter_standard()
        if not standard_found and not selfcall:
            if self.model.try_unzip(datapath, standardpath):
                self._read_standard(selfcall=True)

    def _cancel_loading(self):
        self.model.set_loading_canceled(True)

    def _set_start_end(self, fn_range):
        start, end = fn_range
        self.view.set_start_end_filenumbers_from_arguments(start, end)

    def _files_changed_by_watcher(self):
        """triggered by view if new files are found and timer of 5 s is run
           down"""
        self._read_all(watcher=True)

    def _autoload(self, state):
        datadir = self.param_dict['paths']['data_dir']
        if state == 2 and datadir:
            self.watcher.start_watcher()
            if not self._old_data_set:
                self._read_all()
        else:
            self.watcher.stop_watcher()

    # scan selection

    def _automatic_select_all_standard_files(self):
        was = not self.model.model_is_standard()
        if was:  # if view is not standard we change to it and change back
            self.view.combo_changed(1)
        self._read_standard()
        self._check_all_vissible_scans()
        self.view.show_statusmessage('automatically loaded all standard files',
                                     30)
        if was:
            self.view.combo_changed(0)

    def _check_all_vissible_scans(self):
        self.model.check_scans_by_rows(self._get_non_hidden_rows())

    def _check_last_scans(self, sender_name):
        number_of_scans_to_check = self.view.get_nb_scans_to_check()
        complete = 'complete' in sender_name
        not_hidden_rows = self._get_non_hidden_rows()
        self.model.check_last_scans(number_of_scans_to_check, complete,
                                    not_hidden_rows)

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
        nonhidden_rows = [row for row in scan_range if not hidden(row)]
        return nonhidden_rows

    def _filter_scans(self):
        """
        Hide and uncheck the scans in the treeview which do not
        match filters from get_filters
        """
        self._show_all_scans()
        filters = self.view.get_filters().items()
        hidescans = self.model.filter_scans_for_boxes(filters,
                                                      self._is_modus_tof())
        for row in hidescans:
            self.view.hide_scan(row, hidden=True)

    def _filter_standard(self):
        """
        Hide and uncheck the standard files which do not match filters
        """
        self._show_all_scans()
        filters = self.view.get_standard_filters()
        active = filters['vanadium'] or filters['empty'] or filters['nicr']
        hidescans = self.model.filter_standard_types(filters, active,
                                                     self._is_modus_tof())
        for row in hidescans:
            self.view.hide_scan(row, hidden=True)

    # Change of datasets

    def _changed_to_standard(self):
        if not self.model.get_number_of_scans():
            self._read_standard()
        self._filter_standard()

    def _dataset_changed(self, standard_set):
        if standard_set:
            self.view.sig_read_all.disconnect(self._read_all)
            self.view.sig_read_all.connect(self._read_standard)
            self.model.set_model(standard=True)
            self._changed_to_standard()
        else:
            self.view.sig_read_all.disconnect(self._read_standard)
            self.view.sig_read_all.connect(self._read_all)
            self.model.set_model()
            self._filter_scans()

    # Change of modi

    def _is_modus_tof(self):
        return '_tof' in self.modus

    def _modus_changed(self):
        self._filter_scans()
        self._filter_standard()
        self.view.hide_tof(self._is_modus_tof())

    # model can access this function # have no idea how to do this otherwise

    def update_progress(self, i, end):
        if i % 100 == 0 or i >= end - 1:
            self.view.set_progress(i + 1)

    def _right_click(self, index):
        """Open files in the treeview with rightclick"""
        datapath = self.param_dict['paths']['data_dir']
        standardpath = self.param_dict['paths']['standards_dir']
        self.model.open_datafile(index, datapath, standardpath)

    # normal observer function

    def get_option_dict(self):
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        self.own_dict['full_data'] = self.model.get_data()
        self.own_dict['standard_data'] = self.model.get_data(standard=True)
        self.own_dict['selected_filenumbers'] = self.model.get_data(
            fullinfo=False)
        return self.own_dict

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['auto_standard'] and not own_options['standard_data']:
            self._automatic_select_all_standard_files()

    def set_view_from_param(self):
        """
        Setting of the fields defined in mapping from the parameter dictionary
        and checks scans checked in the dict
        """
        filenumbers = self.param_dict[self.name].pop('selected_filenumbers',
                                                     [])
        self.view.set_state(self.param_dict.get(self.name, {}))
        notfound = self.model.check_by_filenumbers(filenumbers)
        if notfound:
            print(f'Of {len(filenumbers)} loaded checked '
                  f'filenumbers {notfound} were not found '
                  'in list of datafiles')

    def tab_got_focus(self):
        if self.model.model_is_standard():
            self._filter_standard()
        else:
            self._filter_scans()
        self.view.hide_tof(hidden='_tof' not in self.modus)

    def process_commandline_request(self, command_dict):
        ffnmb = int(command_dict['files'][0]['ffnmb'])
        lfnmb = int(command_dict['files'][0]['lfnmb'])
        self.view.set_start_end_filenumbers_from_arguments(ffnmb, lfnmb)
        self._read_filtered()
        self.model.check_fn_range(ffnmb, lfnmb)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS File selector Presenter - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QFileSystemWatcher, QTimer

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.data_structures.dns_treemodel import DNSTreeModel
from DNSReduction.data_structures.dns_file import DNSFile
from DNSReduction.file_selector.file_selector_view import DNSFileSelector_view
from DNSReduction.helpers.file_processing import filter_filenames
from DNSReduction.helpers.file_processing import return_filelist
from DNSReduction.data_structures.object_dict import ObjectDict


class DNSFileSelector_presenter(DNSObserver):
    def __init__(self, parent):
        super(DNSFileSelector_presenter,
              self).__init__(parent, 'file_selector')
        self.view = DNSFileSelector_view(self.parent.view)
        self.name = 'file_selector'
        self.model = DNSTreeModel()
        self.active_model = self.model

        self.standard_data = DNSTreeModel()
        self.view.set_tree_model(self.model)
        self.view.set_tree_model(self.standard_data, standard=True)
        self.loading_canceled = False
        self.old_data_set = set()
        self.fs_watcher = QFileSystemWatcher()
        self.autoload_timer = QTimer()

        ## connect signals
        self.view.sig_read_all.connect(self.read_all)
        self.view.sig_filters_clicked.connect(self.filter_scans)
        self.view.sig_standard_filters_clicked.connect(self.filter_standard)
        self.view.sig_check_all.connect(self.check_all_scans)
        self.view.sig_uncheck_all.connect(self.uncheck_all_scans)
        self.view.sig_check_last.connect(self.check_last_scans)
        self.view.sig_check_selected.connect(self.check_selected_scans)

        self.view.sig_progress_canceled.connect(self.cancel_loading)
        self.view.sig_autoload_clicked.connect(self.autoload)
        self.sig_modus_changed.connect(self.modus_changed)
        self.view.sig_dataset_changed.connect(self.dataset_changed)
        self.autoload_timer.timeout.connect(self.sequential_load)

    def autoload(self, state):
        """Turn on an off watcher for changes in data directory """
        datadir = self.param_dict['paths']['data_dir']
        if state == 2 and datadir:
            self.fs_watcher.addPath(datadir)
            self.fs_watcher.directoryChanged.connect(self.files_changed)
            if not self.old_data_set:  # if nothing has been read - read dir
                self.read_all()
        else:
            self.fs_watcher.removePaths(self.fs_watcher.directories())
            if datadir:
                self.fs_watcher.directoryChanged.disconnect()

    def uncheck_all_scans(self):
        self.model.uncheck_all_scans()

    def check_all_scans(self):
        for row in self.model.scanRange():
            if not self.view.is_scan_hidden(row):
                self.model.setCheckedScan(row, 2)

    def get_non_hidden_scan_rows(self):
        scan_rows = self.model.get_scan_rows()
        return [row for row in scan_rows
                if not self.view.is_scan_hidden(row)]

    def get_complete_scan_rows(self):
        scan_rows = self.get_non_hidden_scan_rows()
        return [row for row in scan_rows
                if self.model.is_scan_complete(row)]

    def check_last_scans(self, sender_name):
        self.uncheck_all_scans()
        number_of_scans_to_check = self.view.get_nb_scans_to_check()
        if 'complete' in sender_name:
            scans = self.get_complete_scan_rows()
        else:
            scans = self.get_non_hidden_scan_rows()
        for row in scans[-number_of_scans_to_check:]:
            self.model.setCheckedScan(row, 2)

    def check_selected_scans(self):
        self.model.uncheck_all_scans()
        for index in self.view.get_selected_indexes():
            self.model.setCheckedFromIndex(index, 2)

    def cancel_loading(self):
        self.loading_canceled = True

    def changed_to_standard(self):
        if self.standard_data.numberOfScans() == 0:
            self.read_standard()
        self.filter_standard()

    def dataset_changed(self, standard_set):
        if standard_set:
            self.view.sig_read_all.disconnect(self.read_all)
            self.view.sig_read_all.connect(self.read_standard)
            self.active_model = self.standard_data
            self.changed_to_standard()
        else:
            self.view.sig_read_all.disconnect(self.read_standard)
            self.view.sig_read_all.connect(self.read_all)
            self.active_model = self.model
            self.filter_scans()

    def is_modus_tof(self):
        return '_tof' in self.modus

    def switch_tof_scans(self):
        for row in self.active_model.scanRange():
            if self.is_modus_tof() != self.active_model.is_scan_tof(row):
                self.view.hide_scan(row)
                self.active_model.setCheckedScan(row, 0)
            else:
                self.view.show_scan(row)

    def filter_scans_for_boxes(self):
        model = self.model
        view = self.view
        for row in model.scanRange():
            for text, filter_condition in view.get_filters().items():
                if (filter_condition and not model.text_in_scan(row, text)):
                    view.hide_scan(row, hidden=True)
                    model.setCheckedScan(row, 0)

    def filter_scans(self):
        """
        Hide and uncheck the scans in the treeview which do not
        match filters from get_filters
        """
        self.switch_tof_scans()
        self.filter_scans_for_boxes()
        return

    def files_changed(self):
        ## we add a 5 second delay, just to not trigger glob for
        ## every single file which is copied to the directory
        if not self.autoload_timer.isActive():
            self.autoload_timer.start(5000)

    def standard_filter_active(self):
        filters = self.view.get_standard_filters()
        return filters['vanadium'] or filters['empty'] or filters['nicr']

    def filter_standard_types(self):
        filters = self.view.get_standard_filters()
        for row in self.active_model.scanRange():
            scan = self.active_model.scanFromRow(row)
            hidden = [filters[sampletype] and scan.is_type(sampletype)
                      for sampletype in ['vanadium', 'nicr', 'empty']]
            if not any(hidden) and self.standard_filter_active():
                self.view.hide_scan(row)

    def filter_standard(self):
        """
        Hide and uncheck the standard files which do not match filters
        """
        self.switch_tof_scans()
        self.filter_standard_types()
        return

    def get_option_dict(self):
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        self.own_dict['full_data'] = self.model.get_checked(fullinfo=True)
        self.own_dict['standard_data'] = self.standard_data.get_checked(
            fullinfo=True)
        self.own_dict['selected_filenumbers'] = self.model.get_checked(
            fullinfo=False)
        return self.own_dict

    def modus_changed(self):
        self.filter_scans()
        self.filter_standard()
        self.view.hide_tof(self.is_modus_tof())

    def check_all_vissible_scans(self):
        for row in self.active_model.scanRange():
            if not self.view.is_scan_hidden(row):
                self.active_model.setCheckedScan(row, 2)

    def automatic_select_all_standard_files(self):
        was = self.active_model != self.standard_data
        if was: # if view is not standard we change to it and change back
            self.view.combo_changed(1)
        self.read_standard()
        self.check_all_vissible_scans()
        self.view.show_statusmessage(
            'automatically loaded all standard files', 30)
        if was:
            self.view.combo_changed(0)

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['auto_standard'] and not own_options['standard_data']:
            self.automatic_select_all_standard_files()

    def load_saved_filelist(self):
        """
        we save the list of loaed files so we have to read the files only once
        """
        path = self.param_dict['paths']['data_dir']
        loaded = {}
        try:
            with open(path + '/last_filelist.txt', 'r') as the_file:
                txt = the_file.readlines()
        except IOError:
            return loaded
        for line in txt:
            dnsfile = ObjectDict()
            data = line[0:-1].split(' ; ')
            dnsfile.filenumber = data[0]
            dnsfile.det_rot = float(data[1])
            dnsfile.sample_rot = float(data[2])
            dnsfile.field = data[3]
            dnsfile.temp_samp = float(data[4])
            dnsfile.sample = data[5]
            dnsfile.endtime = data[6]
            dnsfile.tofchannels = int(data[7])
            dnsfile.channelwidth = float(data[8])
            dnsfile.filename = data[9]
            dnsfile.wavelength = float(data[10])
            dnsfile.selector_speed = float(data[11])
            dnsfile.scannumber = data[12]
            dnsfile.scancommand = data[13]
            dnsfile.scanpoints = data[14]
            dnsfile.new_format = True
            loaded[dnsfile.filename] = dnsfile
        return loaded

    def save_filelist(self):
        txt = self.model.get_txt()
        datapath = self.param_dict['paths']['data_dir']
        with open(datapath + '/last_filelist.txt', 'w') as the_file:
            the_file.writelines(txt)

    def filter_out_files_already_loaded(self, alldatafiles, watcher):
        if watcher:
            datafiles = [dfile for dfile in alldatafiles
                         if dfile not in self.old_data_set]
        else:
            datafiles = alldatafiles
        return datafiles

    def filter_range(self, datafiles, filtered=False):
        if filtered:
            start, end = self.view.get_start_end_filenumbers()
            datafiles = filter_filenames(datafiles, start, end)
        elif datafiles:
            self.view.set_start_end_filenumbers(datafiles)
        return datafiles

    def get_list_of_loaded_files(self, watcher):
        if watcher:
            return {}
        return self.load_saved_filelist()

    def clear_scans_if_not_sequential(self, watcher):
        if not watcher:
            self.model.clearScans()

    def update_progress(self, i, end):
        if i % 100 == 0 or i == end - 1:
            self.view.set_progress(i + 1)

    def load_file_from_chache_or_new(self, loaded, filename, datapath):
        dnsfile = loaded.get(filename, False)
        if not dnsfile:
            dnsfile = DNSFile(datapath, filename)
        return dnsfile

    def add_number_of_files_per_scan(self):
        total_files = self.model.add_number_of_childs()
        self.view.show_statusmessage("{} files loaded".format(total_files))

    def read_all(self, filtered=False, watcher=False):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified
        """
        view = self.view
        model = self.model
        self.loading_canceled = False
        datapath = self.param_dict['paths']['data_dir']
        alldatafiles = return_filelist(datapath)
        datafiles = self.filter_out_files_already_loaded(alldatafiles, watcher)
        self.clear_scans_if_not_sequential(watcher)
        loaded = self.get_list_of_loaded_files(watcher)
        datafiles = self.filter_range(datafiles, filtered)
        number_of_datafiles = len(datafiles)
        self.view.open_progress_dialog(number_of_datafiles)
        for i, filename in enumerate(datafiles):
            self.update_progress(i, number_of_datafiles)
            if self.loading_canceled:
                break
            dnsfile = self.load_file_from_chache_or_new(loaded,
                                                        filename, datapath)
            if dnsfile.new_format:
                model.setupModelData([dnsfile])
        self.old_data_set = set(alldatafiles)
        self.add_number_of_files_per_scan()
        view.set_first_column_spanned(model.scanRange())
        self.filter_scans()
        if model.numberOfScans() == 1:
            self.view.expand(alle=True)
        self.save_filelist()

    def read_standard(self):
        """
        Reding of standard files
        """
        if not self.param_dict['paths']['standards_dir']:
            self.raise_error('No path set for standard data')
            return
        view = self.view
        model = self.standard_data
        datapath = self.param_dict['paths']['standards_dir']
        datafiles = return_filelist(datapath)
        model.clearScans()
        for filename in datafiles:
            dnsfile = DNSFile(datapath, filename)
            model.setupModelData([dnsfile])
        model.add_number_of_childs()
        view.set_first_column_spanned(model.scanRange())
        self.filter_standard()

    def set_view_from_param(self):
        """
        Setting of the fields defined in mapping from the parameter dictionary
        and checks scans checked in the dict
        """
        filenumbers = self.param_dict[self.name].pop('selected_filenumbers',
                                                     [])
        self.view.set_state(self.param_dict.get(self.name, {}))
        filenb_dict = self.model.get_filenumber_dict()
        notfound = 0
        for filenb in filenumbers:
            try:
                self.model.setCheckedFromIndex(filenb_dict[filenb])
            except KeyError:
                notfound += 1
        if notfound:
            print('Of {} loaded checked filenumbers {} were not found '
                  'in list of datafiles'
                  .format(len(filenumbers), notfound))

    def sequential_load(self):
        self.autoload_timer.stop()
        self.read_all(watcher=True)

    def tab_got_focus(self):
        if self.active_model == self.standard_data:
            self.filter_standard()
        else:
            self.filter_scans()
        self.view.hide_tof(hidden='_tof' not in self.modus)

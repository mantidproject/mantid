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
        self.view.sig_checked_clicked.connect(self.check_buttons)
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

    def check_buttons(self, sender_name):
        """
        Processing of the button clicks to check All None, Last scan, Last
        complete Scan and selected
        """
        view = self.view
        model = self.active_model
        for row in model.scanRange():  ## deselect all also hidden
            model.setCheckedScan(row, 0)
        if 'all' in sender_name:
            for row in model.scanRange():
                if not view.is_scan_hidden(row):
                    model.setCheckedScan(row, 2)
        if 'last' in sender_name:
            row = model.numberOfScans() - 1
            number_of_scans_to_check = view.get_nb_scans_to_check()
            if number_of_scans_to_check > model.numberOfScans():
                return
            for i in range(number_of_scans_to_check):
                while ((row - i) >= 0
                       and (view.is_scan_hidden(row - i)
                            or 'scan' not in model.scanCommandFromRow(row - i)
                            )
                       ):
                    row += -1  # do not check hidden rows
                while ((row - i) >= 0
                       and ('scan' not in model.scanCommandFromRow(row - i)
                            or view.is_scan_hidden(row - i)
                            or ('complete' in sender_name
                                and (row - i > 0)
                                and (model.scanFromRow(row - i).childCount()
                                     < model.scanExpectedPointsFromRow(row
                                                                       - i))
                                )
                            )
                       ):
                    row += -1
                model.setCheckedScan(row - i, 2)
        elif 'selected' in sender_name:
            for index in view.get_selected_indexes():
                model.setCheckedFromIndex(index, 2)
        return

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

    def filter_scans(self):
        """
        Hide and uncheck the scans in the treeview which do not
        match filters from get_filters
        """
        tofmode = '_tof' in self.modus
        model = self.model
        filters = self.view.get_filters()
        for row in model.scanRange():
            self.view.hide_scan(row, hidden=False)
            ### filtering for selected filters
            if tofmode != model.is_scan_tof(row):
                self.view.hide_scan(row, hidden=True)
                model.setCheckedScan(row, 0)
            for text, filter_condition in filters.items():
                if (filter_condition
                        and text not in model.scanCommandFromRow(row)):
                    self.view.hide_scan(row, hidden=True)
                    model.setCheckedScan(row, 0)
        return

    def files_changed(self):
        ## we add a 5 second delay, just to not trigger glob for
        ## every single file which is copied to the directory
        if not self.autoload_timer.isActive():
            self.autoload_timer.start(5000)

    def filter_standard(self):
        """
        Hide and uncheck the standard files which do not match filters
        """
        tofmode = '_tof' in self.modus
        model = self.standard_data
        filters = self.view.get_standard_filters()
        for row in model.scanRange():
            scan = model.scanFromRow(row)
            scanindex = model.IndexFromScan(scan)
            self.view.hide_scan(row, hidden=False)
            unhiddenchild = False
            if tofmode != model.is_scan_tof(row):
                self.view.hide_scan(row, hidden=True)
                model.setCheckedScan(row, 0)
            for child in scan.get_childs():
                if not filters['vanadium'] and not filters[
                        'empty'] and not filters['nicr']:
                    self.view.hide_file(child, scanindex, hidden=False)
                else:
                    self.view.hide_file(child, scanindex, hidden=True)
                sample = child.data()[5]
                if (filters['vanadium']
                        and ('vanadium' in sample
                             or 'vana' in sample)):
                    self.view.hide_file(child, scanindex, hidden=False)
                if (filters['nicr']
                        and ('nicr' in sample
                             or 'NiCr' in sample)):
                    self.view.hide_file(child, scanindex, hidden=False)
                if (filters['empty']
                        and ('empty' in sample
                             or 'leer' in sample)):
                    self.view.hide_file(child, scanindex, hidden=False)
                if not self.view.is_file_hidden(child, scanindex):
                    unhiddenchild = True
            if not unhiddenchild:
                self.view.hide_scan(row, hidden=True)
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
        self.view.hide_tof(hidden='_tof' not in self.modus)

    def process_request(self):
        own_options = self.get_option_dict()
        view = self.view
        was = self.active_model != self.standard_data
        if was:
            view.combo_changed(1)
        model = self.standard_data
        if own_options['auto_standard'] and not own_options['standard_data']:
            self.read_standard()
            for row in model.scanRange():
                if not view.is_scan_hidden(row):
                    model.setCheckedScan(row, 2)
            self.view.show_statusmessage(
                'automatically loaded all standard files', 30)
        if was:
            view.combo_changed(0)

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
        datafiles = alldatafiles
        if watcher:  #sequential load called
            datafiles = [
                dfile for dfile in alldatafiles
                if dfile not in self.old_data_set
            ]  #
            loaded = {}
        else:  # load all called manually
            model.clearScans()
            datafiles = alldatafiles
            loaded = self.load_saved_filelist()
        if filtered:
            start, end = view.get_start_end_filenumbers()
            datafiles = filter_filenames(datafiles, start, end)
        if datafiles:
            self.view.open_progress_dialog(len(datafiles))
        for i, filename in enumerate(datafiles):
            ### update progressbar every 100 files
            if i % 100 == 0 or i == len(datafiles) - 1:
                self.view.set_progress(i + 1)
                if self.loading_canceled:
                    break
            dnsfile = loaded.get(filename, False)
            if not dnsfile:
                dnsfile = DNSFile(datapath, filename)
            if dnsfile.new_format:
                model.setupModelData([dnsfile])
        if not filtered and datafiles:
            view.set_start_end_filenumbers(datafiles)
        self.old_data_set = set(alldatafiles)
        total_files = model.add_number_of_childs()
        view.set_first_column_spanned(model.scanRange())
        self.filter_scans()
        if model.numberOfScans() == 1:
            self.view.expand(alle=True)
        self.save_filelist()
        self.view.show_statusmessage("{} files loaded".format(total_files))

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
        Settin of the fields defined in mapping from the parameter dictionary
        and checks scans checked in the dict
        """
        filenumbers = self.param_dict.get(self.name,
                                          {}).get('selected_filenumbers', [])
        self.param_dict[self.name].pop('selected_filenumbers', None)
        self.view.set_state(self.param_dict.get(self.name, {}))
        filenb_dict = {}
        notfound = 0
        for scannb in self.model.scanRange():
            scan = self.model.scanFromRow(scannb)
            scanindex = self.model.IndexFromScan(scan)
            for row in range(scan.childCount()):
                filenb = int(scan.child(row).data(0))
                index = self.model.index(row, 0, scanindex)
                filenb_dict[filenb] = index
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

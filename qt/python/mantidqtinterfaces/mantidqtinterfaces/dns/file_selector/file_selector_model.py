# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
presenter for dns path panel
"""

from mantidqtinterfaces.dns.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns.data_structures.dns_treemodel import DNSTreeModel
from mantidqtinterfaces.dns.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns.helpers.file_processing import (
    filter_filenames, load_txt, open_editor, return_filelist, save_txt,
    unzip_latest_standard)


class DNSFileSelectorModel(DNSObsModel):

    def __init__(self, parent=None):
        super().__init__(parent)
        # model has this function to call back on parent
        # no idea how to avoid this
        self.update_progress = parent.update_progress
        # Qt treemodels
        self.treemodel = DNSTreeModel()
        self.standard_data = DNSTreeModel()
        self.active_model = self.treemodel
        self.old_data_set = None
        self.alldatafiles = None
        self.loading_canceled = False

    def _filter_out_already_loaded(self, alldatafiles, watcher):
        if watcher:
            datafiles = [
                dfile for dfile in alldatafiles
                if dfile not in self.old_data_set
            ]
        else:
            datafiles = alldatafiles
        return datafiles

    def set_datafiles_to_load(
        self,
        datapath,
        fn_range,
        filtered=False,
        watcher=False,
    ):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified
        """
        self.alldatafiles = return_filelist(datapath)
        datafiles = self._filter_out_already_loaded(self.alldatafiles, watcher)
        loaded = self._get_list_of_loaded_files(datapath, watcher)
        datafiles, fn_range = self._filter_range(datafiles, fn_range, filtered)
        number_of_datafiles = len(datafiles)
        return number_of_datafiles, loaded, datafiles, fn_range

    def _get_start_end_filenumbers(self):
        if self.alldatafiles:
            start = int(self.alldatafiles[0].split('_')[-2][:-2])
            end = int(self.alldatafiles[-1].split('_')[-2][:-2])
            return [start, end]
        return [0, 0]

    def _filter_range(self, datafiles, fn_range, filtered=False):
        start, end = fn_range
        if start is None or end is None:
            fn_range = self._get_start_end_filenumbers()
        if filtered:
            datafiles = filter_filenames(datafiles, start, end)
        return datafiles, fn_range

    def read_all(self, datafiles, datapath, loaded, watcher=False):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified
        """
        self.loading_canceled = False
        self._clear_scans_if_not_sequential(watcher)
        for i, filename in enumerate(datafiles):
            self.update_progress(i, len(datafiles))
            if self.loading_canceled:
                break
            dnsfile = self._load_file_from_chache_or_new(
                loaded, filename, datapath)
            if dnsfile.new_format:  # ignore files with old format
                self.treemodel.setup_model_data([dnsfile])
        self.old_data_set = set(self.alldatafiles)
        self._add_number_of_files_per_scan()
        self._save_filelist(datapath)

    def read_standard(self, standardpath):
        """
        Reeding of standard files
        """
        model = self.standard_data
        datafiles = return_filelist(standardpath)
        model.clear_scans()
        for filename in datafiles:
            dnsfile = DNSFile(standardpath, filename)
            model.setup_model_data([dnsfile])
        model.add_number_of_childs()
        return model.rowCount()

    @staticmethod
    def try_unzip(datapath, standardpath):
        # if standard directory is empty try to get standard files from zip
        if standardpath and datapath:
            return unzip_latest_standard(datapath, standardpath)
        return False

    def _add_number_of_files_per_scan(self):
        self.treemodel.add_number_of_childs()

    def _clear_scans_if_not_sequential(self, watcher):
        if not watcher:
            self.treemodel.clear_scans()

    def _get_list_of_loaded_files(self, datapath, watcher):
        if watcher:
            return {}
        return self._load_saved_filelist(datapath)

    @staticmethod
    def _load_saved_filelist(path):
        """
        we save the list of loaed files so we have to read the files only once
        """
        loaded = {}
        try:
            txt = load_txt('last_filelist.txt', path)
        except IOError:
            return loaded
        try:
            for line in txt:
                dnsfile = ObjectDict()
                data = line[0:-1].split(' ; ')
                if len(data) < 15:
                    continue
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
        except IndexError:
            pass
        return loaded

    def get_number_of_scans(self):
        return self.active_model.number_of_scans()

    def get_scan_range(self):
        return range(self.get_number_of_scans())

    # scan checking

    def check_last_scans(self, number_of_scans_to_check, complete,
                         not_hidden_rows):
        self.uncheck_all_scans()
        scans = not_hidden_rows
        if complete:
            scans = self.active_model.get_complete_scan_rows(not_hidden_rows)
        self.check_scans_by_rows(scans[-number_of_scans_to_check:])

    def check_scans_by_indexes(self, indexes):
        self.active_model.check_scans_by_indexes(indexes)

    def check_scans_by_rows(self, rows):
        self.active_model.check_scans_by_rows(rows)

    def uncheck_all_scans(self):
        self.active_model.uncheck_all_scans()

    def check_by_filenumbers(self, filenumbers):
        notfound = 0
        filenb_dict = self.treemodel.get_filenumber_dict()
        for filenb in filenumbers:
            try:
                self.treemodel.set_checked_from_index(filenb_dict[filenb])
            except KeyError:
                notfound += 1
        return notfound

    def check_fn_range(self, ffnmb, lfnmb):
        self.treemodel.check_fn_range(ffnmb, lfnmb)

    def set_loading_canceled(self, canceled=True):
        self.loading_canceled = canceled

    def get_model(self, standard=False):
        if standard:
            return self.standard_data
        return self.treemodel

    # data receiving

    def model_is_standard(self):
        return self.active_model == self.standard_data

    def get_data(self, standard=False, fullinfo=True):
        if standard:
            return self.standard_data.get_checked(fullinfo=True)
        return self.treemodel.get_checked(fullinfo=fullinfo)

    def set_model(self, standard=False):
        if standard:
            self.active_model = self.standard_data
        else:
            self.active_model = self.treemodel

    # scan filtering:

    def filter_scans_for_boxes(self, filters, is_tof):
        model = self.active_model
        hidescans = self._filter_tof_scans(is_tof)
        for row in self.get_scan_range():
            for text, filter_condition in filters:
                if filter_condition and not model.text_in_scan(row, text):
                    model.set_checked_scan(row, 0)
                    hidescans.add(row)
        return hidescans

    def _filter_tof_scans(self, is_tof):
        hidescans = set()
        for row in self.get_scan_range():
            if is_tof != self.active_model.is_scan_tof(row):
                hidescans.add(row)
                self.active_model.set_checked_scan(row, 0)
        return hidescans

    def filter_standard_types(self, filters, active, is_tof):
        hidescans = self._filter_tof_scans(is_tof)
        for row in self.get_scan_range():
            scan = self.active_model.scan_from_row(row)
            hidden = [
                filters[sampletype] and scan.is_type(sampletype)
                for sampletype in ['vanadium', 'nicr', 'empty']
            ]
            if not any(hidden) and active:
                hidescans.add(row)
        return hidescans

    # opening data files in external editor

    def open_datafile(self, index, datapath, standardpath):
        filename = self.active_model.get_filename_from_index(index)
        if self.active_model == self.standard_data:
            path = standardpath
        else:
            path = datapath
        if filename:
            open_editor(filename, path)

    # Chaching of filelist
    @staticmethod
    def _load_file_from_chache_or_new(loaded, filename, datapath):
        dnsfile = loaded.get(filename, False)
        if not dnsfile:
            dnsfile = DNSFile(datapath, filename)
        return dnsfile

    def _save_filelist(self, datapath):
        txt = "".join(self.treemodel.get_txt())
        try:
            save_txt(txt, 'last_filelist.txt', datapath)
        except PermissionError:
            pass

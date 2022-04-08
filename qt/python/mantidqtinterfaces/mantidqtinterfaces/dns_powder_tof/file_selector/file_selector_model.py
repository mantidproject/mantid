# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Presenter for dns path panel.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_file import DNSFile
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treemodel import DNSTreeModel
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import (
    filter_filenames, load_txt, open_editor, return_filelist, save_txt,
    unzip_latest_standard)


class DNSFileSelectorModel(DNSObsModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        # Model has this function to call back on parent.
        # No idea how to avoid this.
        self.update_progress = parent.update_progress
        # Qt tree models
        self.tree_model = DNSTreeModel()
        self.standard_data = DNSTreeModel()
        self.active_model = self.tree_model
        self.old_data_set = None
        self.all_datafiles = None
        self.loading_canceled = False

    def _filter_out_already_loaded(self, all_datafiles, watcher):
        if watcher:
            datafiles = [
                data_file for data_file in all_datafiles
                if data_file not in self.old_data_set
            ]
        else:
            datafiles = all_datafiles
        return datafiles

    def set_datafiles_to_load(self, data_path, fn_range, filtered=False, watcher=False):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified.
        """
        self.all_datafiles = return_filelist(data_path)
        datafiles = self._filter_out_already_loaded(self.all_datafiles, watcher)
        loaded = self._get_list_of_loaded_files(data_path, watcher)
        datafiles, fn_range = self._filter_range(datafiles, fn_range, filtered)
        number_of_datafiles = len(datafiles)
        return number_of_datafiles, loaded, datafiles, fn_range

    def _get_start_end_file_numbers(self):
        if self.all_datafiles:
            start = int(self.all_datafiles[0].split('_')[-2][:-2])
            end = int(self.all_datafiles[-1].split('_')[-2][:-2])
            return [start, end]
        return [0, 0]

    def _filter_range(self, datafiles, fn_range, filtered=False):
        start, end = fn_range
        if start is None or end is None:
            fn_range = self._get_start_end_file_numbers()
        if filtered:
            datafiles = filter_filenames(datafiles, start, end)
        return datafiles, fn_range

    def read_all(self, datafiles, data_path, loaded, watcher=False):
        """
        Reading of new files, if filtered is True, only the files in the
        range specified.
        """
        self.loading_canceled = False
        self._clear_scans_if_not_sequential(watcher)
        for i, filename in enumerate(datafiles):
            self.update_progress(i, len(datafiles))
            if self.loading_canceled:
                break
            dns_file = self._load_file_from_chache_or_new(
                loaded, filename, data_path)
            if dns_file.new_format:  # ignore files with old format
                self.tree_model.setup_model_data([dns_file])
        self.old_data_set = set(self.all_datafiles)
        self._add_number_of_files_per_scan()
        self._save_filelist(data_path)

    def read_standard(self, standard_path):
        """
        Reading of standard files.
        """
        model = self.standard_data
        datafiles = return_filelist(standard_path)
        model.clear_scans()
        for filename in datafiles:
            dns_file = DNSFile(standard_path, filename)
            model.setup_model_data([dns_file])
        model.add_number_of_children()
        return model.rowCount()

    @staticmethod
    def try_unzip(data_path, standard_path):
        # if standard directory is empty try to get standard files from zip
        if standard_path and data_path:
            return unzip_latest_standard(data_path, standard_path)
        return False

    def _add_number_of_files_per_scan(self):
        self.tree_model.add_number_of_children()

    def _clear_scans_if_not_sequential(self, watcher):
        if not watcher:
            self.tree_model.clear_scans()

    def _get_list_of_loaded_files(self, data_path, watcher):
        if watcher:
            return {}
        return self._load_saved_filelist(data_path)

    @staticmethod
    def _load_saved_filelist(path):
        """
        We save the list of load files, so we have to read the files only once.
        """
        loaded = {}
        try:
            txt = load_txt('last_filelist.txt', path)
        except IOError:
            return loaded
        try:
            for line in txt:
                dns_file = ObjectDict()
                data = line[0:-1].split(' ; ')
                if len(data) < 15:
                    continue
                dns_file.file_number = data[0]
                dns_file.det_rot = float(data[1])
                dns_file.sample_rot = float(data[2])
                dns_file.field = data[3]
                dns_file.temp_samp = float(data[4])
                dns_file.sample = data[5]
                dns_file.end_time = data[6]
                dns_file.tof_channels = int(data[7])
                dns_file.channel_width = float(data[8])
                dns_file.filename = data[9]
                dns_file.wavelength = float(data[10])
                dns_file.selector_speed = float(data[11])
                dns_file.scan_number = data[12]
                dns_file.scan_command = data[13]
                dns_file.scan_points = data[14]
                dns_file.new_format = True
                loaded[dns_file.filename] = dns_file
        except IndexError:
            pass
        return loaded

    def get_number_of_scans(self):
        return self.active_model.number_of_scans()

    def get_scan_range(self):
        return range(self.get_number_of_scans())

    # Scan checking
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

    def check_by_file_numbers(self, file_numbers):
        not_found = 0
        file_number_dict = self.tree_model.get_file_number_dict()
        for file_number in file_numbers:
            try:
                self.tree_model.set_checked_from_index(
                    file_number_dict[file_number])
            except KeyError:
                not_found += 1
        return not_found

    def check_fn_range(self, ffnmb, lfnmb):
        self.tree_model.check_fn_range(ffnmb, lfnmb)

    def set_loading_canceled(self, canceled=True):
        self.loading_canceled = canceled

    def get_model(self, standard=False):
        if standard:
            return self.standard_data
        return self.tree_model

    # Data receiving
    def model_is_standard(self):
        return self.active_model == self.standard_data

    def get_data(self, standard=False, full_info=True):
        if standard:
            return self.standard_data.get_checked(full_info=True)
        return self.tree_model.get_checked(full_info=full_info)

    def set_model(self, standard=False):
        if standard:
            self.active_model = self.standard_data
        else:
            self.active_model = self.tree_model

    # Scan filtering
    def filter_scans_for_boxes(self, filters, is_tof):
        model = self.active_model
        hide_scans = self._filter_tof_scans(is_tof)
        for row in self.get_scan_range():
            for text, filter_condition in filters:
                if filter_condition and not model.text_in_scan(row, text):
                    model.set_checked_scan(row, 0)
                    hide_scans.add(row)
        return hide_scans

    def _filter_tof_scans(self, is_tof):
        hide_scans = set()
        for row in self.get_scan_range():
            if is_tof != self.active_model.is_scan_tof(row):
                hide_scans.add(row)
                self.active_model.set_checked_scan(row, 0)
        return hide_scans

    def filter_standard_types(self, filters, active, is_tof):
        hide_scans = self._filter_tof_scans(is_tof)
        for row in self.get_scan_range():
            scan = self.active_model.scan_from_row(row)
            hidden = [
                filters[sample_type] and scan.is_type(sample_type)
                for sample_type in ['vanadium', 'nicr', 'empty']
            ]
            if not any(hidden) and active:
                hide_scans.add(row)
        return hide_scans

    # Opening data files in external editor
    def open_datafile(self, index, data_path, standard_path):
        filename = self.active_model.get_filename_from_index(index)
        if self.active_model == self.standard_data:
            path = standard_path
        else:
            path = data_path
        if filename:
            open_editor(filename, path)

    # Caching of filelist
    @staticmethod
    def _load_file_from_chache_or_new(loaded, filename, data_path):
        dns_file = loaded.get(filename, False)
        if not dns_file:
            dns_file = DNSFile(data_path, filename)
        return dns_file

    def _save_filelist(self, data_path):
        txt = "".join(self.tree_model.get_txt())
        try:
            save_txt(txt, 'last_filelist.txt', data_path)
        except PermissionError:
            pass

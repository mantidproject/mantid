# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Presenter for DNS path panel.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver


class DNSPathPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self._load_polarisation_table = True
        self.view.sig_data_path_is_set.connect(self._set_data_path)
        self.view.sig_clear_cache.connect(self._clear_cache)
        self.view.sig_file_dialog_requested.connect(self._file_dialog_requested)
        if not self.view.within_mantid:
            # if launches from commandline, set path to current working
            # directory
            self.view.set_data_path(self.model.get_current_directory())

    def _cache_polarisation_table(self, polarisation_table):
        own_dict = self.get_option_dict()
        own_dict["polarisation_table"] = polarisation_table

    def _set_data_path(self, dir_name):
        own_dict = self.get_option_dict()
        if own_dict["auto_set_other_dir"]:
            for typename in ["psd", "standards", "script", "export"]:
                self.view.set_path(typename + "_dir", dir_name + "/" + typename)
        self._set_user_prop_from_datafile(dir_name)

    def _set_user_prop_from_datafile(self, dir_name):
        load_polarisation_table = self._load_polarisation_table
        user, prop_nb, pol_table = self.model.get_user_and_proposal_number(dir_name, load_polarisation_table)
        if prop_nb or user:
            self._load_polarisation_table = False
            self.view.set_prop_number(prop_nb)
            self.view.set_user(user)
            self.view.set_asterisk(hide=True)
            self._cache_polarisation_table(pol_table)
        else:
            self.view.set_asterisk(hide=False)
            self.view._clear_user_and_proposal_number()
            self.view.show_status_message("No DNS .d_dat file found in Data Directory", 30)

    def _clear_cache(self):
        path = self.own_dict.get("data_dir", False)
        if path:
            self.model.clear_cache(path)

    def _file_dialog_requested(self, sender):
        path = self.view.get_path("data_dir")
        start_path = self.model.get_start_path_for_dialog(path)
        dir_name = self.view.open_file_dialog(start_path)
        if dir_name:
            # wasn't chosen
            if sender == "data":
                self.view.set_data_path(dir_name)
            else:
                self.view.set_path(sender + "_dir", dir_name)

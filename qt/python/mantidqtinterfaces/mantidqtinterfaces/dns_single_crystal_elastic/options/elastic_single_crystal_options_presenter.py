# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic options tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter import DNSCommonOptionsPresenter
from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_binning import get_automatic_omega_binning, get_automatic_two_theta_binning


class DNSElasticSCOptionsPresenter(DNSCommonOptionsPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self._attach_signal_slots()

    def _set_binning(self):
        sample_data = self.param_dict["file_selector"]["full_data"]
        if sample_data:
            self._set_binning_lims(sample_data)
            if self._get_automatic_binning_is_checked():
                self._set_auto_binning(sample_data)
            self.set_view_from_param()
        else:
            if self._get_automatic_binning_is_checked():
                self._set_default_auto_binning_no_data()

    def _set_auto_binning(self, sample_data):
        own_options = self.get_option_dict()
        binning_data_dict = {}
        binning_data_dict["two_theta"] = get_automatic_two_theta_binning(sample_data)
        binning_data_dict["omega"] = get_automatic_omega_binning(sample_data)
        for key in ["two_theta", "omega"]:
            for data in binning_data_dict[key].keys():
                own_options[data] = binning_data_dict[key][data]

    def _set_binning_lims(self, sample_data):
        binning_data_dict = {}
        binning_data_dict["two_theta"] = get_automatic_two_theta_binning(sample_data)
        binning_data_dict["omega"] = get_automatic_omega_binning(sample_data)
        for key in ["two_theta", "omega"]:
            min_limit = binning_data_dict[key][f"{key}_min"]
            max_limit = binning_data_dict[key][f"{key}_max"]
            self.view._map[f"{key}_min"].setMinimum(min_limit)
            self.view._map[f"{key}_max"].setMinimum(min_limit)
            self.view._map[f"{key}_min"].setMaximum(max_limit)
            self.view._map[f"{key}_max"].setMaximum(max_limit)

    def _get_automatic_binning_is_checked(self):
        return self.view._map["automatic_binning"].isChecked()

    def _get_wavelength_from_data_is_checked(self):
        return self.view._map["get_wavelength"].isChecked()

    def tab_got_focus(self):
        selected_data = self.param_dict["file_selector"]["full_data"]
        if selected_data:
            if self._get_wavelength_from_data_is_checked():
                self._determine_wavelength()
            self._set_binning()
        else:
            if self._get_automatic_binning_is_checked():
                self._set_default_auto_binning_no_data()

    def _set_default_auto_binning_no_data(self):
        keys_to_reset = ["two_theta_min", "two_theta_max", "two_theta_bin_size", "omega_min", "omega_max", "omega_bin_size"]
        for key in keys_to_reset:
            self.view._map[key].setMinimum(0)
            self.view._map[key].setValue(0)

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options["get_wavelength"]:
            self._determine_wavelength()

    def process_commandline_request(self, command_line_options):
        self.view.set_single_state_by_name("use_dx_dy", True)
        for command in ["dx", "dy", "omega_offset", "hkl1", "hkl2", "det_efficiency", "flipping_ratio"]:
            if command in command_line_options:
                self.view.set_single_state_by_name(command, command_line_options[command])

    def get_option_dict(self):
        """
        Return own options from view.
        """
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
            o_dic = self.own_dict
            if not self.own_dict.get("use_dx_dy", False):
                o_dic["dx"], o_dic["dy"] = self.model.get_dx_dy(o_dic)
        return self.own_dict

    def _evaluate_two_theta_bin_size(self):
        self.view._map["two_theta_bin_size"].setMaximum(self.view._map["two_theta_max"].value() - self.view._map["two_theta_min"].value())
        self.view._map["two_theta_bin_size"].setMinimum(0)

    def _evaluate_omega_bin_size(self):
        self.view._map["omega_bin_size"].setMaximum(self.view._map["omega_max"].value() - self.view._map["omega_min"].value())
        self.view._map["omega_bin_size"].setMinimum(0)

    def _attach_signal_slots(self):
        self.view.sig_get_wavelength.connect(self._determine_wavelength)
        self.view.sig_auto_binning_clicked.connect(self._set_binning)
        self.view.sig_two_theta_changed.connect(self._evaluate_two_theta_bin_size)
        self.view.sig_omega_changed.connect(self._evaluate_omega_bin_size)

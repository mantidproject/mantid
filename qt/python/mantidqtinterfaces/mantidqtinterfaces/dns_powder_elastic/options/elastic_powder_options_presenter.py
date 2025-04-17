# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic options tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter import DNSCommonOptionsPresenter
from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_binning import get_automatic_two_theta_binning


class DNSElasticPowderOptionsPresenter(DNSCommonOptionsPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self.view.sig_get_wavelength.connect(self._determine_wavelength)
        self.view.sig_two_theta_min_changed.connect(self._evaluate_max_bin_size)
        self.view.sig_two_theta_max_changed.connect(self._evaluate_max_bin_size)
        self.view.sig_auto_binning_clicked.connect(self._set_manual_binning_lims)
        self.view.sig_auto_binning_clicked.connect(self._set_auto_binning)

    def _set_auto_binning(self):
        """
        Getting binning parameters from selected sample data.
        """
        if self.param_dict["file_selector"]["full_data"]:
            sample_data = self.param_dict["file_selector"]["full_data"]
            two_theta_params_dict = get_automatic_two_theta_binning(sample_data)
            own_options = self.get_option_dict()
            for parameter in two_theta_params_dict.keys():
                own_options[parameter] = two_theta_params_dict[parameter]
            self.set_view_from_param()

    def _set_manual_binning_lims(self):
        if self.param_dict["file_selector"]["full_data"]:
            sample_data = self.param_dict["file_selector"]["full_data"]
            auto_bin_params = get_automatic_two_theta_binning(sample_data)
            min_lower_limit = auto_bin_params["two_theta_min"]
            min_upper_limit = auto_bin_params["two_theta_max"] - auto_bin_params["two_theta_bin_size"]
            max_lower_limit = auto_bin_params["two_theta_min"] + auto_bin_params["two_theta_bin_size"]
            max_upper_limit = auto_bin_params["two_theta_max"]
            self.view._map["two_theta_min"].setMinimum(min_lower_limit)
            self.view._map["two_theta_min"].setMaximum(min_upper_limit)
            self.view._map["two_theta_max"].setMinimum(max_lower_limit)
            self.view._map["two_theta_max"].setMaximum(max_upper_limit)
            self._evaluate_max_bin_size()

    def _get_automatic_binning_state(self):
        return self.view._map["automatic_binning"].isChecked()

    def tab_got_focus(self):
        auto_binning_is_on = self._get_automatic_binning_state()
        if auto_binning_is_on:
            self._set_auto_binning()
        else:
            self._set_manual_binning_lims()

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options["get_wavelength"]:
            self._determine_wavelength()

    def process_commandline_request(self, command_line_options):
        self.view.set_single_state_by_name("use_dx_dy", True)
        for command in ["det_efficiency", "flipping_ratio", "separation_xyz", "separation_coh_inc"]:
            if command in command_line_options:
                self.view.set_single_state_by_name(command, command_line_options[command])

    def _evaluate_max_bin_size(self):
        if self.param_dict["file_selector"]["full_data"] and not self._get_automatic_binning_state():
            own_options = self.get_option_dict()
            two_theta_max = own_options["two_theta_max"]
            two_theta_min = own_options["two_theta_min"]
            self.view._map["two_theta_bin_size"].setMaximum(two_theta_max - two_theta_min)

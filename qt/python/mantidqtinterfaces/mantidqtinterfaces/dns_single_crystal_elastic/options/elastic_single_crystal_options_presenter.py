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


class DNSElasticSCOptionsPresenter(DNSCommonOptionsPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self._attach_signal_slots()

    def _set_auto_two_theta_binning(self):
        if self.param_dict["file_selector"]["full_data"]:
            sample_data = self.param_dict["file_selector"]["full_data"]
            two_theta_params_dict = get_automatic_two_theta_binning(sample_data)
            own_options = self.get_option_dict()
            for parameter in two_theta_params_dict.keys():
                own_options[parameter] = two_theta_params_dict[parameter]
            self.set_view_from_param()

    def _set_auto_omega_binning(self):
        if self.param_dict["file_selector"]["full_data"]:
            sample_data = self.param_dict["file_selector"]["full_data"]
            omega_params_dict = get_automatic_omega_binning(sample_data)
            own_options = self.get_option_dict()
            for parameter in omega_params_dict.keys():
                own_options[parameter] = omega_params_dict[parameter]
            self.set_view_from_param()

    def _get_automatic_binning_state(self):
        return self.view._map["automatic_binning"].isChecked()

    def tab_got_focus(self):
        auto_binning_is_on = self._get_automatic_binning_state()
        if auto_binning_is_on:
            self._set_auto_two_theta_binning()
            self._set_auto_omega_binning()

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options["get_wavelength"]:
            self._determine_wavelength()

    def get_option_dict(self):
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
        return self.own_dict

    def _attach_signal_slots(self):
        self.view.sig_get_wavelength.connect(self._determine_wavelength)
        self.view.sig_auto_binning_clicked.connect(self._set_auto_two_theta_binning)
        self.view.sig_auto_binning_clicked.connect(self._set_auto_omega_binning)


def get_automatic_two_theta_binning(sample_data):
    det_rot = [-x["det_rot"] for x in sample_data]
    two_theta_last_det = 115.0
    two_theta_max = max(det_rot) + two_theta_last_det
    two_theta_min = min(det_rot)
    two_theta_step = 0.5
    number_two_theta_bins = int(round((two_theta_max - two_theta_min) / two_theta_step) + 1)
    two_theta_binning_dict = {
        "two_theta_min": two_theta_min,
        "two_theta_max": two_theta_max,
        "two_theta_bin_size": two_theta_step,
        "nbins": number_two_theta_bins,
    }
    return two_theta_binning_dict


def get_automatic_omega_binning(sample_data):
    omega = [x["sample_rot"] - x["det_rot"] for x in sample_data]
    omega_min = min(omega)
    omega_max = max(omega)
    omega_step = 1.0
    number_omega_bins = int(round((omega_max - omega_min) / omega_step) + 1)
    omega_binning_dict = {"omega_min": omega_min, "omega_max": omega_max, "omega_bin_size": omega_step, "omega_nbins": number_omega_bins}
    return omega_binning_dict

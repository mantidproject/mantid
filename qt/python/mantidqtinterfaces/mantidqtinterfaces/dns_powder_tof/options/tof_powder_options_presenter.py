# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder TOF options tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter import DNSCommonOptionsPresenter


class DNSTofPowderOptionsPresenter(DNSCommonOptionsPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self.view.sig_get_wavelength.connect(self._determine_wavelength)
        self.view.sig_estimate_q_and_binning.connect(self._estimate_q_and_binning)

    def _estimate_q_and_binning(self):
        """
        Estimation of q and ideal binning based on selected sample data.
        """
        own_options = self.get_option_dict()
        if own_options["get_wavelength"]:
            if self._determine_wavelength() is None:
                return False
        wavelength = own_options["wavelength"]
        full_data = self.param_dict["file_selector"]["full_data"]
        if not full_data:
            self.raise_error("No data selected", critical=True)
            return False
        binning, errors = self.model.estimate_q_and_binning(full_data, wavelength)
        self.raise_error(
            f"Warning different channel_widths {errors['channel_widths']} in selected datafiles.", do_raise=errors["chan_error"]
        )
        self.raise_error(
            f"Waning different number of tof channels {errors['tof_channels']} in selected datafiles.", do_raise=errors["tof_error"]
        )
        for key, value in binning.items():
            own_options[key] = value
        self.set_view_from_param()
        return True

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options["get_wavelength"]:
            self._determine_wavelength()
        if own_options["dE_step"] == 0 or own_options["q_step"] == 0:
            self._estimate_q_and_binning()
            self.view.show_status_message("q-range and binning automatically estimated", 30)

    def process_commandline_request(self, com_line_options):
        for command in ["det_efficiency"]:
            if command in com_line_options:
                self.view.set_single_state_by_name(command, com_line_options[command])

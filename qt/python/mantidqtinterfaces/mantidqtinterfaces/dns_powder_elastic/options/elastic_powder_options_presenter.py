# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic options tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter \
    import DNSCommonOptionsPresenter
from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_elastic_powder_dataset \
    import automatic_two_theta_binning


class DNSElasticPowderOptionsPresenter(DNSCommonOptionsPresenter):

    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self.view.sig_get_wavelength.connect(self._determine_wavelength)
        self.view.sig_auto_binning_clicked.connect(self._set_automatic_binning)

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self._determine_wavelength()

    def process_commandline_request(self, command_line_options):
        self.view.set_single_state_by_name('use_dx_dy', True)
        for command in [
            'det_efficiency', 'flipping_ratio', 'separation_xyz',
            'separation_coh_inc'
        ]:
            if command in command_line_options:
                self.view.set_single_state_by_name(
                    command, command_line_options[command]
                )

    def _set_automatic_binning(self):
        """
        Getting binning parameters from selected sample data.
        """
        if self.param_dict['file_selector']['full_data']:
            sample_data = self.param_dict['file_selector']['full_data']
            binning_dict = automatic_two_theta_binning(sample_data)

            two_theta_min = binning_dict['min']
            two_theta_max = binning_dict['max']
            two_theta_bin_size = 0.5
            number_bins = (two_theta_max - two_theta_min) / two_theta_bin_size + 1

            own_options = self.get_option_dict()
            own_options['two_theta_min'] = two_theta_min
            own_options['two_theta_max'] = two_theta_max
            own_options['nbins'] = number_bins
            own_options['two_theta_bin_size'] = two_theta_bin_size
            self.set_view_from_param()

    def _get_automatic_binning_state(self):
        return self.view._map['automatic_binning'].isChecked()

    def tab_got_focus(self):
        auto_binning_is_on = self._get_automatic_binning_state()
        if auto_binning_is_on:
            self._set_automatic_binning()

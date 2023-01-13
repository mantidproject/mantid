# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic options tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter \
    import DNSCommonOptionsPresenter


class DNSElasticSCOptionsPresenter(DNSCommonOptionsPresenter):
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
        if self.param_dict['file_selector']['full_data']:
            sample_data = self.param_dict['file_selector']['full_data']
            two_theta_params_dict = get_automatic_two_theta_binning(sample_data)
            own_options = self.get_option_dict()
            for parameter in two_theta_params_dict.keys():
                own_options[parameter] = two_theta_params_dict[parameter]
            self.set_view_from_param()

    def _set_manual_binning_lims(self):
        if self.param_dict['file_selector']['full_data']:
            sample_data = self.param_dict['file_selector']['full_data']
            auto_bin_params = get_automatic_two_theta_binning(sample_data)
            min_lower_limit = auto_bin_params['two_theta_min']
            min_upper_limit = auto_bin_params['two_theta_max'] - auto_bin_params['two_theta_bin_size']
            max_lower_limit = auto_bin_params['two_theta_min'] + auto_bin_params['two_theta_bin_size']
            max_upper_limit = auto_bin_params['two_theta_max']
            self.view._map['two_theta_min'].setMinimum(min_lower_limit )
            self.view._map['two_theta_min'].setMaximum(min_upper_limit)
            self.view._map['two_theta_max'].setMinimum(max_lower_limit)
            self.view._map['two_theta_max'].setMaximum(max_upper_limit)
            self._evaluate_max_bin_size()

    def _get_automatic_binning_state(self):
        return self.view._map['automatic_binning'].isChecked()

    def tab_got_focus(self):
        auto_binning_is_on = self._get_automatic_binning_state()
        if auto_binning_is_on:
            self._set_auto_binning()
        else:
            self._set_manual_binning_lims()

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self._determine_wavelength()

    def process_commandline_request(self, cloptions):
        self.view.set_single_state_by_name('use_dx_dy', True)
        for command in [
            'dx', 'dy', 'omega_offset', 'hkl1', 'hkl2', 'det_efficiency',
            'flipping_ratio'
        ]:
            if command in cloptions:
                self.view.set_single_state_by_name(command, cloptions[command])

    def get_option_dict(self):
        """Return own options from view"""
        if self.view is not None:
            self.own_dict.update(self.view.get_state())
            odic = self.own_dict
            odic['hkl1'] = self.model.convert_hkl_string(odic['hkl1'])
            odic['hkl2'] = self.model.convert_hkl_string(odic['hkl2'])
            if not self.own_dict.get('use_dx_dy', False):
                odic['dx'], odic['dy'] = self.model.get_dx_dy(odic)
        return self.own_dict

    def _evaluate_max_bin_size(self):
        if self.param_dict['file_selector']['full_data'] and not self._get_automatic_binning_state():
            own_options = self.get_option_dict()
            two_theta_max = own_options['two_theta_max']
            two_theta_min = own_options['two_theta_min']
            self.view._map['two_theta_bin_size'].setMaximum(two_theta_max - two_theta_min)


def get_automatic_two_theta_binning(sample_data):
    '''
    Determines automatic two theta binning parameters from selected sample data.
    '''
    det_rot = [-x['det_rot'] for x in sample_data]
    two_theta_last_det = 115.0
    two_theta_max = (max(det_rot) + two_theta_last_det)
    two_theta_min = (min(det_rot))
    two_theta_step = 0.5
    number_two_theta_bins = int(round((two_theta_max - two_theta_min) / two_theta_step) + 1)
    two_theta_binning_dict = {'two_theta_min': two_theta_min,
                              'two_theta_max': two_theta_max,
                              'two_theta_bin_size': two_theta_step,
                              'nbins': number_two_theta_bins}
    return two_theta_binning_dict

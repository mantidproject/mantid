# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Options Presenter - Tab of DNS Reduction GUI
"""

from mantidqtinterfaces.dns_powder_tof.options.common_options_presenter\
    import DNSCommonOptionsPresenter


class DNSElasticPowderOptionsPresenter(DNSCommonOptionsPresenter):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self.view.sig_get_wavelength.connect(self._determine_wavelength)

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self._determine_wavelength()

    def process_commandline_request(self, cloptions):
        self.view.set_single_state_by_name('use_dx_dy', True)
        for command in [
                'det_efficency', 'flipping_ratio', 'separation_xyz',
                'separation_coh_inc'
        ]:
            if command in cloptions:
                self.view.set_single_state_by_name(command, cloptions[command])

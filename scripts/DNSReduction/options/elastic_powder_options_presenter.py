# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Options Presenter - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.options.elastic_powder_options_view import DNSElasticPowderOptions_view


class DNSElasticPowderOptions_presenter(DNSObserver):
    def __init__(self, parent):
        super(DNSElasticPowderOptions_presenter,
              self).__init__(parent, 'options')
        self.name = 'elastic_powder_options'
        self.view = DNSElasticPowderOptions_view(self.parent.view)

        ## connect signals
        self.view.sig_get_wavelength.connect(self.get_wavelength)

    def get_wavelength(self):
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return None
        wavelengths = [x['wavelength'] for x in fulldata]
        selector_speeds = [x['selector_speed'] for x in fulldata]
        if len(set(selector_speeds)) > 1:
            self.raise_error('Warning, different selector speeds in datafiles')
        elif len(set(wavelengths)) > 1:
            self.raise_error('Warning, different wavelengths in datafiles')
        else:
            wavelength = wavelengths[0] / 10
            if selector_speeds[0] == 0:
                selector_wavelength = 1000
            else:
                selector_wavelength = 1 / selector_speeds[0] * 4.448 / 7500
            if abs(selector_wavelength - wavelength) > 0.1 * wavelength:
                self.raise_error(
                    'Warning, selector speed differs from wavelength'
                    ' more than 10%, set wavelength manually.')
                self.view.deactivate_get_wavelength()
            else:
                own_options = self.get_option_dict()
                own_options['wavelength'] = wavelength
                self.set_view_from_param()
        return wavelength

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self.get_wavelength()

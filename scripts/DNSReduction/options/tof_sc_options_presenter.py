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
from DNSReduction.options.tof_sc_options_view import DNSTofScOptions_view


class DNSTofScOptions_presenter(DNSObserver):
    def __init__(self, parent):
        super(DNSTofScOptions_presenter, self).__init__(parent, 'options')
        self.name = 'tof_sc_options'
        self.view = DNSTofScOptions_view(self.parent.view)
        self.view.sig_get_wavelength.connect(self.get_wavelength)

    def get_wavelength(self):
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return None
        wavelengths = [x['wavelength'] for x in fulldata]
        if len(set(wavelengths)) > 1:
            self.raise_error('Warning, different wavelengths in datafiles')
        else:
            own_options = self.get_option_dict()
            own_options['wavelength'] = wavelengths[0]
            self.set_view_from_param()
        return wavelengths[0]

    def process_request(self):
        self.get_option_dict()

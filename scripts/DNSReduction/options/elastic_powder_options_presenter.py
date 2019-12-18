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

from DNSReduction.options.elastic_powder_options_view import DNSElasticPowderOptions_view
from DNSReduction.options.common_options_presenter import DNSCommonOptions_presenter


class DNSElasticPowderOptions_presenter(DNSCommonOptions_presenter):
    def __init__(self, parent):
        super(DNSElasticPowderOptions_presenter,
              self).__init__(parent, 'options')
        self.name = 'elastic_powder_options'
        self.view = DNSElasticPowderOptions_view(self.parent.view)

        ## connect signals
        self.view.sig_get_wavelength.connect(self.get_wavelength)

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self.determine_wavelength()

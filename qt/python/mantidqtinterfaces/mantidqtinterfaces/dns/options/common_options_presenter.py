# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""

from mantidqtinterfaces.dns.data_structures.dns_observer import DNSObserver


class DNSCommonOptionsPresenter(DNSObserver):

    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)

    def _determine_wavelength(self):
        """
        getting wavelength from selected DNSFiles, checks for deviations
        """
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return None
        warnings = {
            'wavelength_varies': 'Warning, different wavelengths in'
            'datafiles',
            'selector_wavelength_missmatch': 'Warning, selector speeds in'
            ' datafiles '
            'differ by more than 10 rpm.',
            'selector_speed_varies': 'Warning, selector speed differs from'
            ' wavelength more'
            ' than 10%, set wavelength manually. ',
        }
        wavelength, errors = self.model.determine_wavelength(fulldata)
        for key, value in errors.items():
            self.raise_error(warnings[key], doraise=value)
        if any(errors.values()):
            self.view.deactivate_get_wavelength()
        else:
            own_options = self.get_option_dict()
            own_options['wavelength'] = wavelength
            self.set_view_from_param()
        return wavelength

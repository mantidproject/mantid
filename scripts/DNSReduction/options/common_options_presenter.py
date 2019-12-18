# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.data_structures.dns_observer import DNSObserver


class DNSCommonOptions_presenter(DNSObserver):
    def __init__(self, parent, name):
        super(DNSCommonOptions_presenter, self).__init__(parent, name)

    def get_det_rot_min_max(self, fulldata):
        det_rot = [x['det_rot'] for x in fulldata]
        det_rot_max = -min(det_rot)
        det_rot_min = -max(det_rot)
        return [det_rot_min, det_rot_max]

    def selector_wavelength(self, selector_speed):
        if selector_speed == 0:
            return 1000
        return 1 / selector_speed * 4.448 / 7500

    def get_selector_speed(self, fulldata):
        selector_speeds = [x['selector_speed'] for x in fulldata]
        if len(set(selector_speeds)) > 1:
            self.raise_error('Warning, different selector speeds in datafiles')
        return selector_speeds[0]

    def get_wavelength(self, fulldata):
        wavelengths = [x['wavelength'] for x in fulldata]
        if len(set(wavelengths)) > 1:
            self.raise_error('Warning, different wavelengths in datafiles')
        return wavelengths[0] / 10

    def determine_wavelength(self):
        """
        getting wavelength from selected DNSFiles, cheks for deviations
        """
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return None
        wavelength = self.get_wavelength(fulldata)
        selector_speed = self.get_selector_speed(fulldata)
        selector_wavelength = self.selector_wavelength(selector_speed)
        if abs(selector_wavelength - wavelength) > 0.1 * wavelength:
            self.raise_error(
                'Warning, selector speed differs from wavelength more'
                ' than 10%, set wavelength manually.')
            self.view.deactivate_get_wavelength()
        else:
            own_options = self.get_option_dict()
            own_options['wavelength'] = wavelength
            self.set_view_from_param()
        return wavelength

# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder TOF options tab common model of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel


class DNSCommonOptionsModel(DNSObsModel):

    @staticmethod
    def get_det_rot_min_max(full_data):
        det_rot = [x['det_rot'] for x in full_data]
        det_rot_max = -min(det_rot)
        det_rot_min = -max(det_rot)
        return [det_rot_min, det_rot_max]

    @staticmethod
    def determine_wavelength(full_data):
        """
        Getting wavelength from selected DNSFiles, checks for deviations.
        """
        wavelengths = _get_wavelengths(full_data)
        selector_speeds = _get_selector_speeds(full_data)
        selector_wavelength = _selector_wavelength(selector_speeds[0])
        errors = {
            'wavelength_varies': _wavelength_varies(wavelengths),
            'selector_wavelength_missmatch': _selector_wavelength_missmatch(
                selector_wavelength, wavelengths[0]),
            'selector_speed_varies': _selector_speed_varies(selector_speeds)
        }

        return [wavelengths[0], errors]


def _selector_wavelength(selector_speed):
    if selector_speed == 0:
        return 1000
    return 1.0 / selector_speed * 4.448 * 7500


def _get_selector_speeds(full_data):
    return [x['selector_speed'] for x in full_data]


def _selector_speed_varies(selector_speeds):
    return any((abs(x - selector_speeds[0]) > 10 for x in selector_speeds))


def _get_wavelengths(full_data):
    return [x['wavelength'] / 10.0 for x in full_data]


def _wavelength_varies(wavelengths):
    return len(set(wavelengths)) > 1


def _selector_wavelength_missmatch(selector_wavelength, wavelength):
    return abs(selector_wavelength - wavelength) > 0.1 * wavelength

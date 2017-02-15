""" The SANSConfigurations class holds instrument-specific configs to centralize instrument-specific magic numbers"""
# pylint: disable=too-few-public-methods

from __future__ import (absolute_import, division, print_function)


class Configurations(object):

    class LARMOR(object):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 0.5
        wavelength_full_range_high = 13.5

    class SANS2D(object):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 2.0
        wavelength_full_range_high = 14.0

    class LOQ(object):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 2.2
        wavelength_full_range_high = 10.0

        # The default prompt peak range for LOQ
        prompt_peak_correction_min = 19000.0
        prompt_peak_correction_max = 20500.0

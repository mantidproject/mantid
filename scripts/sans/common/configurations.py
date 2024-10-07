# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The SANSConfigurations class holds instrument-specific configs to centralize instrument-specific magic numbers"""

# pylint: disable=too-few-public-methods
from abc import ABCMeta, abstractmethod


class Configurations(object):
    class _EssentialAttrs(metaclass=ABCMeta):
        """
        Interface for essential properties all config classes should have
        """

        @property
        @abstractmethod
        def wavelength_full_range_low(self):
            pass

        @property
        @abstractmethod
        def wavelength_full_range_high(self):
            pass

        # Default monitors are for backwards compatibility with legacy user files
        @property
        @abstractmethod
        def default_incident_monitor(self):
            pass

        @property
        @abstractmethod
        def default_transmission_monitor(self):
            pass

    class LARMOR(_EssentialAttrs):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 0.5
        wavelength_full_range_high = 13.5

        default_incident_monitor = 2
        default_transmission_monitor = 3

    class SANS2D(_EssentialAttrs):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 2.0
        wavelength_full_range_high = 14.0

        default_incident_monitor = 2
        default_transmission_monitor = 3

    class LOQ(_EssentialAttrs):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 2.2
        wavelength_full_range_high = 10.0

        default_incident_monitor = 2
        default_transmission_monitor = 3

        # The default prompt peak range for LOQ
        prompt_peak_correction_min = 19000.0
        prompt_peak_correction_max = 20500.0

    class ZOOM(_EssentialAttrs):
        # The full wavelength range of the instrument
        wavelength_full_range_low = 1.75
        wavelength_full_range_high = 16.5

        default_incident_monitor = 3
        default_transmission_monitor = 4

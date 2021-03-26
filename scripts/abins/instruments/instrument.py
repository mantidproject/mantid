# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# noinspection PyPep8Naming
import logging
from typing import Union

import mantid.kernel

Logger = Union[logging.Logger, mantid.kernel.Logger]


class Instrument:
    _name = None

    def __init__(self, setting: str = ''):
        setting = self._check_setting(setting)
        self._setting = setting

    def get_angles(self):
        """Get a list of detector angles to sample"""
        raise NotImplementedError()

    def get_setting(self):
        return self._setting

    def calculate_q_powder(self, *, input_data=None, angle=None):
        """
        Calculates Q2.


        :param  input_data: data from which Q2 should be calculated
        :param angle: Detector angle in degrees

        :returns:  numpy array with Q2 data

        """
        raise NotImplementedError()

    def convolve_with_resolution_function(self, frequencies=None, bins=None, s_dft=None, scheme='auto'):
        """
        Convolves discrete spectrum with the resolution function for the particular instrument.

        :param frequencies: frequencies for which resolution function should be calculated (frequencies in cm-1)
        :type frequencies: 1D array-like
        :param bins: Bin edges for output histogram. Most broadening implementations expect this to be regularly-spaced.
        :type bins: 1D array-like
        :param s_dft: discrete S calculated directly from DFT
        :type s_dft: 1D array-like
        :param scheme: Broadening scheme. Multiple implementations are available in *Instruments.Broadening* that trade-
            off between speed and accuracy. Not all schemes must (or should?) be implemented for all instruments, but
            'auto' should select something sensible.
        :type scheme: str
        """
        raise NotImplementedError()

    def set_detector_angle(self, angle=None):
        """
        Setter for detector angle.
        :param angle: new angle of detector
        """
        if isinstance(angle, float):
            self._angle = angle
        else:
            raise ValueError("Invalid value of a detector angle (%s, type(%s) = %s; should be float)." %
                             (angle, angle, type(angle)))

    def __str__(self):
        return self._name

    def get_name(self):
        return self._name

    def _check_setting(self, setting: str, logger: Logger = None) -> str:
        """Sanity-check that setting is appropriate for selected instrument

        Return an appropriate value for setting: for example, '' may be set
        to 'default' if appropriate, and casing may be corrected.

        Args:
            setting:
                setting selection. The empty string '' is conventional for a
                default/unselected instrument setting. Settings should generally
                correspond to entries in abins.parameters.
            logger:
                logger used for warning messages. Defaults to the mantid
                logger; this option is mostly for testing.

        Returns:
            A "laundered" string for  data lookup from abins.parameters

        Raises:
            ValueError
                When the selected setting is not supported for the current instrument.

        """
        import abins.parameters

        if logger is None:
            mantid_logger: mantid.kernel.Logger = mantid.kernel.logger
            logger = mantid_logger

        if self.get_name() not in abins.parameters.instruments:
            # If an instrument lacks an entry in abins.parameters, we cannot
            # reason about how appropriate the setting is; assume good.
            return setting

        else:
            parameters = abins.parameters.instruments.get(self.get_name())

            if 'settings' not in parameters:
                if setting != '':
                    logger.warning(f'Instrument "{self.get_name()}" does not use settings. '
                                   f'Ignoring selected setting "{setting}".')
                return ''

            if setting == '':
                if 'settings_default' in parameters:
                    default = parameters['settings_default']
                    logger.notice(f'Using default setting "{default}" for instrument "{self.get_name()}"')
                    return default
                raise ValueError(f'{self.get_name()} instrument does not have a default setting, ' +
                                 'and no setting was specified. Accepted settings: ' +
                                 ', '.join(parameters['settings'].keys()))

            downcased_settings = {s.lower(): s for s in parameters['settings']}
            if setting.lower() in downcased_settings:
                return downcased_settings[setting.lower()]

            else:
                raise ValueError(f'Setting "{setting}" is unknown for instrument {self.get_name()}. '
                                 'Supported settings: ' + ', '.join(sorted(parameters['settings'].keys())))

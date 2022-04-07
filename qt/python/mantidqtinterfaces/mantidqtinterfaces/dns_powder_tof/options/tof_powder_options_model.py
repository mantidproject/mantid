# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""

from mantidqtinterfaces.dns_powder_tof.helpers.converters import (
    lambda_to_energy, twotheta_to_q)
from mantidqtinterfaces.dns_powder_tof.options.common_options_model import \
    DNSCommonOptionsModel


class DNSTofPowderOptionsModel(DNSCommonOptionsModel):
    def estimate_q_and_binning(self, full_data, wavelength):
        """
        Estimation of q and ideal binning based on selected sample data.
        """
        channel_widths, chan_error = get_channelwidths(full_data)
        tof_channels, tof_error = get_tofchannels(full_data)
        det_rot_min, det_rot_max = self.get_det_rot_min_max(full_data)
        d_e = lambda_to_energy(wavelength)
        binning = {
            'dEmin': -d_e + 0.5,
            'dEmax': d_e - 0.5,
            'dEstep': 2 * d_e / max(tof_channels) * 10,
            # in principle *10 should not be done, prevents empty bins
            'qmax': twotheta_to_q(det_rot_max + 115, wavelength, -d_e),
            'qmin': twotheta_to_q(det_rot_min, wavelength, 0),
            'qstep': 0.025,  # anyhow, linear steps not good
        }
        errors = {
            'channelwidths': channel_widths,
            'chan_error': chan_error,
            'tofchannels': tof_channels,
            'tof_error': tof_error
        }
        return [binning, errors]


def get_tofchannels(full_data):
    tof_channels = [x['tofchannels'] for x in full_data]
    error = number_of_tof_channels_varies(tof_channels)
    return [tof_channels, error]


def get_channelwidths(full_data):
    channel_widths = [x['channelwidth'] for x in full_data]
    error = channelwidth_varies(channel_widths)
    return [channel_widths, error]


def channelwidth_varies(channel_widths):
    return len(set(channel_widths)) != 1


def number_of_tof_channels_varies(tof_channels):
    return len(set(tof_channels)) != 1

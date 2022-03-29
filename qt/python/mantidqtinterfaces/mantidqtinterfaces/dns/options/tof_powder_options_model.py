# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""

from mantidqtinterfaces.dns.helpers.converters import (
    lambda_to_energy, twotheta_to_q)
from mantidqtinterfaces.dns.options.common_options_model import \
    DNSCommonOptionsModel


class DNSTofPowderOptionsModel(DNSCommonOptionsModel):

    def estimate_q_and_binning(self, fulldata, wavelength):
        """
        Estimation of q and ideal binning based on selected sample data
        """
        channelwidths, chan_error = get_channelwidths(fulldata)
        tofchannels, tof_error = get_tofchannels(fulldata)
        det_rot_min, det_rot_max = self.get_det_rot_min_max(fulldata)
        d_e = lambda_to_energy(wavelength)
        binning = {
            'dEmin': -d_e + 0.5,
            'dEmax': d_e - 0.5,
            'dEstep': 2 * d_e / max(tofchannels) * 10,
            # in principle *10 should not be done, prevents empty bins
            'qmax': twotheta_to_q(det_rot_max + 115, wavelength, -d_e),
            'qmin': twotheta_to_q(det_rot_min, wavelength, 0),
            'qstep': 0.025,  # anyhow linear steps not good
        }
        errors = {
            'channelwidths': channelwidths,
            'chan_error': chan_error,
            'tofchannels': tofchannels,
            'tof_error': tof_error
        }
        return [binning, errors]


def get_tofchannels(fulldata):
    tofchannels = [x['tofchannels'] for x in fulldata]
    error = number_of_tof_channels_varies(tofchannels)
    return [tofchannels, error]


def get_channelwidths(fulldata):
    channelwidths = [x['channelwidth'] for x in fulldata]
    error = channelwidth_varies(channelwidths)
    return [channelwidths, error]


def channelwidth_varies(channelwidths):
    return len(set(channelwidths)) != 1


def number_of_tof_channels_varies(tofchannels):
    return len(set(tofchannels)) != 1

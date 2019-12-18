# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS TOF powder Options Presenter - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.options.tof_powder_options_view import DNSTofPowderOptions_view
from DNSReduction.helpers.converters import twotheta_to_q, v_to_E
from DNSReduction.options.common_options_presenter import DNSCommonOptions_presenter


class DNSTofPowderOptions_presenter(DNSCommonOptions_presenter):
    def __init__(self, parent):
        super(DNSTofPowderOptions_presenter, self).__init__(parent, 'options')
        self.name = 'tof_powder_options'
        self.view = DNSTofPowderOptions_view(self.parent.view)

        ## connect signals
        self.view.sig_get_wavelength.connect(self.get_wavelength)
        self.view.sig_estimate_q_and_binning.connect(
            self.estimate_q_and_binning)

    def get_channelwidth(self, fulldata):
        channelwidth = [x['channelwidth'] for x in fulldata]
        if len(set(channelwidth)) != 1:
            self.raise_error(
                'Waning different channelwidths in selected datafiles: '
                '{}'.format(channelwidth))
        return channelwidth[0]

    def get_tofchannels(self, fulldata):
        tofchannels = [x['tofchannels'] for x in fulldata]
        if len(set(tofchannels)) != 1:
            self.raise_error(
                'Waning different number of tofchannels in '
                'selected datafiles: {}'.format(tofchannels))
        return tofchannels[0]

    def get_det_rot_min_max(self, fulldata):
        det_rot = [x['det_rot'] for x in fulldata]
        det_rot_max = -min(det_rot)
        det_rot_min = -max(det_rot)
        return [det_rot_min, det_rot_max]

    def tof_to_dE(self, channelwidth, tofchannels):
        velocity = 0.85 * 1000000 / channelwidth / tofchannels
        return v_to_E(velocity)

    def estimate_q_and_binning(self):
        """
        Estimation of q and ideal binning based on selected sample data
        """
        own_options = self.get_option_dict()
        wavelength = own_options['wavelength']
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return
        channelwidth = self.get_channelwidth(fulldata)
        tofchannels = self.get_tofchannels(fulldata)
        dE = self.tof_to_dE(channelwidth, tofchannels)
        det_rot_min, det_rot_max = self.get_det_rot_min_max(fulldata)
        own_options['dEmin'] = -dE
        own_options['dEmax'] = dE
        # in principle *10 should not be done, prevents empty bins
        own_options['dEstep'] = 2*dE / tofchannels * 10  #
        own_options['qmax'] = twotheta_to_q(det_rot_max + 115, wavelength, -dE)
        own_options['qmin'] = twotheta_to_q(det_rot_min, wavelength, 0)
        own_options['qstep'] = 0.025  ## anyhow linear steps not good
        self.set_view_from_param()
        return

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

    def process_request(self):
        own_options = self.get_option_dict()
        if own_options['get_wavelength']:
            self.determine_wavelength()
        if own_options['dEstep'] == 0 or own_options['qstep'] == 0:
            self.estimate_q_and_binning()
            self.view.show_statusmessage(
                'q-range and binning automatically estimated', 30)

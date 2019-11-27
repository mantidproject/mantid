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

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.options.tof_powder_options_view import DNSTofPowderOptions_view
from DNSReduction.helpers.converters import twotheta_to_q, v_to_E


class DNSTofPowderOptions_presenter(DNSObserver):
    def __init__(self, parent):
        super(DNSTofPowderOptions_presenter, self).__init__(parent, 'options')
        self.name = 'tof_powder_options'
        self.view = DNSTofPowderOptions_view(self.parent.view)

        ## connect signals
        self.view.sig_get_wavelength.connect(self.get_wavelength)
        self.view.sig_estimate_q_and_binning.connect(
            self.estimate_q_and_binning)

    def estimate_q_and_binning(self):
        """
        Estimation of q nad ideal binning based on selected sample data
        """
        own_options = self.get_option_dict()
        wavelength = own_options['wavelength']
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return
        det_rot = [x['det_rot'] for x in fulldata]
        channelwidth = [x['channelwidth'] for x in fulldata]
        tofchannels = [x['tofchannels'] for x in fulldata]
        if len(set(channelwidth)) != 1:
            self.raise_error(
                'Waning different channelwidths in selected datafiles: {}'.
                format(str(channelwidth)))
        if len(set(tofchannels)) != 1:
            self.raise_error(
                'Waning different number of tofchannels in ' \
                'selected datafiles: {}'
                .format(str(tofchannels)))
        channelwidth = channelwidth[0]
        tofchannels = tofchannels[0]
        velocity = 0.85 * 1000000 / channelwidth / tofchannels
        dE = v_to_E(velocity)
        det_rot_max = -min(det_rot)
        det_rot_min = -max(det_rot)
        own_options['dEmin'] = -dE
        own_options['dEmax'] = dE
        # in principle *10 should not be done, prevents empty bins
        own_options['dEstep'] = 2 * dE / tofchannels * 10  #
        own_options['qmax'] = twotheta_to_q(det_rot_max + 115, wavelength, -dE)
        own_options['qmin'] = twotheta_to_q(det_rot_min, wavelength, 0)
        own_options['qstep'] = 0.025  ## anyhow linear steps not good
        self.set_view_from_param()
        return

    def get_wavelength(self):
        """
        getting wavelength from selected DNSFiles, cheks for deviations
        """
        fulldata = self.param_dict['file_selector']['full_data']
        if not fulldata:
            self.raise_error('no data selected', critical=True)
            return None
        wavelengths = [x['wavelength'] for x in fulldata]
        selector_speeds = [x['selector_speed'] for x in fulldata]
        if len(set(selector_speeds)) > 1:
            self.raise_error('Warning, different selector speeds in datafiles')
        elif len(set(wavelengths)) > 1:
            self.raise_error('Warning, different wavelengths in datafiles')
        else:
            wavelength = wavelengths[0] / 10
            if selector_speeds[0] == 0:
                selector_wavelength = 1000
            else:
                selector_wavelength = 1 / selector_speeds[0] * 4.448 / 7500
            if abs(selector_wavelength - wavelength) > 0.1 * wavelength:
                self.raise_error(
                    'Warning, selector speed differs from wavelength more' +
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
            self.get_wavelength()
        if own_options['dEstep'] == 0 or own_options['qstep'] == 0:
            self.estimate_q_and_binning()
            self.view.show_statusmessage(
                'q-range and binning automatically estimated', 30)

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS Options Widget = View - Tab of DNS Reduction GUI.
"""

from qtpy.QtCore import Signal

from mantidqt.utils.qt import load_ui

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSElasticPowderOptionsView(DNSView):
    """
    Widget that lets user select reduction options.
    """
    NAME = "Options"

    def __init__(self, parent):
        super().__init__(parent)
        self._content = load_ui(__file__,
                                'elastic_powder_options.ui',
                                baseinstance=self)
        self._map = {
            # 'multiple_scattering': self._content.dSB_multiple_scattering,
            'norm_time': self._content.rB_norm_time,
            'separation_coh_inc': self._content.rB_separation_coh_inc,
            # 'sum_vana_det_pos': self._content.cB_sum_vana_det_pos,
            'sum_vana_sf_nsf': self._content.cB_sum_vana_sf_nsf,
            'ignore_vana_fields': self._content.cB_ignore_vana_fields,
            'separation': self._content.gB_separation,
            'det_efficiency': self._content.cB_det_efficiency,
            'flipping_ratio': self._content.cB_flipping_ratio,
            'separation_xyz': self._content.rB_separation_xyz,
            'subtract_background_from_sample':
                self._content.cB_subtract_background_from_sample,
            'corrections': self._content.gB_corrections,
            'background_factor': self._content.dSB_background_factor,
            'norm_monitor': self._content.rB_norm_monitor,
            'wavelength': self._content.dSB_wavelength,
            'get_wavelength': self._content.cB_get_wavelength,
            'ttheta_min': self._content.dSB_two_theta_min,
            'ttheta_max': self._content.dSB_two_theta_max,
        }

        # connect signals
        self._map['get_wavelength'].stateChanged.connect(
            self._get_wavelength)
        self._map['det_efficiency'].stateChanged.connect(
            self._disable_sub_det_efficiency)
        self._map['subtract_background_from_sample'].stateChanged.connect(
            self._disable_sub_sample_back)

    # signals
    sig_get_wavelength = Signal()

    def _disable_sub_det_efficiency(self, state):
        self._map['ignore_vana_fields'].setEnabled(state)
        self._map['sum_vana_sf_nsf'].setEnabled(state)
        # self._map['sum_vana_det_pos'].setEnabled(state)

    def _disable_sub_sample_back(self, state):
        self._map['background_factor'].setEnabled(state)

    def deactivate_get_wavelength(self):
        self._map['get_wavelength'].setCheckState(0)

    def _get_wavelength(self, state):
        if state:
            self.sig_get_wavelength.emit()

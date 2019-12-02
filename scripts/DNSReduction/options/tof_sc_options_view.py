# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Options Widget = View - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import Signal

from DNSReduction.data_structures.dns_view import DNSView

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui


class DNSTofScOptions_view(DNSView):
    """
        Widget that lets user select redcution options
    """
    name = "Options"

    def __init__(self, parent):
        super(DNSTofScOptions_view, self).__init__(parent)
        self._content = load_ui(__file__,
                                'tof_sc_options.ui',
                                baseinstance=self)
        self._mapping = {
            'dEbins': self._content.sB_dEbins,
            'omega_offset': self._content.dSB_omega_offset,
            'wavelength': self._content.dSB_wavelength,
            'substract_background_from_vanadium':
            self._content.cB_substract_background_from_vanadium,
            'lmin': self._content.dSB_lmin,
            'monitor_normalization': self._content.cB_monitor_normalization,
            'dEmin': self._content.dSB_dEmin,
            'kmin': self._content.dSB_kmin,
            'kbins': self._content.sB_kbins,
            'vanadium_temperature': self._content.dSB_vanadium_temperature,
            'lbins': self._content.sB_lbins,
            'epp_channel': self._content.SB_epp_channel,
            'correct_elastic_peak_position':
            self._content.cB_correct_elastic_peak_position,
            'beta': self._content.dSB_beta,
            'kmax': self._content.dSB_kmax,
            'dEmax': self._content.dSB_dEmax,
            'alpha': self._content.dSB_alpha,
            'det_efficency': self._content.cB_det_efficency,
            'substract_background': self._content.cB_substract_background,
            'mask_bad_detectors': self._content.cB_mask_bad_detectors,
            'a': self._content.dSB_a,
            'c': self._content.dSB_c,
            'b': self._content.dSBl_b,
            'hmax': self._content.dSB_hmax,
            'hbins': self._content.sB_hbins,
            'hmin': self._content.dSB_hmin,
            'lmax': self._content.dSB_lmax,
            'u': self._content.lE_u,
            'v': self._content.lE_v,
            'corrections': self._content.gB_corrections,
            'delete_raw': self._content.cB_delete_raw,
            'gamma': self._content.dSB_gamma,
            'background_factor': self._content.dSB_background_factor
        }
        self._content.pB_get_wavelength.clicked.connect(self.get_wavelength)
        self._content.pB_estimate.clicked.connect(self.estimate_q_and_binning)

    sig_get_wavelength = Signal()
    sig_estimate_q_and_binning = Signal()

    def get_wavelength(self):
        self.sig_get_wavelength.emit()

    def estimate_q_and_binning(self):
        self.sig_estimate_q_and_binning.emit()

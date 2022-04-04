# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Options Widget = View - Tab of DNS Reduction GUI
"""

from mantidqt.utils.qt import load_ui
from qtpy.QtCore import Signal
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSTofPowderOptionsView(DNSView):
    """
        Widget that lets user select redcution options for TOF powder
    """
    NAME = "Options"

    def __init__(self, parent):
        super().__init__(parent)
        self._content = load_ui(__file__,
                                'tof_powder_options.ui',
                                baseinstance=self)
        self._map = {
            'vanadium_temperature': self._content.dSB_vanadium_temperature,
            'dEmin': self._content.dSB_dEmin,
            'correct_elastic_peak_position': self._content.
            cB_correct_elastic_peak_position,
            'qmax': self._content.dSB_qmax,
            'epp_channel': self._content.SB_epp_channel,
            'qmin': self._content.dSB_qmin,
            'dEmax': self._content.dSB_dEmax,
            'dEstep': self._content.dSB_dEstep,
            'corrections': self._content.gB_corrections,
            'wavelength': self._content.dSB_wavelength,
            'det_efficency': self._content.cB_det_efficency,
            'delete_raw': self._content.cB_delete_raw,
            'norm_monitor': self._content.rB_norm_monitor,
            'qstep': self._content.dSB_qstep,
            'substract_sample_back': self._content.cB_substract_sample_back,
            'substract_vana_back': self._content.cB_substract_vana_back,
            'vana_back_factor': self._content.dSB_vana_back_factor,
            'sample_back_factor': self._content.dSB_sample_back_factor,
            'mask_bad_detectors': self._content.cB_mask_bad_detectors,
            'get_wavelength': self._content.cB_get_wavelength,
        }

        # connect signals
        self._content.pB_estimate.clicked.connect(self._estimate_q_and_binning)
        self._map['get_wavelength'].stateChanged.connect(self._get_wavelength)
        self._map['det_efficency'].stateChanged.connect(
            self._disable_sub_det_efficency)
        self._map['substract_sample_back'].stateChanged.connect(
            self._disable_sub_substract_sample_back)
        self._map['substract_vana_back'].stateChanged.connect(
            self._disable_sub_substract_vana_back)

    # Signals
    sig_get_wavelength = Signal()
    sig_estimate_q_and_binning = Signal()

    def deactivate_get_wavelength(self):
        self._map['get_wavelength'].setCheckState(0)

    def _disable_sub_det_efficency(self, state):
        self._map['vanadium_temperature'].setEnabled(state)
        self._map['correct_elastic_peak_position'].setEnabled(state)
        self._map['substract_vana_back'].setEnabled(state)
        self._map['vana_back_factor'].setEnabled(state)
        self._map['mask_bad_detectors'].setEnabled(state)

    def _disable_sub_substract_sample_back(self, state):
        self._map['sample_back_factor'].setEnabled(state)

    def _disable_sub_substract_vana_back(self, state):
        self._map['vana_back_factor'].setEnabled(state)

    def _estimate_q_and_binning(self):
        self.sig_estimate_q_and_binning.emit()

    def _get_wavelength(self, state):
        if state:
            self.sig_get_wavelength.emit()

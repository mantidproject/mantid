# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic options tab view of DNS reduction GUI.
"""

from qtpy.QtCore import Signal

from mantidqt.utils.qt import load_ui

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSElasticSCOptionsView(DNSView):
    """
    Widget that lets user select reduction options.
    """

    NAME = "Options"

    def __init__(self, parent):
        super().__init__(parent)
        self._content = load_ui(__file__, "elastic_single_crystal_options.ui", baseinstance=self)
        self._map = {
            "all_options": self._content,
            "wavelength": self._content.dSB_wavelength,
            "get_wavelength": self._content.cB_get_wavelength,
            "norm_time": self._content.rB_norm_time,
            "norm_monitor": self._content.rB_norm_monitor,
            "corrections": self._content.gB_corrections,
            "det_efficiency": self._content.cB_det_efficiency,
            "sum_vana_sf_nsf": self._content.cB_sum_vana_sf_nsf,
            "ignore_vana_fields": self._content.cB_ignore_vana_fields,
            "flipping_ratio": self._content.cB_flipping_ratio,
            "subtract_background_from_sample": self._content.cB_subtract_background_from_sample,
            "background_factor": self._content.dSB_background_factor,
            "binning": self._content.gB_binning,
            "automatic_binning": self._content.cB_automatic_binning,
            "two_theta_min": self._content.dSB_two_theta_min,
            "two_theta_max": self._content.dSB_two_theta_max,
            "two_theta_bin_size": self._content.dSB_two_theta_bin_size,
            "omega_min": self._content.dSB_omega_min,
            "omega_max": self._content.dSB_omega_max,
            "omega_bin_size": self._content.dSB_omega_bin_size,
            "lattice_parameters": self._content.gB_lattice_parameter,
            "a": self._content.dSB_a,
            "b": self._content.dSB_b,
            "c": self._content.dSB_c,
            "alpha": self._content.dSB_alpha,
            "beta": self._content.dSB_beta,
            "gamma": self._content.dSB_gamma,
            "orientation": self._content.gB_orientation,
            "hkl1": self._content.lE_hkl1,
            "hkl2": self._content.lE_hkl2,
            "omega_offset": self._content.dSB_omega_offset,
            "use_dx_dy": self._content.cB_use_dx_dy,
            "dx": self._content.dSB_dx,
            "dy": self._content.dSB_dy,
        }

        self._map["use_dx_dy"].setChecked(True)
        self._map["dx"].setValue(3.672)
        self._map["dy"].setValue(6.539)
        self._map["corrections"].setChecked(False)
        self._map["all_options"].setEnabled(False)
        # connect signals
        self._attach_signal_slots()

    # signals
    sig_get_wavelength = Signal()
    sig_two_theta_max_changed = Signal()
    sig_two_theta_min_changed = Signal()
    sig_omega_max_changed = Signal()
    sig_omega_min_changed = Signal()
    sig_auto_binning_clicked = Signal(int)

    def deactivate_get_wavelength(self):
        self._map["get_wavelength"].setCheckState(0)

    def _get_wavelength(self, state):
        if state:
            self.sig_get_wavelength.emit()

    def _two_theta_min_changed(self):
        self.sig_two_theta_min_changed.emit()

    def _two_theta_max_changed(self):
        self.sig_two_theta_max_changed.emit()

    def _omega_min_changed(self):
        self.sig_omega_min_changed.emit()

    def _omega_max_changed(self):
        self.sig_omega_max_changed.emit()

    def _attach_signal_slots(self):
        self._map["wavelength"].valueChanged.connect(self.deactivate_get_wavelength)
        self._map["get_wavelength"].stateChanged.connect(self._get_wavelength)
        self._map["two_theta_min"].valueChanged.connect(self._two_theta_min_changed)
        self._map["two_theta_max"].valueChanged.connect(self._two_theta_max_changed)
        self._map["omega_min"].valueChanged.connect(self._omega_min_changed)
        self._map["omega_max"].valueChanged.connect(self._omega_max_changed)

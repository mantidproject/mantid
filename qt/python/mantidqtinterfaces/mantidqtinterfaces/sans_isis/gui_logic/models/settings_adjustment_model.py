# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The state gui model contains all the reduction information which is not explicitly available in the data table.

This is one of the two model_tests which is used for the data reduction. It contains generally all the settings which
are not available in the model associated with the data table.
"""

from SANS.sans.common.enums import SANSInstrument, DataType, DetectorType, RebinType
from mantidqtinterfaces.sans_isis.gui_logic.models.model_common import ModelCommon
from SANS.sans.state.StateObjects.StateCalculateTransmission import StateTransmissionFit
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import meter_2_millimeter, millimeter_2_meter


class SettingsAdjustmentModel(ModelCommon):
    def __init__(self, all_states=None):
        super(SettingsAdjustmentModel, self).__init__(all_states=all_states)

    def __eq__(self, other):
        return self.__dict__ == other.__dict__

    # ================== Logic =======================
    def does_instrument_support_monitor_5(self):
        """
        Checks if the current instrument supports monitor 5
        :return: True if the instrument does
        """
        # At a later date we could programmatically determine if an instrument is using
        # (or supports) monitor 5 by summing the counts and enabling where != 0
        # for the moment however only Zoom has this capability so just hard code check
        return True if self.instrument is SANSInstrument.ZOOM else False

    def has_transmission_fit_got_separate_settings_for_sample_and_can(self):
        can_vals = self._all_states.adjustment.calculate_transmission.fit[DataType.CAN.value]
        sample_vals = self._all_states.adjustment.calculate_transmission.fit[DataType.SAMPLE.value]
        return can_vals == sample_vals

    # ------------------------------------------------------------------------------------------------------------------
    # Adjustment settings
    # ------------------------------------------------------------------------------------------------------------------

    @property
    def pixel_adjustment_det_1(self):
        val = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files[DetectorType.LAB.value].pixel_adjustment_file
        return self._get_val_or_default(val)

    @pixel_adjustment_det_1.setter
    def pixel_adjustment_det_1(self, value):
        self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files[DetectorType.LAB.value].pixel_adjustment_file = value

    @property
    def pixel_adjustment_det_2(self):
        adj_files = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files
        if DetectorType.HAB.value in adj_files:
            val = adj_files[DetectorType.HAB.value].pixel_adjustment_file
            return self._get_val_or_default(val)
        return ""

    @pixel_adjustment_det_2.setter
    def pixel_adjustment_det_2(self, value):
        adj_files = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files
        if DetectorType.HAB.value in adj_files:
            adj_files[DetectorType.HAB.value].pixel_adjustment_file = value

    @property
    def wavelength_adjustment_det_1(self):
        val = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files[
            DetectorType.LAB.value
        ].wavelength_adjustment_file
        return self._get_val_or_default(val)

    @wavelength_adjustment_det_1.setter
    def wavelength_adjustment_det_1(self, value):
        self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files[
            DetectorType.LAB.value
        ].wavelength_adjustment_file = value

    @property
    def wavelength_adjustment_det_2(self):
        adj_files = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files
        if DetectorType.HAB.value in adj_files:
            val = adj_files[DetectorType.HAB.value].wavelength_adjustment_file
            return self._get_val_or_default(val)
        return ""

    @wavelength_adjustment_det_2.setter
    def wavelength_adjustment_det_2(self, value):
        adj_files = self._all_states.adjustment.wavelength_and_pixel_adjustment.adjustment_files
        if DetectorType.HAB.value in adj_files:
            adj_files[DetectorType.HAB.value].wavelength_adjustment_file = value

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission Fitting
    # ------------------------------------------------------------------------------------------------------------------

    def _get_fit_val(self, data_type: DataType) -> StateTransmissionFit:
        return self._all_states.adjustment.calculate_transmission.fit[data_type.value]

    @property
    def transmission_sample_fit_type(self):
        val = self._get_fit_val(DataType.SAMPLE).fit_type
        return self._get_val_or_default(val)

    @transmission_sample_fit_type.setter
    def transmission_sample_fit_type(self, value):
        self._get_fit_val(DataType.SAMPLE).fit_type = value

    @property
    def transmission_can_fit_type(self):
        val = self._get_fit_val(DataType.CAN).fit_type
        return self._get_val_or_default(val)

    @transmission_can_fit_type.setter
    def transmission_can_fit_type(self, value):
        self._get_fit_val(DataType.CAN).fit_type = value

    @property
    def transmission_sample_polynomial_order(self):
        val = self._get_fit_val(DataType.SAMPLE).polynomial_order
        return self._get_val_or_default(val)

    @transmission_sample_polynomial_order.setter
    def transmission_sample_polynomial_order(self, value):
        self._get_fit_val(DataType.SAMPLE).polynomial_order = value

    @property
    def transmission_can_polynomial_order(self):
        val = self._get_fit_val(DataType.CAN).polynomial_order
        return self._get_val_or_default(val)

    @transmission_can_polynomial_order.setter
    def transmission_can_polynomial_order(self, value):
        self._get_fit_val(DataType.CAN).polynomial_order = value

    @property
    def transmission_sample_wavelength_min(self):
        val = self._get_fit_val(DataType.SAMPLE).wavelength_low
        return self._get_val_or_default(val)

    @transmission_sample_wavelength_min.setter
    def transmission_sample_wavelength_min(self, value):
        self._get_fit_val(DataType.SAMPLE).wavelength_low = value

    @property
    def transmission_sample_wavelength_max(self):
        val = self._get_fit_val(DataType.SAMPLE).wavelength_high
        return self._get_val_or_default(val)

    @transmission_sample_wavelength_max.setter
    def transmission_sample_wavelength_max(self, value):
        self._get_fit_val(DataType.SAMPLE).wavelength_high = value

    @property
    def transmission_can_wavelength_min(self):
        val = self._get_fit_val(DataType.CAN).wavelength_low
        return self._get_val_or_default(val)

    @transmission_can_wavelength_min.setter
    def transmission_can_wavelength_min(self, value):
        self._get_fit_val(DataType.CAN).wavelength_low = value

    @property
    def transmission_can_wavelength_max(self):
        val = self._get_fit_val(DataType.CAN).wavelength_high
        return self._get_val_or_default(val)

    @transmission_can_wavelength_max.setter
    def transmission_can_wavelength_max(self, value):
        self._get_fit_val(DataType.CAN).wavelength_high = value

    # ------------------------------------------------------------------------------------------------------------------
    # Monitor normalization
    # ------------------------------------------------------------------------------------------------------------------

    @property
    def normalization_interpolate(self):
        val = self._all_states.adjustment.normalize_to_monitor.rebin_type
        return val is RebinType.INTERPOLATING_REBIN

    @normalization_interpolate.setter
    def normalization_interpolate(self, value):
        new_type = RebinType.INTERPOLATING_REBIN if value else RebinType.REBIN
        self._all_states.adjustment.normalize_to_monitor.rebin_type = new_type

    @property
    def normalization_incident_monitor(self):
        val = self._all_states.adjustment.normalize_to_monitor.incident_monitor
        return self._get_val_or_default(val)

    @normalization_incident_monitor.setter
    def normalization_incident_monitor(self, value):
        self._all_states.adjustment.normalize_to_monitor.incident_monitor = value

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission
    # ------------------------------------------------------------------------------------------------------------------

    @property
    def transmission_incident_monitor(self):
        val = self._all_states.adjustment.calculate_transmission.incident_monitor
        return self._get_val_or_default(val)

    @transmission_incident_monitor.setter
    def transmission_incident_monitor(self, value):
        self._all_states.adjustment.calculate_transmission.incident_monitor = value

    @property
    def transmission_interpolate(self):
        val = self._all_states.adjustment.calculate_transmission.rebin_type
        return val is RebinType.INTERPOLATING_REBIN

    @transmission_interpolate.setter
    def transmission_interpolate(self, value):
        new_type = RebinType.INTERPOLATING_REBIN if value else RebinType.REBIN
        self._all_states.adjustment.calculate_transmission.rebin_type = new_type

    @property
    def transmission_roi_files(self):
        val = self._all_states.adjustment.calculate_transmission.transmission_roi_files
        return self._get_val_or_default(val)

    @transmission_roi_files.setter
    def transmission_roi_files(self, value):
        self._all_states.adjustment.calculate_transmission.transmission_roi_files = value

    @property
    def transmission_mask_files(self):
        val = self._all_states.adjustment.calculate_transmission.transmission_mask_files
        return self._get_val_or_default(val)

    @transmission_mask_files.setter
    def transmission_mask_files(self, value):
        self._all_states.adjustment.calculate_transmission.transmission_mask_files = value

    @property
    def transmission_radius(self):
        val = self._all_states.adjustment.calculate_transmission.transmission_radius_on_detector
        return self._get_val_or_default(val)

    @transmission_radius.setter
    def transmission_radius(self, value):
        self._all_states.adjustment.calculate_transmission.transmission_radius_on_detector = value

    @property
    def transmission_monitor(self):
        val = self._all_states.adjustment.calculate_transmission.transmission_monitor
        return self._get_val_or_default(val, 3)

    @transmission_monitor.setter
    def transmission_monitor(self, value):
        self._all_states.adjustment.calculate_transmission.transmission_monitor = value

    @property
    def transmission_mn_4_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        val = getattr(self._all_states.move, "monitor_4_offset", None)
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @transmission_mn_4_shift.setter
    def transmission_mn_4_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        if hasattr(self._all_states.move, "monitor_4_offset"):
            self._all_states.move.monitor_4_offset = millimeter_2_meter(value)

    @property
    def transmission_mn_5_shift(self):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        val = getattr(self._all_states.move, "monitor_5_offset", None)
        return meter_2_millimeter(self._get_val_or_default(val, 0))

    @transmission_mn_5_shift.setter
    def transmission_mn_5_shift(self, value):
        # Note that this is actually part of the move operation, but is conceptually part of transmission
        if hasattr(self._all_states.move, "monitor_5_offset"):
            self._all_states.move.monitor_5_offset = millimeter_2_meter(value)

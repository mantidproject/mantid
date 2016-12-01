import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateCalculateTransmissionBuilder import get_calculate_transmission_builder
from SANS2.Common.SANSType import (RebinType, RangeStepType, DataType, convert_reduction_data_type_to_string)
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument, FitType)


class SANSStateCalculateTransmissionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_calculate_transmission_builder(data_info)
        self.assertTrue(builder)

        builder.set_prompt_peak_correction_min(12.0)
        builder.set_prompt_peak_correction_max(17.0)

        builder.set_incident_monitor(1)
        builder.set_default_incident_monitor(2)
        builder.set_transmission_monitor(3)
        builder.set_default_transmission_monitor(4)
        builder.set_transmission_radius_on_detector(1.)
        builder.set_transmission_roi_files(["sdfs", "sddfsdf"])
        builder.set_transmission_mask_files(["sdfs", "bbbbbb"])

        builder.set_rebin_type(RebinType.Rebin)
        builder.set_wavelength_low(1.5)
        builder.set_wavelength_high(2.7)
        builder.set_wavelength_step(0.5)
        builder.set_wavelength_step_type(RangeStepType.Lin)
        builder.set_use_full_wavelength_range(True)
        builder.set_wavelength_full_range_low(12.)
        builder.set_wavelength_full_range_high(24.)

        builder.set_background_TOF_general_start(1.4)
        builder.set_background_TOF_general_stop(34.4)
        builder.set_background_TOF_monitor_start({"1": 123, "2": 123})
        builder.set_background_TOF_monitor_stop({"1": 234, "2": 2323})
        builder.set_background_TOF_roi_start(1.4)
        builder.set_background_TOF_roi_stop(34.4)

        builder.set_Sample_fit_type(FitType.Linear)
        builder.set_Sample_polynomial_order(0)
        builder.set_Sample_wavelength_low(10.0)
        builder.set_Sample_wavelength_high(20.0)

        builder.set_Can_fit_type(FitType.Polynomial)
        builder.set_Can_polynomial_order(3)
        builder.set_Can_wavelength_low(10.0)
        builder.set_Can_wavelength_high(20.0)

        state = builder.build()

        # Assert
        self.assertTrue(state.prompt_peak_correction_min == 12.0)
        self.assertTrue(state.prompt_peak_correction_max == 17.0)

        self.assertTrue(state.incident_monitor == 1)
        self.assertTrue(state.default_incident_monitor == 2)
        self.assertTrue(state.transmission_monitor == 3)
        self.assertTrue(state.default_transmission_monitor == 4)
        self.assertTrue(state.transmission_radius_on_detector == 1.)
        self.assertTrue(state.transmission_roi_files == ["sdfs", "sddfsdf"])
        self.assertTrue(state.transmission_mask_files == ["sdfs", "bbbbbb"])

        self.assertTrue(state.rebin_type is RebinType.Rebin)
        self.assertTrue(state.wavelength_low == 1.5)
        self.assertTrue(state.wavelength_high == 2.7)
        self.assertTrue(state.wavelength_step == 0.5)
        self.assertTrue(state.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(state.use_full_wavelength_range is True)
        self.assertTrue(state.wavelength_full_range_low == 12.)
        self.assertTrue(state.wavelength_full_range_high == 24.)

        self.assertTrue(state.background_TOF_general_start == 1.4)
        self.assertTrue(state.background_TOF_general_stop == 34.4)
        self.assertTrue(len(set(state.background_TOF_monitor_start.items()) & set({"1": 123, "2": 123}.items())) == 2)
        self.assertTrue(len(set(state.background_TOF_monitor_stop.items()) & set({"1": 234, "2": 2323}.items())) == 2)
        self.assertTrue(state.background_TOF_roi_start == 1.4)
        self.assertTrue(state.background_TOF_roi_stop == 34.4)

        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Sample)].fit_type is FitType.Linear)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Sample)].polynomial_order == 0)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Sample)].wavelength_low == 10.)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Sample)].wavelength_high == 20.)

        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Can)].fit_type is
                        FitType.Polynomial)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Can)].polynomial_order == 3)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Can)].wavelength_low == 10.)
        self.assertTrue(state.fit[convert_reduction_data_type_to_string(DataType.Can)].wavelength_high == 20.)


if __name__ == '__main__':
    unittest.main()

import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateNormalizeToMonitorBuilder import get_normalize_to_monitor_builder
from SANS2.Common.SANSType import (RebinType, RangeStepType)
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument)


class SANSStateReductionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_normalize_to_monitor_builder(data_info)
        self.assertTrue(builder)

        builder.set_prompt_peak_correction_min(12.0)
        builder.set_prompt_peak_correction_max(17.0)
        builder.set_rebin_type(RebinType.Rebin)
        builder.set_wavelength_low(1.5)
        builder.set_wavelength_high(2.7)
        builder.set_wavelength_step(0.5)
        builder.set_wavelength_step_type(RangeStepType.Lin)
        builder.set_incident_monitor(1)
        builder.set_background_TOF_general_start(1.4)
        builder.set_background_TOF_general_stop(34.4)
        builder.set_background_TOF_monitor_start({"1": 123, "2": 123})
        builder.set_background_TOF_monitor_stop({"1": 234, "2": 2323})

        state = builder.build()

        # Assert
        self.assertTrue(state.prompt_peak_correction_min == 12.0)
        self.assertTrue(state.prompt_peak_correction_max == 17.0)
        self.assertTrue(state.rebin_type is RebinType.Rebin)
        self.assertTrue(state.wavelength_low == 1.5)
        self.assertTrue(state.wavelength_high == 2.7)
        self.assertTrue(state.wavelength_step == 0.5)
        self.assertTrue(state.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(state.background_TOF_general_start == 1.4)
        self.assertTrue(state.background_TOF_general_stop == 34.4)
        self.assertTrue(len(set(state.background_TOF_monitor_start.items()) & set({"1": 123, "2": 123}.items())) == 2)
        self.assertTrue(len(set(state.background_TOF_monitor_stop.items()) & set({"1": 234, "2": 2323}.items())) == 2)
        self.assertTrue(state.incident_monitor == 1)


if __name__ == '__main__':
    unittest.main()

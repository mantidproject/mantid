# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from sans.common.enums import (SANSFacility, SANSInstrument, RebinType, RangeStepType)
from sans.state.data import get_data_builder
from sans.state.wavelength import (StateWavelength, get_wavelength_builder)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthTest(unittest.TestCase):

    def test_that_is_sans_state_data_object(self):
        state = StateWavelength()
        self.assertTrue(isinstance(state, StateWavelength))

    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = StateWavelength()
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_low = [1.]
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_high = [2.]
        with self.assertRaises(ValueError):
            state.validate()

        state.wavelength_step = 2.
        self.assertIsNone(state.validate())

    def test_that_raises_when_lower_wavelength_is_smaller_than_high_wavelength(self):
        state = StateWavelength()
        state.wavelength_low = [2.]
        state.wavelength_high = [1.]
        state.wavelength_step = 2.
        with self.assertRaises(ValueError):
            state.validate()


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------

class StateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_wavelength_builder(data_info)
        self.assertTrue(builder)

        builder.set_wavelength_low([10.0])
        builder.set_wavelength_high([20.0])
        builder.set_wavelength_step(3.0)
        builder.set_wavelength_step_type(RangeStepType.LIN)
        builder.set_rebin_type(RebinType.REBIN)

        # Assert
        state = builder.build()

        self.assertEqual(state.wavelength_low,  [10.0])
        self.assertEqual(state.wavelength_high,  [20.0])
        self.assertEqual(state.wavelength_step_type, RangeStepType.LIN)
        self.assertEqual(state.rebin_type, RebinType.REBIN)


if __name__ == '__main__':
    unittest.main()

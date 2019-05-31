# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
from sans.command_interface.command_interface_state_director import (NParameterCommand, NParameterCommandId,
                                                                     CommandInterfaceStateDirector, DataCommand,
                                                                     DataCommandId, FitData)
from sans.common.enums import (SANSFacility, RebinType, DetectorType, ReductionDimensionality,
                               FitType, RangeStepType, ISISReductionMode, FitModeForMerge, DataType)


class CommandInterfaceStateDirectorTest(unittest.TestCase):
    def _assert_raises_nothing(self, func, parameter):
        try:
            func(parameter)
        except:  # noqa
            self.fail()

    def test_can_set_commands_without_exceptions(self):
        command_interface = CommandInterfaceStateDirector(SANSFacility.ISIS)

        # User file
        command = NParameterCommand(command_id=NParameterCommandId.user_file,
                                    values=["test_user_file_sans2d.txt"])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Mask
        command = NParameterCommand(command_id=NParameterCommandId.mask,
                                    values=["MASK/ FRONT H197>H199"])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Monitor spectrum (incident monitor for monitor normalization)
        command = NParameterCommand(command_id=NParameterCommandId.incident_spectrum,
                                    values=[1, True, False])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Transmission spectrum (incident monitor for transmission calculation)
        command = NParameterCommand(command_id=NParameterCommandId.incident_spectrum, values=[7, False, True])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Reduction Dimensionality One Dim
        command = NParameterCommand(command_id=NParameterCommandId.reduction_dimensionality,
                                    values=[ReductionDimensionality.OneDim])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Reduction Dimensionality Two Dim
        command = NParameterCommand(command_id=NParameterCommandId.reduction_dimensionality,
                                    values=[ReductionDimensionality.TwoDim])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Sample offset
        command = NParameterCommand(command_id=NParameterCommandId.sample_offset, values=[23.6])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Sample scatter data
        command = DataCommand(command_id=DataCommandId.sample_scatter, file_name="SANS2D00022024", period=3)
        self._assert_raises_nothing(command_interface.add_command, command)

        # Detector
        command = NParameterCommand(command_id=NParameterCommandId.detector, values=[ISISReductionMode.HAB])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Gravity
        command = NParameterCommand(command_id=NParameterCommandId.gravity, values=[True, 12.4])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Set centre
        command = NParameterCommand(command_id=NParameterCommandId.centre, values=[12.4, 23.54, DetectorType.HAB])
        self._assert_raises_nothing(command_interface.add_command, command)

        # # Trans fit
        command = NParameterCommand(command_id=NParameterCommandId.trans_fit, values=[FitData.Can, 10.4, 12.54,
                                                                                      FitType.Logarithmic, 0])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Front detector rescale
        command = NParameterCommand(command_id=NParameterCommandId.front_detector_rescale, values=[1.2, 2.4, True,
                                                                                                   False, None, 7.2])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Event slices
        command = NParameterCommand(command_id=NParameterCommandId.event_slices, values="1-23,55:3:65")
        self._assert_raises_nothing(command_interface.add_command, command)

        # Flood file
        command = NParameterCommand(command_id=NParameterCommandId.flood_file, values=["test", DetectorType.LAB])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Phi limits
        command = NParameterCommand(command_id=NParameterCommandId.phi_limit, values=[12.5, 123.6, False])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Wavelength correction file
        command = NParameterCommand(command_id=NParameterCommandId.wavelength_correction_file,
                                    values=["test", DetectorType.HAB])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Radius mask
        command = NParameterCommand(command_id=NParameterCommandId.mask_radius,
                                    values=[23.5, 234.7])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Wavelength limits
        command = NParameterCommand(command_id=NParameterCommandId.wavelength_limit,
                                    values=[1.23, 23., 1.1, RangeStepType.Lin])
        self._assert_raises_nothing(command_interface.add_command, command)

        # QXY Limits
        command = NParameterCommand(command_id=NParameterCommandId.qxy_limit,
                                    values=[1.23, 23., 1.1, RangeStepType.Lin])
        self._assert_raises_nothing(command_interface.add_command, command)

        # Process all commands
        state = command_interface.process_commands()

        # Assert
        # We check here that the elements we set up above (except for from the user file) are being applied
        self.assertTrue(state is not None)
        self.assertTrue(state.mask.detectors[DetectorType.to_string(DetectorType.HAB)].range_horizontal_strip_start[-1]
                        == 197)
        self.assertTrue(state.mask.detectors[DetectorType.to_string(DetectorType.HAB)].range_horizontal_strip_stop[-1]
                        == 199)
        self.assertEqual(state.adjustment.normalize_to_monitor.incident_monitor,  1)
        self.assertTrue(state.adjustment.normalize_to_monitor.rebin_type is RebinType.InterpolatingRebin)
        self.assertEqual(state.adjustment.calculate_transmission.incident_monitor,  7)
        self.assertTrue(state.adjustment.calculate_transmission.rebin_type is RebinType.Rebin)
        self.assertTrue(state.reduction.reduction_dimensionality is ReductionDimensionality.TwoDim)
        self.assertTrue(state.convert_to_q.reduction_dimensionality is ReductionDimensionality.TwoDim)
        self.assertEqual(state.move.sample_offset,  23.6/1000.)
        self.assertEqual(state.data.sample_scatter,  "SANS2D00022024")
        self.assertEqual(state.data.sample_scatter_period,  3)
        self.assertTrue(state.reduction.reduction_mode is ISISReductionMode.HAB)
        self.assertTrue(state.convert_to_q.use_gravity)
        self.assertEqual(state.convert_to_q.gravity_extra_length,  12.4)
        self.assertEqual(state.move.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos1,  12.4/1000.)
        self.assertTrue(state.move.detectors[DetectorType.to_string(DetectorType.HAB)].sample_centre_pos2
                        == 23.54/1000.)
        self.assertTrue(state.adjustment.calculate_transmission.fit[DataType.to_string(DataType.Can)].fit_type
                        is FitType.Logarithmic)
        self.assertTrue(state.adjustment.calculate_transmission.fit[DataType.to_string(DataType.Can)].polynomial_order
                        == 0)

        self.assertTrue(state.adjustment.calculate_transmission.fit[DataType.to_string(DataType.Can)].wavelength_low
                        == 10.4)
        self.assertTrue(state.adjustment.calculate_transmission.fit[DataType.to_string(DataType.Can)].wavelength_high
                        == 12.54)

        self.assertEqual(state.reduction.merge_scale,  1.2)
        self.assertEqual(state.reduction.merge_shift,  2.4)
        self.assertTrue(state.reduction.merge_fit_mode is FitModeForMerge.ScaleOnly)
        self.assertTrue(state.reduction.merge_range_min is None)
        self.assertEqual(state.reduction.merge_range_max,  7.2)

        # Event slices
        start_values = state.slice.start_time
        end_values = state.slice.end_time
        expected_start_values = [1., 55., 58., 61., 64.]
        expected_end_values = [23., 58., 61., 64., 65.]
        for s1, e1, s2, e2 in zip(start_values, end_values, expected_start_values, expected_end_values):
            self.assertEqual(s1,  s2)
            self.assertEqual(e1,  e2)

        self.assertTrue(state.adjustment.wavelength_and_pixel_adjustment.adjustment_files[
                            DetectorType.to_string(DetectorType.LAB)].pixel_adjustment_file == "test")
        self.assertEqual(state.mask.phi_min,  12.5)
        self.assertEqual(state.mask.phi_max,  123.6)
        self.assertFalse(state.mask.use_mask_phi_mirror)
        self.assertTrue(state.adjustment.wavelength_and_pixel_adjustment.adjustment_files[
                            DetectorType.to_string(DetectorType.HAB)].wavelength_adjustment_file == "test")
        self.assertEqual(state.mask.radius_min,  23.5 / 1000.)
        self.assertEqual(state.mask.radius_max,  234.7 / 1000.)
        self.assertEqual(state.wavelength.wavelength_low,  [1.23])
        self.assertEqual(state.adjustment.normalize_to_monitor.wavelength_high,  [23.])
        self.assertEqual(state.adjustment.wavelength_and_pixel_adjustment.wavelength_step,  1.1)
        self.assertTrue(state.adjustment.calculate_transmission.wavelength_step_type is RangeStepType.Lin)
        self.assertEqual(state.convert_to_q.q_xy_max,  23.)
        self.assertEqual(state.convert_to_q.q_xy_step,  1.1)
        self.assertTrue(state.convert_to_q.q_xy_step_type is RangeStepType.Lin)

    def test_that_can_remove_last_command(self):
        # Arrange
        command_interface = CommandInterfaceStateDirector(SANSFacility.ISIS)
        command_interface.add_command(NParameterCommand(command_id=NParameterCommandId.user_file,
                                                        values=["file_1.txt"]))
        command_interface.add_command(NParameterCommand(command_id=NParameterCommandId.user_file,
                                                        values=["file_2.txt"]))
        command_interface.add_command(NParameterCommand(command_id=NParameterCommandId.user_file,
                                                        values=["file_3.txt"]))
        # Act
        commands = command_interface.get_commands()
        self.assertEqual(len(commands),  3)

        command_interface.remove_last_user_file()

        # Assert
        self.assertEqual(len(commands),  2)
        last_command = commands[-1]
        self.assertEqual(last_command.values,  ["file_2.txt"])


if __name__ == '__main__':
    unittest.main()

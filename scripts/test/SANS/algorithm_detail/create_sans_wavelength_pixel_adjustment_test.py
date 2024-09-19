# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import os
import unittest

from mantid.kernel import config
from mantid.simpleapi import CreateSampleWorkspace
from sans.algorithm_detail.CreateSANSWavelengthPixelAdjustment import CreateSANSWavelengthPixelAdjustment
from SANS.sans.common.enums import RangeStepType, DetectorType
from SANS.sans.state.StateObjects.StateWavelengthAndPixelAdjustment import get_wavelength_and_pixel_adjustment_builder
from sans.test_helper.test_director import TestDirector


class CreateSANSWavelengthPixelAdjustmentTest(unittest.TestCase):
    @staticmethod
    def _create_test_wavelength_adjustment_file(file_name):
        test_file = (
            "  Tue 24-MAR-2015 00:02 Workspace: directbeam_new_hist\n"
            "\n"
            "  6    0    0    0    1  6    0\n"
            "         0         0         0         0\n"
            " 3 (F12.5,2E16.6)\n"
            "     1.00000    5.000000e-01    5.000000e-01\n"
            "     3.00000    5.000000e-01    5.000000e-01\n"
            "     5.00000    5.000000e-01    5.000000e-01\n"
            "     7.00000    5.000000e-01    5.000000e-01\n"
            "     9.00000    5.000000e-01    5.000000e-01\n"
            "    11.00000    5.000000e-01    5.000000e-01\n"
        )

        full_file_path = os.path.join(config.getString("defaultsave.directory"), file_name)
        if os.path.exists(full_file_path):
            os.remove(full_file_path)

        with open(full_file_path, "w") as f:
            f.write(test_file)
        return full_file_path

    @staticmethod
    def _remove_test_file(file_name):
        if os.path.exists(file_name):
            os.remove(file_name)

    @staticmethod
    def _get_state(
        lab_pixel_file=None,
        hab_pixel_file=None,
        lab_wavelength_file=None,
        hab_wavelength_file=None,
        wavelength_low=None,
        wavelength_high=None,
        wavelength_step=None,
        wavelength_step_type=None,
    ):
        test_director = TestDirector()
        state = test_director.construct()
        data_state = state.data
        wavelength_and_pixel_builder = get_wavelength_and_pixel_adjustment_builder(data_state)
        if lab_pixel_file:
            wavelength_and_pixel_builder.set_LAB_pixel_adjustment_file(lab_pixel_file)
        if hab_pixel_file:
            wavelength_and_pixel_builder.set_HAB_pixel_adjustment_file(hab_pixel_file)
        if lab_wavelength_file:
            wavelength_and_pixel_builder.set_LAB_wavelength_adjustment_file(lab_wavelength_file)
        if hab_wavelength_file:
            wavelength_and_pixel_builder.set_HAB_wavelength_adjustment_file(hab_wavelength_file)
        if wavelength_step_type:
            wavelength_and_pixel_builder.set_wavelength_step_type(wavelength_step_type)
        if wavelength_low and wavelength_high:
            wav_range = (wavelength_low, wavelength_high)
            wavelength_and_pixel_builder.state.wavelength_interval.wavelength_full_range = wav_range
            wavelength_and_pixel_builder.state.wavelength_interval.selected_ranges = [wav_range]
        if wavelength_step:
            wavelength_and_pixel_builder.state.wavelength_interval.wavelength_step = wavelength_step
        wavelength_and_pixel_state = wavelength_and_pixel_builder.build()
        state.adjustment.wavelength_and_pixel_adjustment = wavelength_and_pixel_state
        return state

    @staticmethod
    def _get_workspace(data):
        workspace = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1, XMin=1, XMax=11, BinWidth=2, XUnit="Wavelength", StoreInADS=False)

        data_y = workspace.dataY(0)
        for index in range(len(data_y)):
            data_y[index] = data[index]
        return workspace

    @staticmethod
    def _run_test(transmission_workspace, norm_workspace, state, is_lab=True):
        state_to_send = state.adjustment.wavelength_and_pixel_adjustment
        wav_range = state_to_send.wavelength_interval.wavelength_full_range

        component = DetectorType.LAB.value if is_lab else DetectorType.HAB.value

        alg = CreateSANSWavelengthPixelAdjustment(state_adjustment_wavelength_and_pixel=state_to_send, component=component)

        wavelength_adjustment, pixel_adjustment = alg.create_sans_wavelength_and_pixel_adjustment(
            transmission_ws=transmission_workspace, monitor_norm_ws=norm_workspace, wav_range=wav_range
        )

        return wavelength_adjustment, pixel_adjustment

    def test_that_gets_wavelength_workspace_when_no_files_are_specified(self):
        # Arrange
        data_trans = [3.0, 4.0, 5.0, 7.0, 3.0]
        data_norm = [9.0, 3.0, 8.0, 3.0, 1.0]
        transmission_workspace = CreateSANSWavelengthPixelAdjustmentTest._get_workspace(data_trans)
        norm_workspace = CreateSANSWavelengthPixelAdjustmentTest._get_workspace(data_norm)

        state = self._get_state(wavelength_low=1.0, wavelength_high=11.0, wavelength_step=2.0, wavelength_step_type=RangeStepType.LIN)

        # Act
        wavelength_adjustment, pixel_adjustment = self._run_test(transmission_workspace, norm_workspace, state, True)
        # Assert
        self.assertEqual(pixel_adjustment, None)
        self.assertEqual(wavelength_adjustment.getNumberHistograms(), 1)
        expected = np.array(data_trans) * np.array(data_norm)
        data_y = wavelength_adjustment.dataY(0)
        for (
            e1,
            e2,
        ) in zip(expected, data_y):
            self.assertEqual(e1, e2)

    def test_that_gets_adjustment_workspace_if_files_are_specified(self):
        # Arrange

        data_trans = [3.0, 4.0, 5.0, 7.0, 3.0]
        data_norm = [9.0, 3.0, 8.0, 3.0, 1.0]
        expected_direct_file_workspace = [0.5, 0.5, 0.5, 0.5, 0.5]
        transmission_workspace = CreateSANSWavelengthPixelAdjustmentTest._get_workspace(data_trans)
        norm_workspace = CreateSANSWavelengthPixelAdjustmentTest._get_workspace(data_norm)

        direct_file_name = "DIRECT_test.txt"
        direct_file_name = CreateSANSWavelengthPixelAdjustmentTest._create_test_wavelength_adjustment_file(direct_file_name)

        state = CreateSANSWavelengthPixelAdjustmentTest._get_state(
            hab_wavelength_file=direct_file_name,
            wavelength_low=1.0,
            wavelength_high=11.0,
            wavelength_step=2.0,
            wavelength_step_type=RangeStepType.LIN,
        )
        # Act
        wavelength_adjustment, pixel_adjustment = CreateSANSWavelengthPixelAdjustmentTest._run_test(
            transmission_workspace, norm_workspace, state, False
        )
        # Assert
        self.assertEqual(pixel_adjustment, None)
        self.assertEqual(wavelength_adjustment.getNumberHistograms(), 1)
        expected = np.array(data_trans) * np.array(data_norm) * np.array(expected_direct_file_workspace)
        data_y = wavelength_adjustment.dataY(0)
        for (
            e1,
            e2,
        ) in zip(expected, data_y):
            self.assertEqual(e1, e2)

        # Clean up
        CreateSANSWavelengthPixelAdjustmentTest._remove_test_file(direct_file_name)


if __name__ == "__main__":
    unittest.main()

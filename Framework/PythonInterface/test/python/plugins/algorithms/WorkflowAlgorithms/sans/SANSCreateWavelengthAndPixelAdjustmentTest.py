# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.api import FrameworkManager
from mantid.kernel import config

import os
import numpy as np
from sans.test_helper.test_director import TestDirector
from sans.state.wavelength_and_pixel_adjustment import get_wavelength_and_pixel_adjustment_builder
from sans.common.enums import (RebinType, RangeStepType, DetectorType)
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.common.constants import EMPTY_NAME


class SANSCalculateTransmissionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _create_test_wavelength_adjustment_file(file_name):
        test_file = ("  Tue 24-MAR-2015 00:02 Workspace: directbeam_new_hist\n"
                     "\n"
                     "  6    0    0    0    1  6    0\n"
                     "         0         0         0         0\n"
                     " 3 (F12.5,2E16.6)\n"
                     "     1.00000    5.000000e-01    5.000000e-01\n"
                     "     3.00000    5.000000e-01    5.000000e-01\n"
                     "     5.00000    5.000000e-01    5.000000e-01\n"
                     "     7.00000    5.000000e-01    5.000000e-01\n"
                     "     9.00000    5.000000e-01    5.000000e-01\n"
                     "    11.00000    5.000000e-01    5.000000e-01\n")

        full_file_path = os.path.join(config.getString('defaultsave.directory'), file_name)
        if os.path.exists(full_file_path):
            os.remove(full_file_path)

        with open(full_file_path, 'w') as f:
            f.write(test_file)
        return full_file_path

    @staticmethod
    def _remove_test_file(file_name):
        if os.path.exists(file_name):
           os.remove(file_name)

    @staticmethod
    def _get_state(lab_pixel_file=None, hab_pixel_file=None, lab_wavelength_file=None, hab_wavelength_file=None,
                   wavelength_low=None, wavelength_high=None, wavelength_step=None,
                   wavelength_step_type=None):
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
        if wavelength_low:
            wavelength_and_pixel_builder.set_wavelength_low([wavelength_low])
        if wavelength_high:
            wavelength_and_pixel_builder.set_wavelength_high([wavelength_high])
        if wavelength_step:
            wavelength_and_pixel_builder.set_wavelength_step(wavelength_step)
        wavelength_and_pixel_state = wavelength_and_pixel_builder.build()
        state.adjustment.wavelength_and_pixel_adjustment = wavelength_and_pixel_state
        return state.property_manager

    @staticmethod
    def _get_workspace(data):
        create_name = "CreateSampleWorkspace"
        create_options = {"NumBanks": 1,
                          "BankPixelWidth": 1,
                          "XMin": 1,
                          "XMax": 11,
                          "BinWidth": 2,
                          "XUnit": "Wavelength",
                          "OutputWorkspace": EMPTY_NAME}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)
        create_alg.execute()
        workspace = create_alg.getProperty("OutputWorkspace").value
        data_y = workspace.dataY(0)
        for index in range(len(data_y)):
            data_y[index] = data[index]
        return workspace

    @staticmethod
    def _run_test(transmission_workspace, norm_workspace, state, is_lab=True):
        adjust_name = "SANSCreateWavelengthAndPixelAdjustment"
        adjust_options = {"TransmissionWorkspace": transmission_workspace,
                          "NormalizeToMonitorWorkspace": norm_workspace,
                          "SANSState": state,
                          "OutputWorkspaceWavelengthAdjustment": "out_wavelength",
                          "OutputWorkspacePixelAdjustment": "out_pixels"}
        if is_lab:
            adjust_options.update({"Component": DetectorType.to_string(DetectorType.LAB)})
        else:
            adjust_options.update({"Component": DetectorType.to_string(DetectorType.HAB)})
        adjust_alg = create_unmanaged_algorithm(adjust_name, **adjust_options)
        adjust_alg.execute()
        wavelength_adjustment = adjust_alg.getProperty("OutputWorkspaceWavelengthAdjustment").value
        pixel_adjustment = adjust_alg.getProperty("OutputWorkspacePixelAdjustment").value
        return wavelength_adjustment, pixel_adjustment

    def test_that_gets_wavelength_workspace_when_no_files_are_specified(self):
        # Arrange
        data_trans = [3., 4., 5., 7., 3.]
        data_norm = [9., 3., 8., 3., 1.]
        transmission_workspace = SANSCalculateTransmissionTest._get_workspace(data_trans)
        norm_workspace = SANSCalculateTransmissionTest._get_workspace(data_norm)

        state = SANSCalculateTransmissionTest._get_state(wavelength_low=1., wavelength_high=11., wavelength_step=2.,
                                                         wavelength_step_type=RangeStepType.Lin)

        # Act
        wavelength_adjustment, pixel_adjustment = SANSCalculateTransmissionTest._run_test(transmission_workspace,
                                                                                          norm_workspace, state, True)
        # Assert
        self.assertTrue(pixel_adjustment is None)
        self.assertTrue(wavelength_adjustment.getNumberHistograms() == 1)
        expected = np.array(data_trans)*np.array(data_norm)
        data_y = wavelength_adjustment.dataY(0)
        for e1, e2, in zip(expected, data_y):
            self.assertTrue(e1 == e2)

    def test_that_gets_adjustment_workspace_if_files_are_specified(self):
        # Arrange

        data_trans = [3., 4., 5., 7., 3.]
        data_norm = [9., 3., 8., 3., 1.]
        expected_direct_file_workspace = [0.5, 0.5, 0.5, 0.5, 0.5]
        transmission_workspace = SANSCalculateTransmissionTest._get_workspace(data_trans)
        norm_workspace = SANSCalculateTransmissionTest._get_workspace(data_norm)

        direct_file_name = "DIRECT_test.txt"
        direct_file_name = SANSCalculateTransmissionTest._create_test_wavelength_adjustment_file(direct_file_name)

        state = SANSCalculateTransmissionTest._get_state(hab_wavelength_file=direct_file_name,
                                                         wavelength_low=1., wavelength_high=11., wavelength_step=2.,
                                                         wavelength_step_type=RangeStepType.Lin)
        # Act
        wavelength_adjustment, pixel_adjustment = SANSCalculateTransmissionTest._run_test(transmission_workspace,
                                                                                          norm_workspace, state, False)
        # Assert
        self.assertTrue(pixel_adjustment is None)
        self.assertTrue(wavelength_adjustment.getNumberHistograms() == 1)
        expected = np.array(data_trans)*np.array(data_norm)*np.array(expected_direct_file_workspace)
        data_y = wavelength_adjustment.dataY(0)
        for e1, e2, in zip(expected, data_y):
            self.assertTrue(e1 == e2)

        # Clean up
        SANSCalculateTransmissionTest._remove_test_file(direct_file_name)


if __name__ == '__main__':
    unittest.main()

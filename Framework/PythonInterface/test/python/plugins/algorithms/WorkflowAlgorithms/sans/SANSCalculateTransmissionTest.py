# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.api import FrameworkManager
import numpy as np
from sans.test_helper.test_director import TestDirector
from sans.state.calculate_transmission import get_calculate_transmission_builder
from sans.common.enums import (RebinType, RangeStepType, FitType)
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.common.constants import EMPTY_NAME


def get_expected_for_spectrum_n(data_workspace, selected_workspace_index, value_array):
    # Expected output.
    # 1. The background correction should produce [0., value., value., value.]
    # 2. Conversion to wavelength changes the x axis to approximately to
    #                                          [2.7, 5.5, 8.2, 10.9, 13.7] for spectrum 1
    #                                          [1.0, 2.0, 3.0, 4.1, 5.1]   for spectrum 3
    # 3. Rebinning from creates the x steps [2, 4, 6, 8]
    # This means for spectrum 1:
    #   Bin0:  0
    #   Bin1: (0 + abs(5.5-6.0)/abs(5.5 - 8.2))*value
    #   Bin2: (abs(6.0-8.0)/abs(5.5 - 8.2))*value
    # This means for spectrum 3:
    #   Bin0: (1. + abs(3.0-4.0)/abs(3.0 - 4.1))*value
    #   Bin1: (abs(4.0-4.1)/abs(3.0 - 4.1) + 1.)*value
    #   Bin2: 0

    # The second bin should have abs(4.0-4.7)/abs(3.2 - 4.7)*value + abs(4.7-6.0)/abs(4.7 - 6.3)*value
    # The third bin should have abs(6.0-6.3)/abs(4.7 - 6.3)*value + value
    instrument = data_workspace.getInstrument()
    source = instrument.getSource()
    detector = data_workspace.getDetector(selected_workspace_index)
    distance_source_detector = detector.getDistance(source)
    h = 6.62606896e-34
    mass = 1.674927211e-27
    times = data_workspace.dataX(0)
    lambda_after_unit_conversion = [(time * 1e-6) * h / distance_source_detector / mass * 1e10 for time in times]
    expected_lambda = [2., 4., 6., 8.]
    if selected_workspace_index == 0:
        expected_signal = [0.,
                           abs(lambda_after_unit_conversion[1] - expected_lambda[2]) /
                           abs(lambda_after_unit_conversion[1] - lambda_after_unit_conversion[2]) * value_array[1],
                           (abs(expected_lambda[3] - expected_lambda[2]) /
                            abs(lambda_after_unit_conversion[1] - lambda_after_unit_conversion[2])) * value_array[1]]
    else:
        expected_signal = [1. * value_array[1] + abs(lambda_after_unit_conversion[2] - expected_lambda[1]) /
                           abs(lambda_after_unit_conversion[2] - lambda_after_unit_conversion[3]) * value_array[2],
                           abs(lambda_after_unit_conversion[3] - expected_lambda[1]) /
                           abs(lambda_after_unit_conversion[2] - lambda_after_unit_conversion[3]) * value_array[2] +
                           1. * value_array[3],
                           0.]

    return np.array(expected_lambda), np.array(expected_signal)


class SANSCalculateTransmissionTest(unittest.TestCase):
    sample_workspace = None
    sample_workspace_2 = None

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _load_workspace(file_name):
        load_name = "Load"
        load_options = {"OutputWorkspace": EMPTY_NAME,
                        "Filename": file_name}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        return load_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _clone_workspace(workspace):
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _get_transmission_workspace(data=None):
        # Load the workspace
        if SANSCalculateTransmissionTest.sample_workspace is None:
            SANSCalculateTransmissionTest.sample_workspace = \
                SANSCalculateTransmissionTest._load_workspace("SANS2D00022024")
        # Clone the workspace
        workspace = SANSCalculateTransmissionTest._clone_workspace(SANSCalculateTransmissionTest.sample_workspace)
        # Prepare the workspace
        return SANSCalculateTransmissionTest._prepare_workspace(workspace, data=data)

    @staticmethod
    def _get_roi_workspace():
        # Load the workspace
        if SANSCalculateTransmissionTest.sample_workspace_2 is None:
            SANSCalculateTransmissionTest.sample_workspace_2 = SANSCalculateTransmissionTest._load_workspace("LOQ74044")
        # Clone the workspace
        return SANSCalculateTransmissionTest._clone_workspace(SANSCalculateTransmissionTest.sample_workspace_2)

    @staticmethod
    def _get_state(transmission_radius_on_detector=None, transmission_roi_files=None, transmission_mask_files=None,
                   transmission_monitor=None, incident_monitor=None, rebin_type=None, wavelength_low=None,
                   wavelength_high=None, wavelength_step=None, wavelength_step_type=None,
                   use_full_wavelength_range=None,
                   wavelength_full_range_low=None, wavelength_full_range_high=None, prompt_peak_correction_min=None,
                   prompt_peak_correction_max=None, background_TOF_general_start=None, background_TOF_general_stop=None,
                   background_TOF_monitor_start=None, background_TOF_monitor_stop=None, background_TOF_roi_start=None,
                   background_TOF_roi_stop=None, sample_fit_type=None, sample_polynomial_order=None,
                   sample_wavelength_low=None, sample_wavelength_high=None, can_fit_type=None,
                   can_polynomial_order=None,
                   can_wavelength_low=None, can_wavelength_high=None):
        test_director = TestDirector()
        state = test_director.construct()
        data_state = state.data
        calculate_transmission_builder = get_calculate_transmission_builder(data_state)
        if transmission_radius_on_detector:
            calculate_transmission_builder.set_transmission_radius_on_detector(transmission_radius_on_detector)
        if transmission_roi_files:
            calculate_transmission_builder.set_transmission_roi_files(transmission_roi_files)
        if transmission_mask_files:
            calculate_transmission_builder.set_transmission_mask_files(transmission_mask_files)
        if transmission_monitor:
            calculate_transmission_builder.set_transmission_monitor(transmission_monitor)

        if incident_monitor:
            calculate_transmission_builder.set_incident_monitor(incident_monitor)
        if rebin_type:
            calculate_transmission_builder.set_rebin_type(rebin_type)
        if wavelength_low:
            calculate_transmission_builder.set_wavelength_low([wavelength_low])
        if wavelength_high:
            calculate_transmission_builder.set_wavelength_high([wavelength_high])
        if wavelength_step:
            calculate_transmission_builder.set_wavelength_step(wavelength_step)
        if wavelength_step_type:
            calculate_transmission_builder.set_wavelength_step_type(wavelength_step_type)
        if use_full_wavelength_range:
            calculate_transmission_builder.set_use_full_wavelength_range(use_full_wavelength_range)
        if wavelength_full_range_low:
            calculate_transmission_builder.set_wavelength_full_range_low(wavelength_full_range_low)
        if wavelength_full_range_high:
            calculate_transmission_builder.set_wavelength_full_range_high(wavelength_full_range_high)

        if prompt_peak_correction_min:
            calculate_transmission_builder.set_prompt_peak_correction_min(prompt_peak_correction_min)
        if prompt_peak_correction_max:
            calculate_transmission_builder.set_prompt_peak_correction_max(prompt_peak_correction_max)
        if prompt_peak_correction_min and prompt_peak_correction_max:
            calculate_transmission_builder.set_prompt_peak_correction_enabled(True)

        if background_TOF_general_start:
            calculate_transmission_builder.set_background_TOF_general_start(background_TOF_general_start)
        if background_TOF_general_stop:
            calculate_transmission_builder.set_background_TOF_general_stop(background_TOF_general_stop)
        if background_TOF_monitor_start:
            calculate_transmission_builder.set_background_TOF_monitor_start(background_TOF_monitor_start)
        if background_TOF_monitor_stop:
            calculate_transmission_builder.set_background_TOF_monitor_stop(background_TOF_monitor_stop)
        if background_TOF_roi_start:
            calculate_transmission_builder.set_background_TOF_roi_start(background_TOF_roi_start)
        if background_TOF_roi_stop:
            calculate_transmission_builder.set_background_TOF_roi_stop(background_TOF_roi_stop)

        if sample_fit_type:
            calculate_transmission_builder.set_Sample_fit_type(sample_fit_type)
        if sample_polynomial_order:
            calculate_transmission_builder.set_Sample_polynomial_order(sample_polynomial_order)
        if sample_wavelength_low:
            calculate_transmission_builder.set_Sample_wavelength_low(sample_wavelength_low)
        if sample_wavelength_high:
            calculate_transmission_builder.set_Sample_wavelength_high(sample_wavelength_high)

        if can_fit_type:
            calculate_transmission_builder.set_Can_fit_type(can_fit_type)
        if can_polynomial_order:
            calculate_transmission_builder.set_Can_polynomial_order(can_polynomial_order)
        if can_wavelength_low:
            calculate_transmission_builder.set_Can_wavelength_low(can_wavelength_low)
        if can_wavelength_high:
            calculate_transmission_builder.set_Can_wavelength_high(can_wavelength_high)
        calculate_transmission = calculate_transmission_builder.build()
        state.adjustment.calculate_transmission = calculate_transmission
        return state.property_manager

    @staticmethod
    def _prepare_workspace(workspace, data=None):
        """
        Creates a test monitor workspace with 4 bins
        """
        # Rebin the workspace
        rebin_name = "Rebin"
        rebin_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME,
                         "Params": "5000,5000,25000"}
        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
        rebin_alg.execute()
        rebinned = rebin_alg.getProperty("OutputWorkspace").value

        # Now set specified monitors to specified values
        if data is not None:
            for key, value in list(data.items()):
                data_y = rebinned.dataY(key)
                for index in range(len(data_y)):
                    data_y[index] = value[index]
        return rebinned

    @staticmethod
    def _run_test(transmission_workspace, direct_workspace, state, is_sample=True):
        calculate_transmission_name = "SANSCalculateTransmission"
        calculate_transmission_options = {"TransmissionWorkspace": transmission_workspace,
                                          "DirectWorkspace": direct_workspace,
                                          "SANSState": state,
                                          "OutputWorkspace": EMPTY_NAME,
                                          "UnfittedData": "unfitted"}
        if is_sample:
            calculate_transmission_options.update({"DataType": "Sample"})
        else:
            calculate_transmission_options.update({"DataType": "Can"})

        calculate_transmission_alg = create_unmanaged_algorithm(calculate_transmission_name,
                                                                **calculate_transmission_options)
        calculate_transmission_alg.execute()
        workspace = calculate_transmission_alg.getProperty("OutputWorkspace").value
        unfitted = calculate_transmission_alg.getProperty("UnfittedData").value
        return workspace, unfitted

    def _do_assert(self, transmission_workspace, direct_workspace, unfitted_workspace, fitted_workspace,
                   trans_incident, trans_trans, direct_incident, direct_trans):
        # Perform background correction, conversion to wavelength and rebinning
        trans_incident_background_corrected = np.array(trans_incident) - trans_incident[0]
        trans_trans_background_corrected = np.array(trans_trans) - trans_trans[0]
        direct_incident_background_corrected = np.array(direct_incident) - direct_incident[0]
        direct_trans_background_corrected = np.array(direct_trans) - direct_trans[0]

        trans_incident_lambda, trans_incident_signal = get_expected_for_spectrum_n(transmission_workspace, 0,
                                                                                   trans_incident_background_corrected)
        trans_lambda, trans_signal = get_expected_for_spectrum_n(transmission_workspace, 2,
                                                                 trans_trans_background_corrected)
        direct_incident_lambda, direct_incident_signal = get_expected_for_spectrum_n(direct_workspace, 0,
                                                                                     direct_incident_background_corrected)
        direct_lambda, direct_signal = get_expected_for_spectrum_n(direct_workspace, 2,
                                                                   direct_trans_background_corrected)

        # Perform step transmission calculation
        with np.errstate(divide='ignore'):
            ratio = (trans_signal / trans_incident_signal) / (direct_signal / direct_incident_signal)
            ratio = np.nan_to_num(ratio)

        # Assert
        tolerance = 1e-8
        self.assertEqual(fitted_workspace.getNumberHistograms(),  1)
        self.assertEqual(unfitted_workspace.getNumberHistograms(),  1)
        self.assertEqual(fitted_workspace.YUnitLabel(),  "Transmission")
        self.assertEqual(unfitted_workspace.YUnitLabel(),  "Transmission")
        self.assertTrue(unfitted_workspace.isDistribution())
        self.assertTrue(fitted_workspace.isDistribution())

        for e1, e2 in zip(fitted_workspace.dataX(0), trans_lambda):
            self.assertTrue(abs(e1 - e2) < tolerance)

        for e1, e2 in zip(unfitted_workspace.dataY(0), ratio):
            self.assertTrue(abs(e1 - e2) < tolerance)
            self.assertTrue(e1 <= 1.0)  # The transmission has to be smaller or equal to 1

    def test_that_calculates_transmission_for_general_background_and_no_prompt_peak(self):
        # Arrange
        state = SANSCalculateTransmissionTest._get_state(rebin_type=RebinType.Rebin, wavelength_low=2.,
                                                         wavelength_high=8., wavelength_step=2.,
                                                         wavelength_step_type=RangeStepType.Lin,
                                                         background_TOF_general_start=5000.,
                                                         background_TOF_general_stop=10000., incident_monitor=1,
                                                         transmission_monitor=3, sample_fit_type=FitType.Linear,
                                                         sample_polynomial_order=0, sample_wavelength_low=2.,
                                                         sample_wavelength_high=8.)
        # Get a test monitor workspace with 4 bins where the first bin is the back ground
        trans_incident = [20., 220., 210., 230.]
        trans_trans = [10., 70., 50., 80.]
        direct_incident = [40., 401., 430., 420.]
        direct_trans = [30., 320., 350., 335.]
        data_transmission = {0: trans_incident, 2: trans_trans}
        transmission_workspace = SANSCalculateTransmissionTest._get_transmission_workspace(data=data_transmission)
        data_direct = {0: direct_incident, 2: direct_trans}
        direct_workspace = SANSCalculateTransmissionTest._get_transmission_workspace(data=data_direct)

        # Act
        fitted_workspace, unfitted_workspace = SANSCalculateTransmissionTest._run_test(transmission_workspace,
                                                                                       direct_workspace, state,
                                                                                       is_sample=True)
        # Assert
        self._do_assert(transmission_workspace, direct_workspace, unfitted_workspace, fitted_workspace,
                        trans_incident, trans_trans, direct_incident, direct_trans)

    def test_that_calculates_transmission_for_monitor_specific_background_and_prompt_peak_for_can(self):
        # Arrange
        incident_spectrum = 1
        transmission_spectrum = 3
        fix_for_remove_bins = 1e-6
        background_TOF_monitor_start = {str(incident_spectrum): 5000., str(transmission_spectrum): 5000.}
        background_TOF_monitor_stop = {str(incident_spectrum): 10000., str(transmission_spectrum): 10000.}
        state = SANSCalculateTransmissionTest._get_state(rebin_type=RebinType.Rebin, wavelength_low=2.,
                                                         wavelength_high=8., wavelength_step=2.,
                                                         wavelength_step_type=RangeStepType.Lin,
                                                         prompt_peak_correction_min=15000. + fix_for_remove_bins,
                                                         prompt_peak_correction_max=20000.,
                                                         background_TOF_monitor_start=background_TOF_monitor_start,
                                                         background_TOF_monitor_stop=background_TOF_monitor_stop,
                                                         incident_monitor=incident_spectrum,
                                                         transmission_monitor=transmission_spectrum,
                                                         can_fit_type=FitType.Linear,
                                                         can_polynomial_order=0, can_wavelength_low=2.,
                                                         can_wavelength_high=8.)
        # Get a test monitor workspace with 4 bins where the first bin is the back ground
        trans_incident = [20., 220., 210000000., 220.]
        trans_trans = [10., 80., 210000000., 80.]
        direct_incident = [40., 401., 210000000., 401.]
        direct_trans = [30., 320., 210000000., 320.]
        data_transmission = {0: trans_incident, 2: trans_trans}
        transmission_workspace = SANSCalculateTransmissionTest._get_transmission_workspace(data=data_transmission)
        data_direct = {0: direct_incident, 2: direct_trans}
        direct_workspace = SANSCalculateTransmissionTest._get_transmission_workspace(data=data_direct)

        # Act
        fitted_workspace, unfitted_workspace = SANSCalculateTransmissionTest._run_test(transmission_workspace,
                                                                                       direct_workspace, state,
                                                                                       is_sample=False)
        # Assert
        trans_incident[2] = trans_incident[3]
        trans_trans[2] = trans_trans[3]
        direct_incident[2] = direct_incident[3]
        direct_trans[2] = direct_trans[3]

        self._do_assert(transmission_workspace, direct_workspace, unfitted_workspace, fitted_workspace,
                        trans_incident, trans_trans, direct_incident, direct_trans)

    def test_that_can_get_transmission_for_region_of_interest_radius(self):
        # This test picks the monitor detector ids based on a radius around the centre of the detector. This is much
        # more tricky to test here and in principle the main tests should be happening in the actual
        # CalculateTransmission algorithm.
        # Arrange
        state = SANSCalculateTransmissionTest._get_state(rebin_type=RebinType.Rebin, wavelength_low=2.,
                                                         wavelength_high=8., wavelength_step=2.,
                                                         wavelength_step_type=RangeStepType.Lin,
                                                         background_TOF_general_start=5000.,
                                                         background_TOF_general_stop=10000., incident_monitor=1,
                                                         transmission_radius_on_detector=0.01,
                                                         sample_fit_type=FitType.Linear,
                                                         sample_polynomial_order=0, sample_wavelength_low=2.,
                                                         sample_wavelength_high=8.)
        # Gets the full workspace
        transmission_workspace = SANSCalculateTransmissionTest._get_roi_workspace()
        direct_workspace = SANSCalculateTransmissionTest._get_roi_workspace()

        # Act
        try:
            fitted_workspace, unfitted_workspace = SANSCalculateTransmissionTest._run_test(transmission_workspace,
                                                                                           direct_workspace, state,
                                                                                           is_sample=True)
            was_successful = True
            self.assertEqual(fitted_workspace.getNumberHistograms(),  1)
        except:  # noqa
            was_successful = False

        # Assert
        self.assertTrue(was_successful)


if __name__ == '__main__':
    unittest.main()

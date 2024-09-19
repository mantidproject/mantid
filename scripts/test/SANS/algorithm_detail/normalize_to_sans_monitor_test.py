# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace, Rebin
from sans.algorithm_detail.normalize_to_sans_monitor import normalize_to_monitor
from SANS.sans.state.StateObjects.StateNormalizeToMonitor import get_normalize_to_monitor_builder
from sans.test_helper.test_director import TestDirector


def get_expected_for_spectrum_1_case(monitor_workspace, selected_detector):
    # Expected output.
    # 1. The background correction should produce [0., 90., 90., 90.]
    # 2. Conversion to wavelength changes the x axis to approximately [1.6, 3.2, 4.7, 6.3, 7.9]
    # 3. Rebinning from creates the x steps [2, 4, 6, 8]
    # The first bin should have 0 + abs(3.2-4.0)/abs(3.2 - 4.7)*90
    # The second bin should have abs(4.0-4.7)/abs(3.2 - 4.7)*90 + abs(4.7-6.0)/abs(4.7 - 6.3)*90
    # The third bin should have abs(6.0-6.3)/abs(4.7 - 6.3)*90 + 90
    instrument = monitor_workspace.getInstrument()
    source = instrument.getSource()
    detector = monitor_workspace.getDetector(selected_detector)
    distance_source_detector = detector.getDistance(source)
    h = 6.62606896e-34
    mass = 1.674927211e-27
    times = monitor_workspace.dataX(0)
    lambda_after_unit_conversion = [(time * 1e-6) * h / distance_source_detector / mass * 1e10 for time in times]
    expected_lambda = [2.0, 4.0, 6.0, 8.0]
    expected_signal = [
        abs(lambda_after_unit_conversion[1] - expected_lambda[1])
        / abs(lambda_after_unit_conversion[1] - lambda_after_unit_conversion[2])
        * 90,
        abs(lambda_after_unit_conversion[2] - expected_lambda[1])
        / abs(lambda_after_unit_conversion[1] - lambda_after_unit_conversion[2])
        * 90
        + abs(lambda_after_unit_conversion[2] - expected_lambda[2])
        / abs(lambda_after_unit_conversion[2] - lambda_after_unit_conversion[3])
        * 90,
        abs(lambda_after_unit_conversion[3] - expected_lambda[2])
        / abs(lambda_after_unit_conversion[2] - lambda_after_unit_conversion[3])
        * 90
        + 90,
    ]
    return expected_lambda, expected_signal


class SANSNormalizeToMonitorTest(unittest.TestCase):
    def setUp(self):
        AnalysisDataService.clear()

    def tearDown(self):
        AnalysisDataService.clear()

    @staticmethod
    def _get_monitor_workspace(data=None):
        norm_to_sans_monitor_ws = CreateSampleWorkspace(NumBanks=0, NumMonitors=8)
        ws = SANSNormalizeToMonitorTest._prepare_workspace(norm_to_sans_monitor_ws, data=data)
        return ws

    @staticmethod
    def _get_state(
        background_TOF_general_start=None,
        background_TOF_general_stop=None,
        background_TOF_monitor_start=None,
        background_TOF_monitor_stop=None,
        incident_monitor=None,
        prompt_peak_correction_min=None,
        prompt_peak_correction_max=None,
    ):
        test_director = TestDirector()
        state = test_director.construct()

        data_state = state.data
        normalize_to_monitor_builder = get_normalize_to_monitor_builder(data_state)
        wav_range = (2.0, 8.0)  # Override from test director
        state.wavelength.wavelength_interval.wavelength_full_range = wav_range
        state.wavelength.wavelength_interval.selected_ranges = [wav_range]
        state.wavelength.wavelength_interval.wavelength_step = 2.0
        if background_TOF_general_start:
            normalize_to_monitor_builder.set_background_TOF_general_start(background_TOF_general_start)
        if background_TOF_general_stop:
            normalize_to_monitor_builder.set_background_TOF_general_stop(background_TOF_general_stop)
        if background_TOF_monitor_start:
            normalize_to_monitor_builder.set_background_TOF_monitor_start(background_TOF_monitor_start)
        if background_TOF_monitor_stop:
            normalize_to_monitor_builder.set_background_TOF_monitor_stop(background_TOF_monitor_stop)
        if incident_monitor:
            normalize_to_monitor_builder.set_incident_monitor(incident_monitor)
        if prompt_peak_correction_min:
            normalize_to_monitor_builder.set_prompt_peak_correction_min(prompt_peak_correction_min)
        if prompt_peak_correction_max:
            normalize_to_monitor_builder.set_prompt_peak_correction_max(prompt_peak_correction_max)
        if prompt_peak_correction_min and prompt_peak_correction_max:
            normalize_to_monitor_builder.set_prompt_peak_correction_enabled(True)

        normalize_to_monitor_state = normalize_to_monitor_builder.build()
        state.adjustment.normalize_to_monitor = normalize_to_monitor_state

        return state

    @staticmethod
    def _prepare_workspace(workspace, data=None):
        """
        Creates a test monitor workspace with 4 bins
        """
        rebinned = Rebin(InputWorkspace=workspace, Params="5000, 5000, 25000")

        # Now set specified monitors to specified values
        if data is not None:
            for key, value in list(data.items()):
                data_y = rebinned.dataY(key)
                for index in range(len(data_y)):
                    data_y[index] = value[index]

        return rebinned

    @staticmethod
    def _run_test(workspace, state, scale=1.0):
        wav_range = state.wavelength.wavelength_interval.wavelength_full_range
        output_ws = normalize_to_monitor(
            workspace=workspace,
            scale_factor=scale,
            wav_range=wav_range,
            normalize_to_monitor=state.adjustment.normalize_to_monitor,
            wavelengths=state.wavelength,
        )
        return output_ws

    def _do_assert(self, workspace, expected_monitor_spectrum, expected_lambda, expected_signal):
        # Check the units
        axis = workspace.getAxis(0)
        unit = axis.getUnit()
        self.assertEqual(unit.unitID(), "Wavelength")

        # Check the spectrum
        self.assertEqual(len(workspace.dataY(0)), 3)
        self.assertEqual(workspace.getNumberHistograms(), 1)
        single_spectrum = workspace.getSpectrum(0)
        self.assertEqual(single_spectrum.getSpectrumNo(), expected_monitor_spectrum)

        # Check the values
        tolerance = 1e-8
        for (
            e1,
            e2,
        ) in zip(workspace.dataX(0), expected_lambda):
            self.assertTrue(abs(e1 - e2) < tolerance)
        for (
            e1,
            e2,
        ) in zip(workspace.dataY(0), expected_signal):
            self.assertLess(abs(e1 - e2), tolerance)

    def test_that_gets_normalization_for_general_background_and_no_prompt_peak(self):
        # Arrange
        incident_spectrum = 1
        state = self._get_state(
            background_TOF_general_start=5000.0, background_TOF_general_stop=10000.0, incident_monitor=incident_spectrum
        )
        # Get a test monitor workspace with 4 bins where the first bin is the back ground
        data = {0: [10.0, 100.0, 100.0, 100.0]}
        monitor_workspace = SANSNormalizeToMonitorTest._get_monitor_workspace(data=data)
        # Act
        workspace = SANSNormalizeToMonitorTest._run_test(monitor_workspace, state)

        # Assert
        expected_lambda, expected_signal = get_expected_for_spectrum_1_case(monitor_workspace, selected_detector=incident_spectrum - 1)
        self._do_assert(workspace, incident_spectrum, expected_lambda, expected_signal)

    def test_that_gets_normalization_for_specific_monitor_background_and_no_prompt_peak(self):
        # Arrange
        incident_spectrum = 1
        background_TOF_monitor_start = {str(incident_spectrum): 5000.0}
        background_TOF_monitor_stop = {str(incident_spectrum): 10000.0}
        state = self._get_state(
            background_TOF_monitor_start=background_TOF_monitor_start,
            background_TOF_monitor_stop=background_TOF_monitor_stop,
            incident_monitor=incident_spectrum,
        )
        # Get a test monitor workspace with 4 bins where the first bin is the back ground
        data = {0: [10.0, 100.0, 100.0, 100.0]}
        monitor_workspace = SANSNormalizeToMonitorTest._get_monitor_workspace(data=data)
        # Act
        workspace = SANSNormalizeToMonitorTest._run_test(monitor_workspace, state)

        # Assert
        expected_lambda, expected_signal = get_expected_for_spectrum_1_case(monitor_workspace, selected_detector=incident_spectrum - 1)
        self._do_assert(workspace, incident_spectrum, expected_lambda, expected_signal)

    def test_that_gets_normalization_for_general_background_and_prompt_peak(self):
        # Arrange
        incident_spectrum = 1
        # There seems to be an issue with RemoveBins which does not like us specifying xmin or xmax on bin boundaries
        # This is a quick workaround.
        fix_for_remove_bins = 1e-6
        state = self._get_state(
            background_TOF_general_start=5000.0,
            background_TOF_general_stop=10000.0,
            prompt_peak_correction_min=15000.0 + fix_for_remove_bins,
            prompt_peak_correction_max=20000.0,
            incident_monitor=incident_spectrum,
        )
        # Get a test monitor workspace with 4 bins where the first bin is the back ground and the third bin has
        # a prompt peak which will be removed
        data = {0: [10.0, 100.0, 1000000.0, 100.0]}
        monitor_workspace = SANSNormalizeToMonitorTest._get_monitor_workspace(data=data)
        # Act
        workspace = SANSNormalizeToMonitorTest._run_test(monitor_workspace, state)

        # Assert
        expected_lambda, expected_signal = get_expected_for_spectrum_1_case(monitor_workspace, selected_detector=incident_spectrum - 1)
        self._do_assert(workspace, incident_spectrum, expected_lambda, expected_signal)


if __name__ == "__main__":
    unittest.main()

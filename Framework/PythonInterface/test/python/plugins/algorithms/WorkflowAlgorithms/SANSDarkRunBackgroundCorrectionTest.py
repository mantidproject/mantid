# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import mtd, AlgorithmManager, AnalysisDataService
from testhelpers import run_algorithm
import numpy as np
from SANSDarkRunBackgroundCorrection import DarkRunMonitorAndDetectorRemover


class SANSDarkRunBackgroundCorrectionTest(unittest.TestCase):
    # -----
    # Workspace2D tests

    def test_dark_run_correction_with_uniform_and_not_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4
        bin_boundaries_scatter = 5
        y_value_scatter_run = 4.0
        e_value_scatter_run = 1.0
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries_scatter, y_value_scatter_run, e_value_scatter_run, name_scatter, spectra)

        bin_boundaries_dark_run = 20
        y_value_dark_run = 0.3
        e_value_dark_run = 0.0
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries_dark_run, y_value_dark_run, e_value_dark_run, name_dark_run, spectra)
        # Algorithm configuration
        mean = False
        uniform = True
        normalization_ratio = 0.5

        # Act
        out_ws_name = "out_test"
        run_algorithm(
            "SANSDarkRunBackgroundCorrection",
            InputWorkspace=name_scatter,
            DarkRun=name_dark_run,
            Mean=mean,
            Uniform=uniform,
            NormalizationRatio=normalization_ratio,
            OutputWorkspace=out_ws_name,
            ApplyToDetectors=True,
            ApplyToMonitors=False,
            SelectedMonitors=[],
            rethrow=True,
        )

        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio
        # Then divide by the bins in the scatterer.
        expected_integration = y_value_dark_run * float(bin_boundaries_dark_run - 1)
        expected_correction_value = normalization_ratio * expected_integration / float(bin_boundaries_scatter - 1)

        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace(mtd[name_scatter], mtd[out_ws_name], expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    def test_dark_run_correction_with_uniform_and_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4

        bin_boundaries_scatter = 5
        y_value_scatter_run = 4.0
        e_value_scatter_run = 1.0
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries_scatter, y_value_scatter_run, e_value_scatter_run, name_scatter, spectra)

        bin_boundaries_dark_run = 20
        y_value_spectra_even_dark_run = [0.3 for element in range(bin_boundaries_dark_run - 1)]
        y_value_spectra_odd_dark_run = [0.2 for element in range(bin_boundaries_dark_run - 1)]
        y_value_dark_run = (
            y_value_spectra_even_dark_run + y_value_spectra_odd_dark_run + y_value_spectra_even_dark_run + y_value_spectra_odd_dark_run
        )
        e_value_dark_run = 0
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries_dark_run, y_value_dark_run, e_value_dark_run, name_dark_run, spectra, True)
        # Algorithm configuration
        mean = True
        uniform = True
        normalization_ratio = 0.5

        # Act
        out_ws_name = "out_test"
        run_algorithm(
            "SANSDarkRunBackgroundCorrection",
            InputWorkspace=name_scatter,
            DarkRun=name_dark_run,
            Mean=mean,
            Uniform=uniform,
            NormalizationRatio=normalization_ratio,
            OutputWorkspace=out_ws_name,
            ApplyToDetectors=True,
            ApplyToMonitors=False,
            SelectedMonitors=[],
            rethrow=True,
        )

        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio
        # Then divide by the bins in the scatterer.
        expected_integration = sum(y_value_dark_run) / float(mtd[name_dark_run].getNumberHistograms())
        expected_correction_value = normalization_ratio * expected_integration / float(bin_boundaries_scatter - 1)

        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace(mtd[name_scatter], mtd[out_ws_name], expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    def test_dark_run_correction_with_non_uniform_and_not_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4

        bin_boundaries = 5
        y_value_scatter_run = spectra * [element for element in range(bin_boundaries - 1)]
        e_value_scatter_run = 1.0
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries, y_value_scatter_run, e_value_scatter_run, name_scatter, spectra, True)

        y_value_dark_run = spectra * [element * 0.2 for element in range(bin_boundaries - 1)]
        e_value_dark_run = 0
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries, y_value_dark_run, e_value_dark_run, name_dark_run, spectra, True)
        # Algorithm configuration
        mean = False
        uniform = False
        normalization_ratio = 0.6

        # Act
        out_ws_name = "out_test"
        run_algorithm(
            "SANSDarkRunBackgroundCorrection",
            InputWorkspace=name_scatter,
            DarkRun=name_dark_run,
            Mean=mean,
            Uniform=uniform,
            NormalizationRatio=normalization_ratio,
            OutputWorkspace=out_ws_name,
            ApplyToDetectors=True,
            ApplyToMonitors=False,
            SelectedMonitors=[],
            rethrow=True,
        )

        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio
        # Then divide by the bins in the scatterer.
        expected_correction_value = normalization_ratio
        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace_non_uniform(mtd[name_scatter], mtd[out_ws_name], mtd[name_dark_run], expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    def test_that_only_monitors_are_corrected_if_only_monitors_should_be_corrected(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)
        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)

        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = True
        applyToDetectors = False
        out_ws_name = "out_test"
        selected_monitor = []
        # Act
        ws = self._do_run_dark_subtraction(
            scatter_ws, dark_run, mean, uniform, normalization_ratio, out_ws_name, applyToMonitors, applyToDetectors, selected_monitor
        )

        # Assert
        self.assertAlmostEqual(ws.getNumberHistograms(), scatter_ws.getNumberHistograms(), 5)

        # Expected value for monitors
        expected_monitor_Y = monY_scatter - monY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        self._comparison(ws.dataY(0), expected_monitor_Y)
        self._comparison(ws.dataY(1), expected_monitor_Y)

        # Expected value for detectors
        expected_detector_Y = dataY_scatter
        for index in range(2, ws.getNumberHistograms()):
            self._comparison(ws.dataY(index), expected_detector_Y)

    def test_that_individual_monitor_is_corrected_if_only_individual_monitor_is_chosen(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)
        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)

        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = True
        applyToDetectors = False
        out_ws_name = "out_test"
        selected_monitor = [2]
        # Act
        ws = self._do_run_dark_subtraction(
            scatter_ws, dark_run, mean, uniform, normalization_ratio, out_ws_name, applyToMonitors, applyToDetectors, selected_monitor
        )

        # Assert
        self.assertAlmostEqual(ws.getNumberHistograms(), scatter_ws.getNumberHistograms(), 5)

        # Expected value for monitor 2 -- workspace index 1
        expected_monitor_Y_1 = monY_scatter - monY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        self._comparison(ws.dataY(1), expected_monitor_Y_1)

        # Expected value for monitor 1  -- workspace index 0
        expected_monitor_Y_0 = monY_scatter
        self._comparison(ws.dataY(0), expected_monitor_Y_0)

        # Expected value for detectors
        expected_detector_Y = dataY_scatter
        for index in range(2, ws.getNumberHistograms()):
            self._comparison(ws.dataY(index), expected_detector_Y)

    def test_that_selecting_monitors_and_detectors_is_allowed(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)
        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)

        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = True
        applyToDetectors = True
        out_ws_name = "out_test"
        selected_monitor = []
        # Act
        ws = self._do_run_dark_subtraction(
            scatter_ws, dark_run, mean, uniform, normalization_ratio, out_ws_name, applyToMonitors, applyToDetectors, selected_monitor
        )

        # Assert
        self.assertAlmostEqual(ws.getNumberHistograms(), scatter_ws.getNumberHistograms(), 5)

        # Expected value for monitors
        expected_monitor_Y = monY_scatter - monY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        self._comparison(ws.dataY(1), expected_monitor_Y)
        self._comparison(ws.dataY(0), expected_monitor_Y)

        # Expected value for detectors
        expected_detector_Y = dataY_scatter - dataY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        for index in range(2, ws.getNumberHistograms()):
            self._comparison(ws.dataY(index), expected_detector_Y)

    def test_that_selecting_invidual_monitors_and_detectors_is_allowed(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)
        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)
        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = True
        applyToDetectors = True
        out_ws_name = "out_test"
        selected_monitor = [2]
        # Act
        ws = self._do_run_dark_subtraction(
            scatter_ws, dark_run, mean, uniform, normalization_ratio, out_ws_name, applyToMonitors, applyToDetectors, selected_monitor
        )

        # Assert
        self.assertAlmostEqual(ws.getNumberHistograms(), scatter_ws.getNumberHistograms(), 5)

        # Expected value for monitor 2 -- workspace index 1
        expected_monitor_Y_1 = monY_scatter - monY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        self._comparison(ws.dataY(1), expected_monitor_Y_1)

        # Expected value for monitor 1 -- workspace index 0
        expected_monitor_Y_0 = monY_scatter
        self._comparison(ws.dataY(0), expected_monitor_Y_0)

        # Expected value for detectors
        expected_detector_Y = dataY_scatter - dataY_dark * len(dark_run.dataY(0)) / len(scatter_ws.dataY(0)) * normalization_ratio
        for index in range(2, ws.getNumberHistograms()):
            self._comparison(ws.dataY(index), expected_detector_Y)

    def test_that_throws_if_monitor_selection_is_invalid(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)

        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)

        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = True
        applyToDetectors = False
        selected_monitor = [3]  # only has det IDs 1 and 2 as monitors
        # Act + Assert
        kwds = {
            "InputWorkspace": scatter_ws,
            "DarkRun": dark_run,
            "NormalizationRatio": normalization_ratio,
            "Mean": mean,
            "Uniform": uniform,
            "ApplyToDetectors": applyToDetectors,
            "ApplyToMonitors": applyToMonitors,
            "SelectedMonitors": selected_monitor,
            "OutputWorkspace": "out_ws",
        }

        scatter_name = "scatter_workspace_test"
        dark_name = "dark_workspace_test"

        AnalysisDataService.add(scatter_name, scatter_ws)
        AnalysisDataService.add(dark_name, dark_run)
        self.assertRaisesRegex(
            RuntimeError,
            "The selected monitors are not part of the workspace.",
            run_algorithm,
            "SANSDarkRunBackgroundCorrection",
            rethrow=True,
            **kwds,
        )

        # Clean up
        ws_to_clean = [scatter_name, dark_name]
        self._clean_up(ws_to_clean)

    def test_that_throws_if_neither_monitor_nor_detectors_are_selected(self):
        # Arrange
        monY_scatter = 1.0
        monE_scatter = 1.0
        dataY_scatter = 2.0
        dataE_scatter = 2.0
        scatter_ws = self._load_workspace_with_monitors(monY_scatter, monE_scatter, dataY_scatter, dataE_scatter, as_dark_run=False)
        monY_dark = 3.0
        monE_dark = 3.0
        dataY_dark = 4.0
        dataE_dark = 4.0
        dark_run = self._load_workspace_with_monitors(monY_dark, monE_dark, dataY_dark, dataE_dark, as_dark_run=True)
        mean = False
        uniform = True
        normalization_ratio = 0.6
        applyToMonitors = False
        applyToDetectors = False
        selected_monitor = []
        # Act + Assert
        kwds = {
            "InputWorkspace": scatter_ws,
            "DarkRun": dark_run,
            "NormalizationRatio": normalization_ratio,
            "Mean": mean,
            "Uniform": uniform,
            "ApplyToDetectors": applyToDetectors,
            "ApplyToMonitors": applyToMonitors,
            "SelectedMonitors": selected_monitor,
            "OutputWorkspace": "out_ws",
        }

        scatter_name = "scatter_workspace_test"
        dark_name = "dark_workspace_test"

        AnalysisDataService.add(scatter_name, scatter_ws)
        AnalysisDataService.add(dark_name, dark_run)
        self.assertRaisesRegex(
            RuntimeError,
            "Must provide either ApplyToDetectors or ApplyToMonitors or both",
            run_algorithm,
            "SANSDarkRunBackgroundCorrection",
            rethrow=True,
            **kwds,
        )

        # Clean up
        ws_to_clean = [scatter_name, dark_name]
        self._clean_up(ws_to_clean)

    # ------
    # Helper methods
    def _create_test_workspace(self, name, x, y, error, number_of_spectra):
        alg = run_algorithm("CreateWorkspace", DataX=x, DataY=y, DataE=error, NSpec=number_of_spectra, OutputWorkspace=name)
        return alg.getPropertyValue("OutputWorkspace")

    def _check_output_workspace(self, original_ws, corrected_ws, expected_correction_value):
        # Iterate over all spectra
        num_spectra = original_ws.getNumberHistograms()

        for index in range(0, num_spectra):
            y_original = original_ws.dataY(index)
            y_corrected = corrected_ws.dataY(index)
            for elem in range(0, len(y_original)):
                expected = y_original[elem] - expected_correction_value
                self.assertAlmostEqual(expected, y_corrected[elem], 4)

    def _do_run_dark_subtraction(
        self, scatter, dark_run, mean, uniform, normalization_ratio, out_ws_name, applyToMonitors, applyToDetectors, selected_monitor
    ):
        alg_dark = AlgorithmManager.createUnmanaged("SANSDarkRunBackgroundCorrection")
        alg_dark.initialize()
        alg_dark.setChild(True)
        alg_dark.setProperty("InputWorkspace", scatter)
        alg_dark.setProperty("DarkRun", dark_run)
        alg_dark.setProperty("Mean", mean)
        alg_dark.setProperty("Uniform", uniform)
        alg_dark.setProperty("NormalizationRatio", normalization_ratio)
        alg_dark.setProperty("OutputWorkspace", out_ws_name)
        alg_dark.setProperty("ApplyToMonitors", applyToMonitors)
        alg_dark.setProperty("ApplyToDetectors", applyToDetectors)
        alg_dark.setProperty("SelectedMonitors", selected_monitor)
        alg_dark.execute()

        return alg_dark.getProperty("OutputWorkspace").value

    def _check_output_workspace_non_uniform(self, original_ws, corrected_ws, dark_ws, expected_correction_value):
        # Iterate over all spectra
        num_spectra = original_ws.getNumberHistograms()

        for index in range(0, num_spectra):
            y_original = original_ws.dataY(index)
            y_dark = dark_ws.dataY(index)
            y_corrected = corrected_ws.dataY(index)
            for elem in range(0, len(y_original)):
                expected = y_original[elem] - y_dark[elem] * expected_correction_value
                self.assertAlmostEqual(expected, y_corrected[elem], 4)

    def _provide_workspace2D(self, bin_boundaries, y_value, e_value, name, spectra, use_y_list=False):
        x = spectra * [element for element in range(bin_boundaries)]
        y = None
        if use_y_list:
            y = y_value
        else:
            y = spectra * [y_value for element in range(bin_boundaries - 1)]
        e = spectra * [e_value for element in range(bin_boundaries - 1)]
        self._create_test_workspace(name, x, y, e, spectra)

    def _comparison(self, data, expected):
        return all([self.assertAlmostEqual(data[i], expected, 5, "Should be equal") for i in range(0, len(data))])

    def _clean_up(self, ws_to_clean):
        for ws in ws_to_clean:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)

    def _load_workspace_with_monitors(self, monY, monE, dataY, dataE, as_dark_run=False):
        filename = "LOQ48127np.nxs"
        out_ws_name = "sans_workspace_test"
        if as_dark_run:
            out_ws_name = "dark_run_workspace_test"

        alg_load = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
        alg_load.initialize()
        alg_load.setChild(True)
        alg_load.setProperty("Filename", filename)
        alg_load.setProperty("OutputWorkspace", out_ws_name)
        alg_load.execute()
        ws = alg_load.getProperty("OutputWorkspace").value

        if as_dark_run:
            ws.setY(0, ws.dataY(0) * 0.0 + monY)
            ws.setE(0, ws.dataE(0) * 0.0 + monE)
            ws.setY(1, ws.dataY(1) * 0.0 + monY)
            ws.setE(1, ws.dataE(1) * 0.0 + monE)

            for element in range(2, ws.getNumberHistograms()):
                ws.setY(element, ws.dataY(element) * 0.0 + dataY)
                ws.setE(element, ws.dataE(element) * 0.0 + dataE)

        else:
            ws.setY(0, ws.dataY(0) * 0.0 + monY)
            ws.setE(0, ws.dataE(0) * 0.0 + monE)
            ws.setY(1, ws.dataY(1) * 0.0 + monY)
            ws.setE(1, ws.dataE(1) * 0.0 + monE)

            # Set the detector Y and E to 4 and 0.4
            for element in range(2, ws.getNumberHistograms()):
                ws.setY(element, ws.dataY(element) * 0.0 + dataY)
                ws.setE(element, ws.dataE(element) * 0.0 + dataE)

        return ws


class DarkRunMonitorAndDetectorRemoverTest(unittest.TestCase):
    def test_finds_all_monitor_indices_when_monitor_is_present(self):
        # Arrange
        test_ws = self._load_workspace_with_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        # Act

        indices = remover.find_monitor_workspace_indices(ws)

        # Assert
        ws_index, det_ids = zip(*indices)
        self.assertEqual(len(indices), 2, "There should be two monitors")
        self.assertEqual(ws_index[0], 0, "The first monitor should have a workspace index of 0")
        self.assertEqual(ws_index[1], 1, "The second monitor should have a workspace index of 1")
        self.assertEqual(det_ids[0], 1, "The first monitor should have a detector ID of 1")
        self.assertEqual(det_ids[1], 2, "The second monitor should have a detector ID of 2")
        # Clean up
        ws_to_clean = [test_ws]
        self._clean_up(ws_to_clean)

    def test_find_no_monitors_when_no_monitors_are_present(self):
        # Arrange
        test_ws = self._load_workspace_without_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        # Act
        indices = remover.find_monitor_workspace_indices(ws)

        # Assert
        self.assertEqual(len(indices), 0, "There should be no monitors")

        # Clean up
        ws_to_clean = [test_ws]
        self._clean_up(ws_to_clean)

    def test_keep_all_monitors_discard_detectors(self):
        # Arrange
        test_ws = self._load_workspace_with_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        dataY0_reference = np.copy(ws.dataY(0))
        dataE0_reference = np.copy(ws.dataE(0))
        dataY1_reference = np.copy(ws.dataY(1))
        dataE1_reference = np.copy(ws.dataE(1))
        number_histograms_reference = ws.getNumberHistograms()
        zero_reference = dataY0_reference * 0

        # Act
        monitor_selection = []
        dark_run_corrected = remover.set_pure_monitor_dark_run(ws, monitor_selection)

        # Assert
        self.assertEqual(
            dark_run_corrected.getNumberHistograms(), number_histograms_reference, "The number of histograms should not have changed"
        )

        self._assert_items_are_equal(dark_run_corrected.dataY(0), dataY0_reference, "First monitor Y data should not have changed")
        self._assert_items_are_equal(dark_run_corrected.dataE(0), dataE0_reference, "First monitor E data should not have changed")

        self._assert_items_are_equal(dark_run_corrected.dataY(1), dataY1_reference, "Second monitor Y data should not have changed")
        self._assert_items_are_equal(dark_run_corrected.dataE(1), dataE1_reference, "Second monitor E data should not have changed")

        for element in range(2, dark_run_corrected.getNumberHistograms()):
            self._assert_items_are_equal(
                dark_run_corrected.dataY(element), zero_reference, "The Y data of non-monitor detectors should be 0"
            )
            self._assert_items_are_equal(
                dark_run_corrected.dataE(element), zero_reference, "The E data of non-monitor detectors should be 0"
            )

        # Clean up
        ws_to_clean = [test_ws]
        self._clean_up(ws_to_clean)

    def test_keep_all_detectors_discard_monitors(self):
        # Arrange
        test_ws = self._load_workspace_with_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        ref_ws = ws.clone()
        zero_reference = ref_ws.dataY(0) * 0

        # Act
        dark_run_corrected = remover.set_pure_detector_dark_run(ws)

        # Assert
        self.assertEqual(
            dark_run_corrected.getNumberHistograms(), ref_ws.getNumberHistograms(), "The number of histograms should not have changed"
        )

        self._assert_items_are_equal(dark_run_corrected.dataY(0), zero_reference, "First monitor Y data should be 0")
        self._assert_items_are_equal(dark_run_corrected.dataE(0), zero_reference, "First monitor E data should be 0")

        self._assert_items_are_equal(dark_run_corrected.dataY(1), zero_reference, "Second monitor Y data should be 0")
        self._assert_items_are_equal(dark_run_corrected.dataE(1), zero_reference, "Second monitor E data should be 0")

        for element in range(2, dark_run_corrected.getNumberHistograms()):
            self._assert_items_are_equal(
                dark_run_corrected.dataY(element), ref_ws.dataY(element), "The Y data of non-monitor detectors should not have changed"
            )
            self._assert_items_are_equal(
                dark_run_corrected.dataE(element), ref_ws.dataE(element), "The E data of non-monitor detectors should not have changed"
            )

        # Clean up
        ws_to_clean = [test_ws, "ref_ws"]
        self._clean_up(ws_to_clean)

    def test_that_individual_monitors_can_be_selected(self):
        # Arrange
        test_ws = self._load_workspace_with_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        zero_reference = np.copy(ws.dataY(0)) * 0
        dataY0_reference = np.copy(ws.dataY(0))
        dataE0_reference = np.copy(ws.dataE(0))
        number_histograms_reference = ws.getNumberHistograms()

        monitor_selection = [1]  # We select the monitor with detector ID 1
        # which is workspace index 0 for this workspace

        # Act
        dark_run_corrected = remover.set_pure_monitor_dark_run(ws, monitor_selection)

        # Assert
        self.assertEqual(
            dark_run_corrected.getNumberHistograms(), number_histograms_reference, "The number of histograms should not have changed"
        )

        self._assert_items_are_equal(dark_run_corrected.dataY(0), dataY0_reference, "First monitor Y data should be 0")

        self._assert_items_are_equal(dark_run_corrected.dataE(0), dataE0_reference, "First monitor E data should be 0")

        self._assert_items_are_equal(dark_run_corrected.dataY(1), zero_reference, "Second monitor Y data should not have changed")
        self._assert_items_are_equal(dark_run_corrected.dataE(1), zero_reference, "Second monitor E data should not have changed")

        for element in range(2, dark_run_corrected.getNumberHistograms()):
            self._assert_items_are_equal(
                dark_run_corrected.dataY(element), zero_reference, "The Y data of non-monitor detectors should be 0"
            )
            self._assert_items_are_equal(
                dark_run_corrected.dataE(element), zero_reference, "The E data of non-monitor detectors should be 0"
            )

        # Clean up
        ws_to_clean = [test_ws]
        self._clean_up(ws_to_clean)

    def test_that_throws_if_selection_does_not_match_available_monitor_list(self):
        # Arrange
        test_ws = self._load_workspace_with_monitors()
        ws = mtd[test_ws]
        remover = DarkRunMonitorAndDetectorRemover()

        monitor_selection = [0, 2]

        # Act+ Assert
        args = [ws, monitor_selection]
        self.assertRaisesRegex(
            RuntimeError, "The selected monitors are not part of the workspace.", remover.set_pure_monitor_dark_run, *args
        )

        # Clean up
        ws_to_clean = [test_ws]
        self._clean_up(ws_to_clean)

    def _load_workspace_with_monitors(self):
        filename = "LOQ48127np.nxs"
        out_ws_name = "dark_run_monitor_test_ws"
        alg = run_algorithm("LoadNexusProcessed", Filename=filename, OutputWorkspace=out_ws_name, rethrow=True)
        return alg.getPropertyValue("OutputWorkspace")

    def _load_workspace_without_monitors(self):
        out_ws_name = "dark_run_monitor_test_ws"
        alg = run_algorithm("CreateSampleWorkspace", OutputWorkspace=out_ws_name, rethrow=True)
        return alg.getPropertyValue("OutputWorkspace")

    def _clean_up(self, ws_to_clean):
        for ws in ws_to_clean:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)

    def _assert_items_are_equal(self, list1, list2, message):
        # This method is needed since RHEL6 cannot handle assertItemsEqual
        for index in range(0, len(list1)):
            self.assertEqual(list1[index], list2[index], message)


if __name__ == "__main__":
    unittest.main()

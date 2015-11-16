import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
import time

class SANSDarkRunBackgroundCorrectionTest(unittest.TestCase):
    #-----
    # Workspace2D tests
    def test_dark_run_correction_with_uniform_and_not_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4

        bin_boundaries_scatter = 5
        y_value_scatter_run = 4.
        e_value_scatter_run = 1.
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries_scatter, y_value_scatter_run,
                                  e_value_scatter_run,name_scatter, spectra)

        bin_boundaries_dark_run = 20
        y_value_dark_run = 0.3
        e_value_dark_run = 0.
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries_dark_run, y_value_dark_run,
                                  e_value_dark_run,name_dark_run, spectra)
        # Algorithm configuration
        mean = False
        uniform = True
        normalization_ratio = 0.5

        # Act
        out_ws_name = "out_test"
        alg = run_algorithm(
                    'SANSDarkRunBackgroundCorrection',
                    InputWorkspace= name_scatter,
                    DarkRun = name_dark_run,
                    Mean = mean,
                    Uniform =uniform,
                    NormalizationRatio=normalization_ratio, 
                    OutputWorkspace = out_ws_name,
                    rethrow = True)
        
        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio 
        # Then divide by the bins in the scatterer.
        expected_integration = y_value_dark_run* float(bin_boundaries_dark_run - 1)
        expected_correction_value = (normalization_ratio*expected_integration/float(bin_boundaries_scatter - 1))

        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace(mtd[name_scatter],
                                     mtd[out_ws_name],
                                     expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    def test_dark_run_correction_with_uniform_and_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4

        bin_boundaries_scatter = 5
        y_value_scatter_run = 4.
        e_value_scatter_run = 1.
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries_scatter, y_value_scatter_run,
                                    e_value_scatter_run,name_scatter, spectra)

        bin_boundaries_dark_run = 20
        y_value_spectra_even_dark_run = [0.3 for element in xrange(bin_boundaries_dark_run - 1)]
        y_value_spectra_odd_dark_run = [0.2 for element in xrange(bin_boundaries_dark_run - 1)]
        y_value_dark_run = (y_value_spectra_even_dark_run + y_value_spectra_odd_dark_run + 
                            y_value_spectra_even_dark_run + y_value_spectra_odd_dark_run)
        e_value_dark_run = 0
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries_dark_run, y_value_dark_run,
                                                e_value_dark_run,name_dark_run, spectra, True)
        # Algorithm configuration
        mean = True
        uniform = True
        normalization_ratio = 0.5

        # Act
        out_ws_name = "out_test"
        alg = run_algorithm(
                    'SANSDarkRunBackgroundCorrection',
                    InputWorkspace= name_scatter,
                    DarkRun = name_dark_run,
                    Mean = mean,
                    Uniform =uniform,
                    NormalizationRatio=normalization_ratio,
                    OutputWorkspace = out_ws_name,
                    rethrow = True)

        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio
        # Then divide by the bins in the scatterer.
        expected_integration = sum(y_value_dark_run)/float(mtd[name_dark_run].getNumberHistograms())
        expected_correction_value = (normalization_ratio*expected_integration/float(bin_boundaries_scatter - 1))

        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace(mtd[name_scatter],
                                     mtd[out_ws_name],
                                     expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    def test_dark_run_correction_with_non_uniform_and_not_mean_for_workspace2D_input(self):
        # Arrange
        spectra = 4

        bin_boundaries= 5
        y_value_scatter_run = spectra*[element for element in xrange(bin_boundaries-1)]
        e_value_scatter_run = 1.
        name_scatter = "_scatter_SANS_test"
        self._provide_workspace2D(bin_boundaries, y_value_scatter_run,
                                    e_value_scatter_run,name_scatter, spectra, True)

        y_value_dark_run = spectra*[element*0.2 for element in xrange(bin_boundaries - 1)]
        e_value_dark_run = 0
        name_dark_run = "_dark_run_SANS_test"
        self._provide_workspace2D(bin_boundaries, y_value_dark_run,
                                  e_value_dark_run, name_dark_run, spectra, True)
        # Algorithm configuration
        mean = False
        uniform = False
        normalization_ratio = 0.6

        # Act
        out_ws_name = "out_test"
        alg = run_algorithm(
                    'SANSDarkRunBackgroundCorrection',
                    InputWorkspace= name_scatter,
                    DarkRun = name_dark_run,
                    Mean = mean,
                    Uniform =uniform,
                    NormalizationRatio=normalization_ratio, 
                    OutputWorkspace = out_ws_name,
                    rethrow = True)
        
        # Assert
        # We should sum up all bins in the dark run (all y values, hence bin_boundaries_dark_run - 1).
        # Then multpliy by the normalization ratio 
        # Then divide by the bins in the scatterer.
        expected_correction_value = normalization_ratio
        self.assertTrue(AnalysisDataService.doesExist(out_ws_name))
        self._check_output_workspace_non_uniform(mtd[name_scatter],
                                                 mtd[out_ws_name],
                                                 mtd[name_dark_run],
                                                 expected_correction_value)

        # Clean up
        ws_to_clean = [out_ws_name, name_dark_run, name_scatter]
        self._clean_up(ws_to_clean)

    #------
    # Helper methods
    def _create_test_workspace(self, name, x, y, error, number_of_spectra):
        alg = run_algorithm('CreateWorkspace',
                            DataX = x,
                            DataY = y,
                            DataE = error,
                            NSpec = number_of_spectra,
                            OutputWorkspace= name)
        return alg.getPropertyValue("OutputWorkspace")

    def _check_output_workspace(self, original_ws, corrected_ws, expected_correction_value):
        # Iterate over all spectra
        num_spectra = original_ws.getNumberHistograms()

        for index in range(0, num_spectra):
            y_original = original_ws.dataY(index)
            y_corrected = corrected_ws.dataY(index)
            for elem in range(0, len(y_original)):
                expected = y_original[elem] - expected_correction_value
                self.assertAlmostEqual(expected,
                                       y_corrected[elem], 4)

    def _check_output_workspace_non_uniform(self, original_ws, corrected_ws,
                                            dark_ws, expected_correction_value):
        # Iterate over all spectra
        num_spectra = original_ws.getNumberHistograms()

        for index in range(0, num_spectra):
            y_original = original_ws.dataY(index)
            y_dark = dark_ws.dataY(index)
            y_corrected = corrected_ws.dataY(index)
            for elem in range(0, len(y_original)):
                expected = y_original[elem] - y_dark[elem]*expected_correction_value
                self.assertAlmostEqual(expected, y_corrected[elem], 4)

    def _provide_workspace2D(self, bin_boundaries, y_value, e_value, name, spectra, use_y_list = False):
        x = spectra*[element for element in xrange(bin_boundaries)]
        y = None
        if use_y_list:
            y = y_value
        else:
            y = spectra*[y_value for element in xrange(bin_boundaries - 1)]
        e = spectra*[e_value for element in xrange(bin_boundaries - 1)]
        self._create_test_workspace(name, x, y, e, spectra)

    def _clean_up(self, ws_to_clean):
        for ws in ws_to_clean:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)


class DarkRunMonitorAndDetectorRemover(unittest.TestCase):
    def _create_workspace_with_monitors_and_detectors(self):
        pass

    def test_selects_all_monitors(self):
        pass

    def test_selects_specific_monitor(self):
        pass

if __name__ == '__main__':
    unittest.main()
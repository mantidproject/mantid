# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.simpleapi import AnalysisDataService, ClipPeaks, CreateWorkspace, DeleteWorkspace, DeleteWorkspaces


class ClipPeaksTest(unittest.TestCase):
    # Sets the max length of the x-axis
    length = np.pi * 2.0
    # How many data points are used between [1,length]
    resolution = 1000
    # How many peaks are randomly added to the baseline
    npeaks = 10
    # Controls the size of peaks
    peak_amplitude = 1.0
    # Min/Max border (from the resolution) that a peak can be placed on
    peak_border_lim = 50
    # Specify a specific seed to used for adding peaks.  Set to 'None' to get a new seed.
    #rand_seed = 57689
    rand_seed = None

    # Whether to add random noise to the base function (with normal distribution)
    add_background_noise = True
    # Seed to be used for adding random background noise. 'None' for a new seed each time.
    noise_seed = rand_seed
    # Mean for the normal dist. used to add noise
    noise_mean = 0.0
    # Std dev. for the normal dist. used in adding noise
    noise_scale = 0.1
    # Tolerance (rel err) accepted
    tolerance = 6e-02

    def setUp(self):
        return

    def tearDown(self):
        return

    def __setupTestWS(self):
        """
        Creates the workspace for testing, sets up a simple baseline log+sin^2 function.
        """
        x = np.linspace(start=1.0, stop=self.length, num=self.resolution)
        y = np.log(x) + np.square(np.sin(x))

        # Save the original function to use for validation.
        CreateWorkspace(OutputWorkspace="Baseline", DataX=x, DataY=y, NSpec=1)

    def __addNoise(self, mean=0.0, scale=0.1):
        """
        Add some random background noise to the baseline using a normal distribution
        """
        noise_gen = np.random.default_rng(self.noise_seed)
        self.assertTrue(AnalysisDataService.doesExist("Baseline"))
        basews = AnalysisDataService.retrieve("Baseline")
        y = basews.extractY()
        y = np.add(y, noise_gen.normal(mean, scale, self.resolution))
        basews.setY(y)

    def __createRandPeaksWS(self, amplitude=1.0):
        """
        Creates a test WS with random peaks added to the baseline function
        """
        # Create a new generator, get a permutation of indices used to add peaks to the data.
        gen = np.random.default_rng(self.rand_seed)
        peaklist = np.random.randint(self.peak_border_lim, self.resolution - self.peak_border_lim, self.npeaks)

        self.assertTrue(AnalysisDataService.doesExist("Baseline"))
        basews = AnalysisDataService.retrieve("Baseline")
        x = basews.extractX()
        y = basews.extractY()

        # Add a simple peak to the indices chosen by the random permutation
        for i in peaklist:
            y[0][i] = y[0][i] + amplitude * np.abs(np.sin(x[0][i]))

        CreateWorkspace(DataX=x, DataY=y, OutputWorkspace="PeakData")

    def testNoNans(self):
        # If y field is 0, then make sure this doesn't return nans
        x = np.linspace(1.0, 100.0, 10)
        y = np.zeros(10)
        test = CreateWorkspace(DataX=x, DataY=y)
        res = ClipPeaks(test)
        self.assertTrue(np.allclose(res.extractY(), 0.0))
        DeleteWorkspace(test)

    def testNoBackgroundNoiseDefaults(self):
        self.__setupTestWS()
        self.__createRandPeaksWS(self.peak_amplitude)

        basews = AnalysisDataService.retrieve("Baseline")
        peakws = AnalysisDataService.retrieve("PeakData")

        clippedws = ClipPeaks(peakws, LLSCorrection=True, IncreasingWindow=False, SmoothingRange=10,
                              WindowSize=10, OutputWorkspace="clipout")

        # Validate by subtracting peak clip results with baseline function
        np.testing.assert_allclose(clippedws.extractY(), basews.extractY(), rtol=self.tolerance)

        DeleteWorkspaces(WorkspaceList=["Baseline", "PeakData", "clipout"])


if __name__ == '__main__':
    unittest.main()

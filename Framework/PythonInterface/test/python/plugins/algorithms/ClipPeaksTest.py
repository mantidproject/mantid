# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.simpleapi import ClipPeaks, CreateWorkspace, Subtract


class ClipPeaksTest(unittest.TestCase):
    # Sets the max length of the x-axis
    length = np.pi * 2.0
    # How many data points are used between [1,length]
    resolution = 1000
    # How many peaks are randomly added to the baseline
    npeaks = 50
    # Controls the size of peaks
    peak_amplitude = 1.0
    # Specify a specific seed to used for adding peaks.  Set to 'None' to get a new seed.
    rand_seed = 57689

    # Whether to add random noise to the base function (with normal distribution)
    add_background_noise = True
    # Seed to be used for adding random background noise. 'None' for a new seed each time.
    noise_seed = rand_seed
    # Mean for the normal dist. used to add noise
    noise_mean = 0.0
    # Std dev. for the normal dist. used in adding noise
    noise_scale = 0.1

    def setUp(self):
        return

    def tearDown(self):
        return

    def testClipPeakBasic(self):
        x = np.linspace(start=1, stop=self.length, num=self.resolution)
        y = np.log(x) + np.square(np.sin(x))

        # Save the original function to use for validation.
        BaselineData = CreateWorkspace(DataX=x, DataY=y, WorkspaceTitle="FakeBaselineData")

        if self.add_background_noise:
            # Add some random background noise using a normal distribution
            noise_gen = np.random.default_rng(self.noise_seed)
            y = np.add(y, noise_gen.normal(self.noise_mean, self.noise_scale, self.resolution))

        # Create a new generator, get a permutation of indices used to add peaks to the data.
        gen = np.random.default_rng(self.rand_seed)
        noise = gen.choice(self.resolution, self.npeaks)
        # print(noise)

        # Add a simple peak to the indices chosen by the random permutation
        for i in noise:
            y[i] = y[i] + self.peak_amplitude * np.abs(np.sin(x[i]))

        NoiseData = CreateWorkspace(DataX=x, DataY=y, WorkspaceTitle="FakeDataWithPeaks")

        PeakClippedData = ClipPeaks(NoiseData, LLSCorrection=False, IncreasingWindow=True, SmoothingRange=10,
                                    WindowSize=20)

        # Validate by subtracting peak clip results with baseline
        Diff = Subtract(PeakClippedData, BaselineData)

if __name__ == '__main__':
    unittest.main()

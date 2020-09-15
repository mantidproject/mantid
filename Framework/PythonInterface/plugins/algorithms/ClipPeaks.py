# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, WorkspaceProperty
from mantid.simpleapi import *
from mantid.kernel import Direction, IntBoundedValidator

import numpy as np


class ClipPeaks(PythonAlgorithm):
    """
    Removes large peaks from data, trying to leave background behind.

    This algorithm is extracted from the SNAPReduce Normalization option.
    """

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input),
            "The workspace containing the normalization data.")

        self.declareProperty(
            "LLSCorrection", True,
            "Whether to apply a log-log-sqrt transformation to make data more sensitive to weaker peaks."
        )

        self.declareProperty(
            "IncreasingWindow", False,
            "Use an increasing moving window when clipping."
        )

        self.declareProperty(
            name="SmoothingRange",
            defaultValue=10,
            validator=IntBoundedValidator(lower=0),
            doc="The size of the window used for smoothing data. No smoothing if set to 0."
        )

        self.declareProperty(
            name="WindowSize",
            defaultValue=10,
            validator=IntBoundedValidator(lower=0),
            doc="The size of the peak clipping window to be used."
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "The workspace containing the normalization data.")

    def category(self):
        return "Diffraction\\Corrections"

    def seeAlso(self):
        return ["StripPeaks", "StripVanadiumPeaks"]

    def name(self):
        return "ClipPeaks"

    def summary(self):
        return "Removes peaks from the input data, providing an estimation of the background in spectra."

    def smooth(self, data, order):
        """
        This smooths data based on linear weighted average around
        point i for example for an order of 7 the i point is
        weighted 4, i=+/- 1 weighted 3, i+/-2 weighted 2 and i+/-3
        weighted 1 this input is only the y values. See the equation below:

        Y_{i}^{s} = \\frac{ \\sum_{r=-r_{max}}^{r_{max}} (r_{max} - \\vert r \\vert ) \\cdot Y_{i+r} }
                         { \\sum_{r=-r_{max}}^{r_{max}} (r_{max} - \\vert r \\vert ) }
        where r_{max} is the "order" parameter (smoothing factor).

        :param data: Input numpy.ndarray to smooth (Y data of the input workspace).
        :param order: Smoothing factor, assumed an odd integer (even factors will be rounded to the next odd).
        """
        sm = np.zeros(len(data))
        factor = int(order / 2) + 1

        for i in range(len(data)):
            temp = 0
            ave = 0
            for r in range(max(0, i - int(order / 2)), min(i + int(order / 2), len(data) - 1) + 1):
                temp = temp + (factor - abs(r - i)) * data[r]
                ave = ave + factor - abs(r - i)
            sm[i] = temp / ave

        return sm

    def log_log_sqrt_transformation(self, input):
        """
        This transforms data to be more sensitive to weak peaks. The
        function is reversed by the inv_log_log_sqrt_transformation function below
        """
        out = np.log(np.log((input + 1) ** 0.5 + 1) + 1)

        return out

    def inv_log_log_sqrt_transformation(self, input):
        """
        Applies the inverse log-log-sqrt function on the input data. See Function LLS function above
        """
        out = (np.exp(np.exp(input) - 1) - 1) ** 2 - 1

        return out

    def peak_clip(self, data, win=30, decrease=True, lls=True, smooth_window=0):
        """
        Performs peak clipping on the data iteratively over the size of win. The input data can be optionally
        smoothed if smooth_window is greater than 0. The LLS function can be performed on the data after smoothing
        to help exaggerate smaller peaks present in the data before clipping. The peak clipping window used for
        iteration is done with a decreasing window unless decrease is False.

        :param data: Input (Y) data to clip.
        :param win: Window size of the clipping (controls the iteration count of the clipping function).
        :param decrease: Flag to determine if an increasing/decreasing window is applied to the data.
        :param lls: Flag to enable/disable applying the LLS function to data before clipping.
        :param smooth_window: Smoothing factor for the smoothing function. 0 to apply no smoothing.
        :return: np.ndarray of peak clipped data
        """
        start_data = np.copy(data)

        window = win
        self.log().information('Smoothing window: {0}'.format(str(smooth_window)))

        if smooth_window > 0:
            data = self.smooth(data, smooth_window)

        if lls:
            data = self.log_log_sqrt_transformation(data)

        temp = data.copy()

        if decrease:
            scan = range(window + 1, 0, -1)
        else:
            scan = range(1, window + 1)

        for w in scan:
            for i in range(len(temp)):
                if i < w or i > (len(temp) - w - 1):
                    continue
                else:
                    win_array = temp[i - w:i + w + 1]
                    win_array_reversed = win_array[::-1]
                    average = (win_array + win_array_reversed) / 2
                    temp[i] = np.min(average[:int(len(average) / 2)])

        if lls:
            temp = self.inv_log_log_sqrt_transformation(temp)

        self.log().information('Minimum of starting data - peak clipped data: {0}'.format(str(min(start_data - temp))))

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        # Return 0 if nan
        if np.any(np.isnan(output)):
            return temp * 0.0

        return output

    def PyExec(self):
        """
        Creates a clone of the input workspace to be used for the output, then performs the peak clipping algorithm
        on each histogram using the specified options. The X and E data of the input remain unmodified in the
        output workspace.
        """

        input_ws = self.getProperty("InputWorkspace").value

        window = self.getProperty("WindowSize").value

        smooth_range = self.getProperty("SmoothingRange").value

        lls_set = self.getProperty("LLSCorrection").value

        decreasing = self.getProperty("IncreasingWindow").value

        n_histo = input_ws.getNumberHistograms()

        peak_clip_ws = CloneWorkspace(input_ws, OutputWorkspace=self.getPropertyValue("OutputWorkspace"))

        x = input_ws.extractX()
        y = input_ws.extractY()
        e = input_ws.extractE()

        for h in range(n_histo):
            peak_clip_ws.setX(h, x[h])
            peak_clip_ws.setY(
                h,
                self.peak_clip(y[h],
                               win=window,
                               decrease=decreasing,
                               lls=lls_set,
                               smooth_window=smooth_range))
            peak_clip_ws.setE(h, e[h])

        self.setProperty("OutputWorkspace", peak_clip_ws)


AlgorithmFactory.subscribe(ClipPeaks)

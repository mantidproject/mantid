# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, PropertyMode, WorkspaceProperty
from mantid.simpleapi import *
from mantid.kernel import Direction

import numpy as np


class ClipPeaks(PythonAlgorithm):
    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),
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
            "SmoothingRange", 10,
            "The size of the window used for smoothing data. No smoothing if set to 0."
        )

        self.declareProperty(
            "WindowSize", 10,
            "The size of the peak clipping window to be used."
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "The workspace containing the normalization data.")

    def category(self):
        return "Diffraction\\Corrections"

    def smooth(self, data, order):
        """
        This smooths data based on linear weighted average around
        point i for example for an order of 7 the i point is
        weighted 4, i=/- 1 weighted 3, i+/-2 weighted 2 and i+/-3
        weighted 1 this input is only the y values
        """
        sm = np.zeros(len(data))
        factor = order / 2 + 1

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
        this transforms data to be more sensitive to weak peaks. The
        function is reversed by the Inv_LLS function below
        """
        out = np.log(np.log((input + 1)**0.5 + 1) + 1)

        return out

    def Inv_log_log_sqrt_transformation(self, input):
        """
        See Function LLS function above
        """
        out = (np.exp(np.exp(input) - 1) - 1)**2 - 1

        return out

    def peak_clip(self, data, win=30, decrease=True, LLS=True, smooth_window=0):
        start_data = np.copy(data)

        window = win
        self.log().information(str(smooth_window))

        if smooth_window > 0:
            data = self.smooth(data, smooth_window)

        if LLS:
            data = self.log_log_sqrt_transformation(data)

        temp = data.copy()

        if decrease:
            scan = list(range(window + 1, 0, -1))
        else:
            scan = list(range(1, window + 1))

        for w in scan:
            for i in range(len(temp)):
                if i < w or i > (len(temp) - w - 1):
                    continue
                else:
                    win_array = temp[i - w:i + w + 1].copy()
                    win_array_reversed = win_array[::-1]
                    average = (win_array + win_array_reversed) / 2
                    temp[i] = np.min(average[:int(len(average) / 2)])

        if LLS:
            temp = self.Inv_log_log_sqrt_transformation(temp)

        self.log().information(str(min(start_data - temp)))

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        # Return 0 if nan
        if np.any(np.isnan(output)):
            return temp * 0.0

        return output

    def PyExec(self):
        # Retrieve all relevant notice

        WS = self.getProperty("InputWorkspace").value

        window = self.getProperty("WindowSize").value

        smooth_range = self.getProperty("SmoothingRange").value

        LLS_set = self.getProperty("LLSCorrection").value

        decreasing = self.getProperty("IncreasingWindow").value

        n_histo = WS.getNumberHistograms()

        peak_clip_WS = CloneWorkspace(WS)

        x = WS.extractX()
        y = WS.extractY()
        e = WS.extractE()

        for h in range(n_histo):

            peak_clip_WS.setX(h, x[h])
            peak_clip_WS.setY(
                h,
                self.peak_clip(y[h],
                               win=window,
                               decrease=decreasing,
                               LLS=LLS_set,
                               smooth_window=smooth_range))
            peak_clip_WS.setE(h, e[h])

        self.setProperty("OutputWorkspace", peak_clip_WS)

AlgorithmFactory.subscribe(ClipPeaks)

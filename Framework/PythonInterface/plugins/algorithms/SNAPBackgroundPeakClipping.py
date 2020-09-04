# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *
import numpy as np
import math


class SNAPBackgroundPeakClipping(PythonAlgorithm):
    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("Input Workspace", "", Direction.Input, PropertyMode.Optional),
            "The workspace containing the normalization data.")

        self.declareProperty(
            "LLS Correction", True,
            "Read live data - requires a saved run in the current IPTS with the same Instrumnet configuration"
        )

        self.declareProperty(
            "Increasing Window", False,
            "Read live data - requires a saved run in the current IPTS with the same Instrumnet configuration"
        )

        self.declareProperty(
            "Smoothing Range", 10,
            "Read live data - requires a saved run in the current IPTS with the same Instrumnet configuration"
        )

        self.declareProperty(
            "Peak Clipping Window Size", 10,
            "Read live data - requires a saved run in the current IPTS with the same Instrumnet configuration"
        )

        self.declareProperty(WorkspaceProperty("Output Workspace", "", Direction.Output),
                             "The workspace containing the normalization data.")

    def category(self):
        return "Diffraction\\Reduction"

############################

    def smooth(self, data, order):
        # This smooths data based on linear weigthed average around
        # point i for example for an order of 7 the i point is
        # weighted 4, i=/- 1 weighted 3, i+/-2 weighted 2 and i+/-3
        # weighted 1 this input is only the y values
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

    def LLS_transformation(self, input):
        # this transforms data to be more sensitive to weak peaks. The
        # function is reversed by the Inv_LLS function below
        out = np.log(np.log((input + 1)**0.5 + 1) + 1)

        return out

    def Inv_LLS_transformation(self, input):
        # See Function LLS function above
        out = (np.exp(np.exp(input) - 1) - 1)**2 - 1

        return out

    def peak_clip(self, data, win=30, decrese=True, LLS=True, smooth_window=0):
        start_data = np.copy(data)

        window = win
        self.log().information(str(smooth_window))

        if smooth_window > 0:
            data = self.smooth(data, smooth_window)

        if LLS:
            data = self.LLS_transformation(data)

        temp = data.copy()

        if decrese:
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
            temp = self.Inv_LLS_transformation(temp)

        self.log().information(str(min(start_data - temp)))

        index = np.where((start_data - temp) == min(start_data - temp))[0][0]

        output = temp * (start_data[index] / temp[index])

        return output


##############################

    def PyExec(self):
        # Retrieve all relevant notice

        WS = self.getProperty("Input Workspace").value

        window = self.getProperty("Peak Clipping Window Size").value

        smooth_range = self.getProperty("Smoothing Range").value

        LLS_set = self.getProperty("LLS Correction").value

        decreasing = self.getProperty("Increasing Window").value

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
                               decrese=decreasing,
                               LLS=LLS_set,
                               smooth_window=smooth_range))
            peak_clip_WS.setE(h, e[h])

        self.setProperty("Output Workspace", peak_clip_WS)

AlgorithmFactory.subscribe(SNAPBackgroundPeakClipping)

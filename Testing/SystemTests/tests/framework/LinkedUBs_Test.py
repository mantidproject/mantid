# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
# System test that loads short SXD numors and runs LinkedUBs.
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import FindSXPeaks, LinkedUBs, LoadRaw, PredictPeaks, SetGoniometer, SetUB
import numpy as np


class LinkedUBs_Test(systemtesting.MantidSystemTest):
    def runTest(self):
        ws1 = LoadRaw(Filename="SXD30904.raw.md5", OutputWorkspace="SXD30904")

        ws2 = LoadRaw(Filename="SXD30905.raw.md5", OutputWorkspace="SXD30905")

        UB1 = np.array(
            [[0.00099434, 0.17716870, -0.00397909], [0.17703120, -0.00117345, -0.00800899], [-0.00803319, -0.00393000, -0.17699037]]
        )

        SetUB(ws1, UB=UB1)

        SetGoniometer(Workspace=ws1, Axis0="-206,0,1,0,-1")
        PredictPeaks(
            InputWorkspace=ws1,
            WavelengthMin=0.2,
            WavelengthMax=10,
            MinDSpacing=0.5,
            ReflectionCondition="All-face centred",
            OutputWorkspace="SXD30905_predict_peaks",
        )

        FindSXPeaks(
            InputWorkspace=ws2,
            PeakFindingStrategy="AllPeaks",
            ResolutionStrategy="AbsoluteResolution",
            XResolution=0.2,
            PhiResolution=2,
            TwoThetaResolution=2,
            OutputWorkspace="SXD30905_find_peaks",
        )

        # linkedUBs
        LinkedUBs(
            qTolerance=0.1,
            qDecrement=0.95,
            dTolerance=0.02,
            numPeaks=25,
            peakIncrement=10,
            Iterations=20,
            a=5.6428,
            b=5.6428,
            c=5.6428,
            alpha=90,
            beta=90,
            gamma=90,
            MinWavelength=0.2,
            MaxWavelength=10,
            MinDSpacing=0.5,
            MaxDSpacing=5,
            ReflectionCondition="All-face centred",
            Workspace=ws2,
            ObservedPeaks="SXD30905_find_peaks",
            PredictedPeaks="SXD30905_predict_peaks",
            LinkedPeaks="SXD30905_linked_peaks",
            LinkedPredictedPeaks="SXD30905_linked_peaks_predicted",
            DeleteWorkspace=False,
        )

        UB2 = np.array([[0.00588768, -0.15971797, 0.07655954], [0.17702963, 0.0028714, -0.00762388], [0.0056306, 0.07673189, 0.15964452]])

        linked_peaks = mtd["SXD30905_linked_peaks"]
        linked_UB = linked_peaks.sample().getOrientedLattice().getUB()

        diff = linked_UB - UB2
        print(diff)

        for i in range(len(diff)):
            for j in range(len(diff)):
                if abs(diff[i, j]) > 1e-7:
                    raise Exception("More than 1e-7 difference between UB matrices")

    def doValidation(self):
        # If we reach here, no validation failed
        return True

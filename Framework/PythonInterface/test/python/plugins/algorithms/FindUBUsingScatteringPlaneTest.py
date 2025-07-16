import numpy as np
import unittest
from mantid.simpleapi import SetUB, CreatePeaksWorkspace, FindUBUsingScatteringPlane


def add_peaksHKL(ws_list, Hs, Ks, L):
    for h in Hs:
        for k in Ks:
            for peaks in ws_list:
                pk = peaks.createPeakHKL([h, k, L])
                peaks.addPeak(pk)


def getBMatrix(ws):
    return ws.sample().getOrientedLattice().getB()


def getUMatrix(ws):
    return ws.sample().getOrientedLattice().getU()


def getuVector(ws):
    return ws.sample().getOrientedLattice.getuVector()


def getvVector(ws):
    return ws.sample().getOrientedLattice.getvVector()


def getUBMatrix(ws):
    return ws.sample().getOrientedLattice().getUB()


class FindUBUsingScatteringPlaneTest(unittest.TestCase):
    @classmethod
    def test_find_correct_ub(self):
        peaks1 = CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace="peaks1")
        UB = np.diag([0.25, 0.25, 0.1])
        add_peaksHKL([peaks1], [1], [1], [4])
        SetUB(peaks1, UB=UB)
        Vector1 = getuVector(peaks1)
        Vector2 = getvVector(peaks1)
        OutputWorkspace = FindUBUsingScatteringPlane(
            a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, PeakWorkSpace="peaks1", Vector1=Vector1, Vector2=Vector2
        )
        ## need to check how it's tied to output workspace in algorithm
        CalcUB = getUBMatrix(OutputWorkspace)
        InputUB = getUBMatrix(peaks1)
        print(np.allclose(CalcUB, InputUB, atol=0.1))

    def test_multiple_peaks_provided(self):
        peaks1 = CreatePeaksWorkspace(NumberOfPeaks=0, OutputWorkspace="SXD_peaks1")
        UB = np.diag([0.25, 0.25, 0.1])
        add_peaksHKL([peaks1], range(0, 3), range(0, 3), 4)
        SetUB(peaks1, UB=UB)
        Vector1 = getuVector(peaks1)
        Vector2 = getvVector(peaks1)
        OutputWorkspace = FindUBUsingScatteringPlane(
            a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89, PeakWorkSpace="peaks1", Vector1=Vector1, Vector2=Vector2
        )
        UB = getUBMatrix(OutputWorkspace)


if __name__ == "__main__":
    unittest.main()


##need to test output if no v1 and v2 are provided
# what happens if more than 1 peak in workspace

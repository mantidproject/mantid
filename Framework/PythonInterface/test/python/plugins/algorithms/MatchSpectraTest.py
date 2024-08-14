# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, DeleteWorkspaces, MatchSpectra, mtd
import numpy as np
import unittest


class MatchSpectraTest(unittest.TestCase):
    numhist = 4
    numpoints = 100

    def __createWorkspace(self, name, histogram, uncertainties):
        if histogram:
            x = np.arange(self.numpoints + 1, dtype=float)
        else:
            x = np.arange(self.numpoints, dtype=float)
        x = np.tile(x, self.numhist)
        y = np.arange(self.numpoints, dtype=float)
        y = np.tile(y, self.numhist)
        y[self.numpoints : 2 * self.numpoints] += 10  # for offset test
        y[2 * self.numpoints : 3 * self.numpoints] *= 10  # for scale test
        y[3 * self.numpoints :] = y[3 * self.numpoints :] * 10 + 10  # for scale and offset test
        dy = np.zeros(y.size, dtype=float) + 1

        if uncertainties:
            CreateWorkspace(OutputWorkspace=name, DataX=x, DataY=y, DataE=dy, NSpec=self.numhist)
        else:
            CreateWorkspace(OutputWorkspace=name, DataX=x, DataY=y, NSpec=self.numhist)

    def __createRaggedWorkspace(self, name, histogram):
        xlen = self.numpoints
        if histogram:
            xlen += 1
        x = np.arange(xlen, dtype=float)
        x = np.tile(x, self.numhist)
        shifts = (0.0, 10.0, 20.0, 30.0)
        # shift x-values so they are not common
        for i, shift in enumerate(shifts):
            x[i * xlen : (i + 1) * xlen] += shift

        y = np.zeros(self.numhist * self.numpoints, dtype=float)
        for i, shift in enumerate(shifts):
            # follow shift from above
            newY = np.arange(self.numpoints, dtype=float) + shift
            if i == 1:
                newY += 10.0
            elif i == 2:
                newY *= 10.0
            elif i == 3:
                newY = newY * 10 + 10
            # otherwise do nothing
            y[i * self.numpoints : (i + 1) * self.numpoints] = newY

        dy = np.zeros(y.size, dtype=float) + 1

        CreateWorkspace(OutputWorkspace=name, DataX=x, DataY=y, DataE=dy, NSpec=self.numhist)

    def __checkValues(self, results, index, scale, offset):
        self.assertEqual(results.Scale[index], scale, "wksp index {} scale exp={} found={}".format(index, scale, results.Scale[index]))
        self.assertEqual(results.Offset[index], offset, "wksp index {} offset exp={} found={}".format(index, offset, results.Offset[index]))
        self.assertEqual(results.ChiSq[index], 0.0, "wksp index {} chisq".format(index))

    def __checkReference(self, results, index):
        self.assertEqual(results.Scale[index], 1.0, "reference scale")
        self.assertEqual(results.Offset[index], 0.0, "reference offset")
        self.assertEqual(results.ChiSq[index], 0.0, "reference chisq")

    def testWithoutUncertainties(self):
        inwsname = "MatchSpectraTest_withoutUncertainties"
        outwsname = inwsname + "_out"  # never gets created

        for histogram in (True, False):
            self.__createWorkspace(inwsname, histogram, False)

            with self.assertRaisesRegex(RuntimeError, "None of the uncertainties in the reference spectrum is greater than zero."):
                results = MatchSpectra(
                    InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=1, CalculateOffset=True, CalculateScale=True
                )

        DeleteWorkspace(Workspace=inwsname)

    def __testCommonBoundaries(self, histogram):
        inwsname = "MatchSpectraTest_commonBoundaries"
        if histogram:
            inwsname += "Histogram"
        else:
            inwsname += "Frequency"
        outwsname = inwsname + "_out"
        self.__createWorkspace(inwsname, histogram, True)

        ##### offset only
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=2, CalculateOffset=True, CalculateScale=False
        )
        self.__checkReference(results, 1)
        self.__checkValues(results, 0, 1.0, 10.0)
        self.assertTrue(np.all(mtd[outwsname].readY(0) == mtd[outwsname].readY(1)))

        ##### scale only
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=3, CalculateOffset=False, CalculateScale=True
        )
        self.__checkReference(results, 2)
        self.__checkValues(results, 0, 10.0, 0.0)
        self.assertTrue(np.all(mtd[outwsname].readY(0) == mtd[outwsname].readY(2)))

        ##### both
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=4, CalculateOffset=True, CalculateScale=True
        )
        self.__checkReference(results, 3)
        self.__checkValues(results, 0, 10.0, 10.0)
        self.assertTrue(np.all(mtd[outwsname].readY(0) == mtd[outwsname].readY(3)))

        DeleteWorkspaces(WorkspaceList=[inwsname, outwsname])

    def testCommonBoundariesHistogram(self):
        self.__testCommonBoundaries(True)

    def testCommonBoundariesFrequency(self):
        self.__testCommonBoundaries(False)

    def __testRaggedBoundaries(self, histogram):
        inwsname = "MatchSpectraTest_raggedBoundaries"
        if histogram:
            inwsname += "Histogram"
        else:
            inwsname += "Frequency"
        outwsname = inwsname + "_out"
        self.__createRaggedWorkspace(inwsname, histogram)

        ##### offset only
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=2, CalculateOffset=True, CalculateScale=False
        )
        self.__checkReference(results, 1)
        self.__checkValues(results, 0, 1.0, 10.0)

        ##### scale only
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=3, CalculateOffset=False, CalculateScale=True
        )
        self.__checkReference(results, 2)
        self.__checkValues(results, 0, 10.0, 0.0)

        ##### both
        results = MatchSpectra(
            InputWorkspace=inwsname, OutputWorkspace=outwsname, ReferenceSpectrum=4, CalculateOffset=True, CalculateScale=True
        )
        self.__checkReference(results, 3)
        self.__checkValues(results, 0, 10.0, 10.0)

        DeleteWorkspaces(WorkspaceList=[inwsname, outwsname])

    def testRaggedBoundariesHistogram(self):
        self.__testRaggedBoundaries(True)

    def testRaggedBoundariesFrequency(self):
        self.__testRaggedBoundaries(False)


if __name__ == "__main__":
    unittest.main()

# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import MatrixWorkspace, AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from testhelpers import run_algorithm
import numpy as np


class CreateWorkspaceTest(unittest.TestCase):
    def test_create_with_1D_numpy_array(self):
        x = np.array([1.0, 2.0, 3.0, 4.0])
        y = np.array([1.0, 2.0, 3.0])
        e = np.sqrt(np.array([1.0, 2.0, 3.0]))

        wksp = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="TOF")
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEqual(wksp.getNumberHistograms(), 1)

        self.assertEqual(len(wksp.readY(0)), len(y))
        self.assertEqual(len(wksp.readX(0)), len(x))
        self.assertEqual(len(wksp.readE(0)), len(e))

        for index in range(len(y)):
            self.assertEqual(wksp.readY(0)[index], y[index])
            self.assertEqual(wksp.readE(0)[index], e[index])
            self.assertEqual(wksp.readX(0)[index], x[index])
        # Last X value
        self.assertEqual(wksp.readX(0)[len(x) - 1], x[len(x) - 1])
        AnalysisDataService.remove("wksp")

    def test_create_with_2D_numpy_array(self):
        x = np.array([1.0, 2.0, 3.0, 4.0])
        y = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]])
        e = np.sqrt(y)

        wksp = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=2, UnitX="TOF")
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEqual(wksp.getNumberHistograms(), 2)

        for i in [0, 1]:
            for j in range(len(y[0])):
                self.assertEqual(wksp.readY(i)[j], y[i][j])
                self.assertEqual(wksp.readE(i)[j], e[i][j])
                self.assertEqual(wksp.readX(i)[j], x[j])
            # Last X value
            self.assertEqual(wksp.readX(i)[len(x) - 1], x[len(x) - 1])

        AnalysisDataService.remove("wksp")

    def test_with_data_from_other_workspace(self):
        x1 = np.array([1.0, 2.0, 3.0, 4.0])
        y1 = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]])
        e1 = np.sqrt(y1)
        loq = CreateWorkspace(DataX=x1, DataY=y1, DataE=e1, NSpec=2, UnitX="Wavelength")

        x2 = loq.extractX()
        y2 = loq.extractY()
        e2 = loq.extractE()

        wksp = CreateWorkspace(DataX=x2, DataY=y2, DataE=e2, NSpec=2, UnitX="Wavelength")
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEqual(wksp.getNumberHistograms(), 2)

        for i in [0, 1]:
            for j in range(len(y2[0])):
                self.assertEqual(wksp.readY(i)[j], loq.readY(i)[j])
                self.assertEqual(wksp.readE(i)[j], loq.readE(i)[j])
                self.assertEqual(wksp.readX(i)[j], loq.readX(i)[j])
            # Last X value
            self.assertEqual(wksp.readX(i)[len(x2) - 1], loq.readX(i)[len(x2) - 1])

        AnalysisDataService.remove("wksp")

    def test_create_with_numerical_vertical_axis_values(self):
        data = [1.0, 2.0, 3.0]
        axis_values = [5, 6, 7]
        alg = run_algorithm(
            "CreateWorkspace",
            DataX=data,
            DataY=data,
            NSpec=3,
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=axis_values,
            child=True,
        )
        wksp = alg.getProperty("OutputWorkspace").value
        for i in range(len(axis_values)):
            self.assertEqual(wksp.getAxis(1).getValue(i), axis_values[i])

    def test_create_with_numpy_vertical_axis_values(self):
        data = [1.0, 2.0, 3.0]
        axis_values = np.array([6.0, 7.0, 8.0])
        alg = run_algorithm(
            "CreateWorkspace",
            DataX=data,
            DataY=data,
            NSpec=3,
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=axis_values,
            child=True,
        )
        wksp = alg.getProperty("OutputWorkspace").value
        for i in range(len(axis_values)):
            self.assertEqual(wksp.getAxis(1).getValue(i), axis_values[i])


if __name__ == "__main__":
    unittest.main()

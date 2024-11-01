# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Specifically tests the Fit function in the simple API
"""

import unittest
import testhelpers
import platform

from mantid.simpleapi import CreateWorkspace, Fit, FunctionWrapper
from mantid.api import mtd, MatrixWorkspace, ITableWorkspace


import numpy as np
from testhelpers import run_algorithm


class SimpleAPIFitTest(unittest.TestCase):
    _raw_ws = None

    def setUp(self):
        if self._raw_ws is None:
            dataX = np.linspace(start=5.6e4, stop=5.701e4, num=101)
            dataY = 5 * dataX + 4
            ws = CreateWorkspace(DataX=dataX, DataY=dataY, NSpec=1)
            self.__class__._raw_ws = ws

    def test_minimal_positional_arguments_work(self):
        if platform.system() == "Darwin":  # crashes
            return
        testhelpers.assertRaisesNothing(self, Fit, "name=FlatBackground", self._raw_ws)

    def test_minimal_positional_arguments_with_functionwrapper_work(self):
        if platform.system() == "Darwin":  # crashes
            return
        fb = FunctionWrapper("FlatBackground")
        testhelpers.assertRaisesNothing(self, Fit, fb, self._raw_ws)

    def test_function_positional_and_workspace_keyword_arguments_work(self):
        if platform.system() == "Darwin":  # crashes
            return
        testhelpers.assertRaisesNothing(self, Fit, "name=FlatBackground", InputWorkspace=self._raw_ws)

    def test_function_and_workspace_keyword_arguments_work(self):
        if platform.system() == "Darwin":  # crashes
            return
        testhelpers.assertRaisesNothing(self, Fit, Function="name=FlatBackground", InputWorkspace=self._raw_ws)

    def test_function_returns_are_correct_type_when_no_output_ws_requested(self):
        if platform.system() == "Darwin":  # crashes
            return
        retvals = Fit("name=FlatBackground", self._raw_ws)
        self.assertTrue(isinstance(retvals.OutputStatus, str))
        self.assertTrue(isinstance(retvals.OutputChi2overDoF, float))

    def test_function_accepts_all_arguments_as_keywords(self):
        if platform.system() == "Darwin":  # crashes
            return
        output_name = "kwargsfitWS"
        retvals = Fit(Function="name=FlatBackground", InputWorkspace=self._raw_ws, Output=output_name)
        self._check_returns_are_correct_type_with_workspaces(retvals)
        self.assertTrue(output_name + "_Workspace" in mtd)

    def test_function_returns_are_correct_type_when_output_ws_is_requested(self):
        if platform.system() == "Darwin":  # crashes
            return
        output_name = "fitWS"
        retvals = Fit("name=FlatBackground", self._raw_ws, Output="fitWS")
        self._check_returns_are_correct_type_with_workspaces(retvals)
        self.assertTrue(output_name + "_Workspace" in mtd)

    def test_other_arguments_are_accepted_by_keyword(self):
        if platform.system() == "Darwin":  # crashes
            return
        output_name = "otherargs_fitWS"
        retvals = Fit("name=FlatBackground", self._raw_ws, MaxIterations=2, Output=output_name)
        self._check_returns_are_correct_type_with_workspaces(retvals)
        self.assertTrue(output_name + "_Workspace" in mtd)

    def test_Fit_accepts_EnableLogging_keyword(self):
        if platform.system() == "Darwin":  # crashes
            return
        output_name = "otherargs_fitWS"
        retvals = Fit("name=FlatBackground", self._raw_ws, MaxIterations=2, Output=output_name, EnableLogging=False)
        self._check_returns_are_correct_type_with_workspaces(retvals)
        self.assertTrue(output_name + "_Workspace" in mtd)

    def _check_returns_are_correct_type_with_workspaces(self, retvals):
        self.assertTrue(isinstance(retvals.OutputStatus, str))
        self.assertTrue(isinstance(retvals.OutputChi2overDoF, float))
        self.assertTrue(isinstance(retvals.OutputNormalisedCovarianceMatrix, ITableWorkspace))
        self.assertTrue(isinstance(retvals.OutputParameters, ITableWorkspace))
        self.assertTrue(isinstance(retvals.OutputWorkspace, MatrixWorkspace))
        self.assertTrue(isinstance(retvals.Function, FunctionWrapper))
        self.assertTrue(isinstance(retvals.CostFunction, str))

    def test_Fit_works_with_multidomain_functions(self):
        x1 = np.arange(10)
        y1 = np.empty(0)
        y2 = np.empty(0)
        y3 = np.empty(0)

        for x in x1:
            y1 = np.append(y1, 3)
            y2 = np.append(y2, 2.9 + 3 * x)
            y3 = np.append(y3, 3.1 + 3 * x * x)

        x = np.concatenate((x1, x1, x1))
        y = np.concatenate((y1, y2, y3))

        data_name = "dataWS"
        run_algorithm("CreateWorkspace", OutputWorkspace=data_name, DataX=x, DataY=y, DataE=np.ones(30), NSpec=3, UnitX="TOF")

        f1 = ";name=UserFunction,$domains=i,Formula=a+b*x+c*x^2"
        func = "composite=MultiDomainFunction,NumDeriv=1" + f1 + f1 + f1 + ";ties=(f2.a=f1.a=f0.a)"

        output_name = "fitWS"
        Fit(
            Function=func,
            InputWorkspace=data_name,
            WorkspaceIndex=0,
            Output=output_name,
            InputWorkspace_1=data_name,
            WorkspaceIndex_1=1,
            InputWorkspace_2=data_name,
            WorkspaceIndex_2=2,
        )

        self.assertTrue(output_name + "_Parameters" in mtd)
        params = mtd[output_name + "_Parameters"]
        self.assertEqual(params.rowCount(), 10)

        self.assertAlmostEqual(params.row(0)["Value"], 3.0, 10)
        self.assertAlmostEqual(params.row(3)["Value"], 3.0, 10)
        self.assertAlmostEqual(params.row(6)["Value"], 3.0, 10)
        self.assertAlmostEqual(params.row(4)["Value"], 3.0, 1)
        self.assertAlmostEqual(params.row(8)["Value"], 3.0, 1)


if __name__ == "__main__":
    unittest.main()

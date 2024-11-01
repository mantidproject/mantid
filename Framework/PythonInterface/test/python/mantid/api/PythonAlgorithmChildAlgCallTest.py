# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""A test for the simple API dedicated to Python algorithms. Checks
things like Child Algorithm calls
"""
import unittest
from testhelpers import run_algorithm

from mantid import mtd
import mantid.simpleapi as api

__PARENTALG__ = """
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *

class PythonAlgorithmChildAlgCallTestAlg(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction = Direction.Output))

    def PyExec(self):
        inputWS = self.getPropertyValue("InputWorkspace")
        outputWS = api.Scale(InputWorkspace=inputWS, Factor=2.0)
        self.setProperty("OutputWorkspace", outputWS)

AlgorithmFactory.subscribe(PythonAlgorithmChildAlgCallTestAlg)
"""


class PythonAlgorithmChildAlgCallTest(unittest.TestCase):

    _alg_reg = False
    _ws_name = "test_ws"
    _ws_name2 = "test_ws_1"

    def setUp(self):
        if not self._alg_reg:
            # Register algorithm
            exec(__PARENTALG__)
            self.__class__._alg_reg = True

    def tearDown(self):
        if self._ws_name in mtd:
            mtd.remove(self._ws_name)
        if self._ws_name2 in mtd:
            mtd.remove(self._ws_name2)

    def test_ChildAlg_call_with_output_and_input_ws_the_same_succeeds(self):
        data = [1.0]
        api.CreateWorkspace(DataX=data, DataY=data, NSpec=1, UnitX="Wavelength", OutputWorkspace=self._ws_name)
        try:
            run_algorithm("PythonAlgorithmChildAlgCallTestAlg", InputWorkspace=self._ws_name, OutputWorkspace=self._ws_name)
        except Exception as exc:
            self.fail("Algorithm call failed: %s" % str(exc))

        self.assertTrue(self._ws_name in mtd)
        self.assertAlmostEqual(mtd[self._ws_name].readY(0)[0], 2.0, places=10)

    def test_ChildAlg_call_with_output_and_input_ws_different_succeeds(self):
        data = [1.0]
        api.CreateWorkspace(DataX=data, DataY=data, NSpec=1, UnitX="Wavelength", OutputWorkspace=self._ws_name)

        try:
            run_algorithm("PythonAlgorithmChildAlgCallTestAlg", InputWorkspace=self._ws_name, OutputWorkspace=self._ws_name2)
        except Exception as exc:
            self.fail("Algorithm call failed: %s" % str(exc))

        self.assertTrue(self._ws_name2 in mtd)
        self.assertAlmostEqual(mtd[self._ws_name2].readY(0)[0], 2.0, places=10)


if __name__ == "__main__":
    unittest.main()

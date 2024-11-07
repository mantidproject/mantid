# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.geometry import Goniometer
from testhelpers import can_be_instantiated
from testhelpers import run_algorithm
import numpy as np


class GoniometerTest(unittest.TestCase):
    def test_Goniometer_can_be_instantiated(self):
        self.assertTrue(can_be_instantiated(Goniometer))

    def test_getEulerAngles(self):
        g = Goniometer()
        self.assertEqual(g.getEulerAngles()[0], 0)
        self.assertEqual(g.getEulerAngles()[1], 0)
        self.assertEqual(g.getEulerAngles()[2], 0)

    def test_setR_getR(self):
        g = Goniometer()
        r = np.array([(1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0)])
        g.setR(r)
        self.assertTrue((g.getR() == r).all())

    def test_default_goniometer(self):
        """
        Default goniometer is the identity matrix
        """
        alg = run_algorithm("CreateWorkspace", DataX=[1, 2, 3, 4, 5], DataY=[1, 2, 3, 4, 5], NSpec=1, child=True)
        ws = alg.getProperty("OutputWorkspace").value
        run = ws.run()
        g = run.getGoniometer()
        self.assertTrue(isinstance(g, Goniometer))
        self.assertTrue((g.getR() == np.identity(3)).all())

    def test_change_default_goniometer(self):
        """
        Get the default gonimoter
        Set it to something else and make sure it was successfully changed
        """
        alg = run_algorithm("CreateWorkspace", DataX=[1, 2, 3, 4, 5], DataY=[1, 2, 3, 4, 5], NSpec=1, child=True)
        ws = alg.getProperty("OutputWorkspace").value
        run = ws.run()
        g = run.getGoniometer()
        # change the matrix:
        r = np.array([(1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0)])
        g.setR(r)
        self.assertTrue((g.getR() == r).all())


if __name__ == "__main__":
    unittest.main()

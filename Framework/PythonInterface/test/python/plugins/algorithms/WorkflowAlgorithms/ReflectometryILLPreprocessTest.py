# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import numpy
import unittest

from mantid.kernel import config
from mantid.api import mtd
from testhelpers import assertRaisesNothing, create_algorithm
import ReflectometryILL_common as common


class ReflectometryILLPreprocessTest(unittest.TestCase):
    def setUp(self):
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]

        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"

    def tearDown(self):
        if self._facility:
            config["default.facility"] = self._facility
        if self._instrument:
            config["default.instrument"] = self._instrument
        mtd.clear()

    def testDirectBeamD17(self):
        args = {"Run": "ILL/D17/317369.nxs", "Measurement": "DirectBeam", "OutputWorkspace": "outWS", "rethrow": True, "child": True}
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 202.177, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight)) / 2.0
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 0.0, delta=0.1)

    def testReflectedBeamUserAngleD17(self):
        args = {
            "Run": "ILL/D17/317370.nxs",
            "Measurement": "ReflectedBeam",
            "AngleOption": "UserAngle",
            "BraggAngle": 1.5,
            "OutputWorkspace": "outWS",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight)) / 2.0
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 3.0, delta=0.01)

    def testReflectedBeamSanAngleD17(self):
        args = {
            "Run": "ILL/D17/317370.nxs",
            "Measurement": "ReflectedBeam",
            "AngleOption": "SampleAngle",
            "OutputWorkspace": "outWS",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight)) / 2.0
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 1.6, delta=0.01)

    def testReflectedBeamDanAngleD17(self):
        args = {
            "Run": "ILL/D17/317370.nxs",
            "Measurement": "ReflectedBeam",
            "AngleOption": "DetectorAngle",
            "DirectBeamForegroundCentre": 202.177,
            "DirectBeamDetectorAngle": 0.1,
            "OutputWorkspace": "outWS",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight)) / 2.0
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 3.097, delta=0.01)

    def testTwoInputFiles(self):
        outWSName = "outWS"
        args = {"Run": "ILL/D17/317369, ILL/D17/317370.nxs", "OutputWorkspace": outWSName, "rethrow": True, "child": True}
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        self.assertEqual(mtd.getObjectNames(), [])

    def test_no_normalisation(self):
        outWSName = "outWS"
        args = {
            "Run": "ILL/D17/317370.nxs",
            "OutputWorkspace": outWSName,
            "FluxNormalisation": "Normalisation OFF",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        self.assertEqual(mtd.getObjectNames(), [])
        self.assertAlmostEqual(outWS.readY(69)[307], 5.240, 3)
        self.assertAlmostEqual(outWS.readE(69)[307], 6.550, 3)

    def test_time_normalisation(self):
        outWSName = "outWS"
        args = {
            "Run": "ILL/D17/317370.nxs",
            "OutputWorkspace": outWSName,
            "FluxNormalisation": "Normalise To Time",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        self.assertEqual(mtd.getObjectNames(), [])
        duration = outWS.getRun().getLogData("duration").value
        self.assertAlmostEqual(outWS.readY(69)[307], 5.240 / duration, 3)
        self.assertAlmostEqual(outWS.readE(69)[307], 6.550 / duration, 3)

    def test_monitor_normalisation(self):
        outWSName = "outWS"
        args = {
            "Run": "ILL/D17/317370.nxs",
            "OutputWorkspace": outWSName,
            "FluxNormalisation": "Normalise To Monitor",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), "Wavelength")
        self.assertEqual(mtd.getObjectNames(), [])
        self.assertAlmostEqual(outWS.readY(69)[307], 9.080e-07, 3)
        self.assertAlmostEqual(outWS.readE(69)[307], 1.135e-06, 3)

    def testReplaceSampleLogs(self):
        outWSName = "outWS"
        logsToReplace = {"ChopperSetting.firstChopper": 2, "ChopperSetting.secondChopper": 1}
        args = {"Run": "ILL/D17/317369", "OutputWorkspace": outWSName, "LogsToReplace": logsToReplace, "rethrow": True, "child": True}
        alg = create_algorithm("ReflectometryILLPreprocess", **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty("OutputWorkspace").value
        self.assertEqual(
            float(outWS.getRun().getLogData("ChopperSetting.firstChopper").value), 2.0
        )  # explicit cast to float for OSX and Windows which implicitly cast it to int
        self.assertEqual(float(outWS.getRun().getLogData("ChopperSetting.secondChopper").value), 1.0)


if __name__ == "__main__":
    unittest.main()

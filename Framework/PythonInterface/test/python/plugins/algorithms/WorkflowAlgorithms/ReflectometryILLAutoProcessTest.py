# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import mtd, config
from testhelpers import assertRaisesNothing, create_algorithm
import unittest
import numpy


class ReflectometryILLAutoProcessTest(unittest.TestCase):
    # cache the faciility, instruments and the dirs
    _def_fac = config["default.facility"]
    _def_inst = config["default.instrument"]
    _data_dirs = config["datasearch.directories"]

    def setUp(self):
        # set instrument and append datasearch directory
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config.appendDataSearchSubDir("ILL/D17/")

    def tearDown(self):
        # set cached facility and datasearch directory
        config["default.facility"] = self._def_fac
        config["default.instrument"] = self._def_inst
        config["datasearch.directories"] = self._data_dirs
        mtd.clear()

    def testDetectorAngle(self):
        args = {"Run": "317370", "DirectRun": "317369", "OutputWorkspace": "outWS", "rethrow": True, "child": True}
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 1)

    def testUserAngle(self):
        args = {
            "Run": "317370",
            "DirectRun": "317369",
            "OutputWorkspace": "outWS",
            "AngleOption": "UserAngle",
            "Theta": 0.8,
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 1)
        self.assertAlmostEqual(mtd["outWS"].getItem(0).spectrumInfo().signedTwoTheta(0), 2.0 * 0.8 * numpy.pi / 180.0, delta=0.00001)

    def testSampleAngle(self):
        args = {
            "Run": "317370",
            "DirectRun": "317369",
            "AngleOption": "SampleAngle",
            "OutputWorkspace": "outWS",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 1)

    def testTwoRunsMerged(self):
        args = {"Run": "317370+317369", "DirectRun": "317369+317370", "OutputWorkspace": "outWS", "rethrow": True, "child": True}
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 1)

    def testMultipleAngles(self):
        args = {
            "Run": "317370, 317370",
            "DirectRun": "317369, 317369",
            "OutputWorkspace": "outWS",
            "DeltaQFractionBinning": "0.5, 0.5",
            "rethrow": True,
            "child": True,
        }
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 3)

    def testNexus2019(self):
        directBeams = "541838,541839"
        reflectedBeams = "541882,541883"
        foregroundWidth = [3, 15]
        angleOffset = [2, 5]
        angleWidth = 5
        braggAngles = [0.8, 3.5]
        args = {
            "Run": reflectedBeams,
            "DirectRun": directBeams,
            "OutputWorkspace": "outWS",
            "Theta": braggAngles,
            "DeltaQFractionBinning": 0.5,
            "DirectLowAngleFrgHalfWidth": foregroundWidth,
            "DirectHighAngleFrgHalfWidth": foregroundWidth,
            "DirectLowAngleBkgOffset": angleOffset,
            "DirectLowAngleBkgWidth": angleWidth,
            "DirectHighAngleBkgOffset": angleOffset,
            "DirectHighAngleBkgWidth": angleWidth,
            "ReflLowAngleFrgHalfWidth": foregroundWidth,
            "ReflHighAngleFrgHalfWidth": foregroundWidth,
            "ReflLowAngleBkgOffset": angleOffset,
            "ReflLowAngleBkgWidth": angleWidth,
            "ReflHighAngleBkgOffset": angleOffset,
            "ReflHighAngleBkgWidth": angleWidth,
            "WavelengthLowerBound": [3.0, 3.0],
            "WavelengthUpperBound": [27.0, 25.0],
            "GlobalScaleFactor": 0.13,
            "rethrow": True,
        }
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        self.checkOutput(mtd["outWS"], 3)

    def testDefaultValues(self):
        args = {"Run": "317370", "DirectRun": "317369", "OutputWorkspace": "outWS", "rethrow": True}
        alg = create_algorithm("ReflectometryILLAutoProcess", **args)
        assertRaisesNothing(self, alg.execute)
        out = mtd["outWS"].getItem(0)
        self.assertEqual(out.getHistory().size(), 1)
        algH = out.getHistory().getAlgorithmHistory(0)

        from ReflectometryILLAutoProcess import PropertyNames
        from ReflectometryILLPreprocess import Prop

        self.assertEqual(algH.getPropertyValue("AngleOption"), "DetectorAngle")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_BKG_OFFSET_DIRECT), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_BKG_OFFSET), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_BKG_WIDTH_DIRECT), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_BKG_WIDTH), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_BKG_OFFSET_DIRECT), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_BKG_OFFSET), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_BKG_WIDTH_DIRECT), "5")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_BKG_WIDTH), "5")
        self.assertEqual(algH.getPropertyValue(Prop.SUBALG_LOGGING), "Logging OFF")
        self.assertEqual(algH.getPropertyValue(Prop.CLEANUP), "Cleanup ON")
        self.assertEqual(algH.getPropertyValue(Prop.SLIT_NORM), "Slit Normalisation AUTO")
        self.assertEqual(algH.getPropertyValue(Prop.FLUX_NORM_METHOD), "Normalise To Time")
        self.assertEqual(algH.getPropertyValue(PropertyNames.CACHE_DIRECT_BEAM), "0")
        self.assertEqual(algH.getPropertyValue(PropertyNames.BKG_METHOD_DIRECT), "Background Average")
        self.assertEqual(algH.getPropertyValue(PropertyNames.BKG_METHOD), "Background Average")
        self.assertEqual(algH.getPropertyValue(PropertyNames.START_WS_INDEX_DIRECT), "0")
        self.assertEqual(algH.getPropertyValue(PropertyNames.START_WS_INDEX), "0")
        self.assertEqual(algH.getPropertyValue(PropertyNames.END_WS_INDEX_DIRECT), "255")
        self.assertEqual(algH.getPropertyValue(PropertyNames.END_WS_INDEX), "255")
        self.assertEqual(algH.getPropertyValue(PropertyNames.SUM_TYPE), "Incoherent")
        self.assertEqual(algH.getPropertyValue(PropertyNames.WAVELENGTH_LOWER), "2")
        self.assertEqual(algH.getPropertyValue(PropertyNames.WAVELENGTH_UPPER), "30")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_FRG_HALF_WIDTH), "2")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_FRG_HALF_WIDTH), "2")
        self.assertEqual(algH.getPropertyValue(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT), "2")
        self.assertEqual(algH.getPropertyValue(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT), "2")

    def checkOutput(self, ws_group, n_ws):
        self.assertTrue(isinstance(ws_group, WorkspaceGroup))
        self.assertEqual(ws_group.getNumberOfEntries(), n_ws)
        for ws in ws_group:
            self.assertTrue(isinstance(ws, MatrixWorkspace))
            self.assertEqual(ws.getNumberHistograms(), 1)
            self.assertFalse(ws.isHistogramData())
            self.assertEqual(ws.getAxis(0).getUnit().unitID(), "MomentumTransfer")
            self.assertTrue(ws.hasDx(0))


if __name__ == "__main__":
    unittest.main()

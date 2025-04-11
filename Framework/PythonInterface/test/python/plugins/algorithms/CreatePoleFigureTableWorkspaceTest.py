# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    CreatePoleFigureTableWorkspace,
    AnalysisDataService,
    SetSample,
    SetGoniometer,
    CreateWorkspace,
    EditInstrumentGeometry,
    CloneWorkspace,
    CreateEmptyTableWorkspace,
)
from mantid.geometry import CrystalStructure
import numpy as np


class CreatePoleFigureTableTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ws = CreateWorkspace(DataX=[0.0, 1.0], DataY=[1.0, 2.0, 3.0, 4.0], NSpec=4)

        # set the instrument to have four detectors
        EditInstrumentGeometry(
            Workspace="ws",
            PrimaryFlightPath=50,
            L2=[50, 50, 50, 50],
            Polar=[85, 90, 95, 90],
            Azimuthal=[0, 0, 0, 45],
            DetectorIDs=[0, 1, 2, 3],
        )
        # polar is taken from z towards positive x. Here there are 2 detectors along x axis and 2 at +/-5degs
        # azimuthal is taken from z as well. Here 3 in XZ plane, 1 at +45 degrees towards Y

        xtal = CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.05")

        sample_shape = """
        <cylinder id="A">
          <centre-of-bottom-base x="0" y="0" z="0.05" />
          <axis x="0" y="0" z="-1" />
          <radius val="0.1" />
          <height val="0.1" />
        </cylinder>
        """

        ws2 = CloneWorkspace(ws)

        SetGoniometer("ws2", Axis0="0,0,1,0,1")
        SetSample("ws", Geometry={"Shape": "CSG", "Value": sample_shape})
        ws.sample().setCrystalStructure(xtal)

        SetGoniometer("ws2", Axis0="45,0,1,0,1")
        SetSample("ws2", Geometry={"Shape": "CSG", "Value": sample_shape})
        ws2.sample().setCrystalStructure(xtal)

        peak_param_table = CreateEmptyTableWorkspace(OutputWorkspace="PeakParameterWS")
        for col in ("I", "X0", "chi2"):
            peak_param_table.addColumn("double", col)
        for i in range(4):
            val = 1.0 + (i / 10)
            peak_param_table.addRow([val, 3.14 + (i / 100), i])  # 3.14 hkl (1,1,1) dSpacing

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.Instance().clear()

    def test_alg_with_default_parameters(self):
        outws = CreatePoleFigureTableWorkspace(InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws")
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        # det 1 along X axis, det 0 is 5 degrees towards +Z
        # det 1 Q is at -45 degrees from X axis (it points towards -Z)
        # det 0 Q is at -42.5 degrees
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5, -45))))

        # beta
        # both det 0 and det 1 are in plane, beta = 90 degrees from Y
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90))))

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("Intensity"), np.array((1.0, 1.1)))

    def test_alg_with_goniometer_applied(self):
        # ws 2 is rotated 45 degrees away from +Z
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws2", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", Chi2Threshold=0.0
        )
        self.assertEqual(outws.rowCount(), 4)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        # det 1 along X axis, det 0 is 5 degrees towards +Z
        # det 1 Q is at -45 degrees from X axis (it points towards -Z)
        # det 0 Q is at -42.5 degrees
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((2.5, 0, -2.5, -9.74))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90, 90, 60))))
        # intensity
        self.eval_arrays(outws.column("Intensity"), np.array((1.0, 1.1, 1.2, 1.3)))

    def test_alg_with_chi_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", Chi2Threshold=5
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # det 0 Q is (1/sqrt2,0,1/sqrt2) but det 3 Q is (0.5,0.5,1/sqrt2)
        # det 3 alpha is -54.74 degs
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5, -45, -47.5, -54.74))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90, 90, 60))))
        # intensity
        self.eval_arrays(outws.column("Intensity"), np.array((1.0, 1.1, 1.2, 1.3)))

    def test_alg_with_hkl(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection="(1,1,1)",
            PeakPositionThreshold=0.0,
            Chi2Threshold=0.0,
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # det 0 Q is (1/sqrt2,0,1/sqrt2) but det 3 Q is (0.5,0.5,1/sqrt2)
        # det 3 alpha is -54.74 degs
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5, -45, -47.5, -54.74))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90, 90, 60))))
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("Intensity"), np.array((1.0, 1.1, 1.2, 1.3)) / scat_power)

    def test_alg_with_hkl_and_pos_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection="(1,1,1)",
            PeakPositionThreshold=0.01,
            Chi2Threshold=0.0,
        )
        self.assertEqual(outws.rowCount(), 1)  # only det 0 should be within range

        # alpha
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5,))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90,))))
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("Intensity"), np.array((1.0,)) / scat_power)

    def test_alg_with_hkl_and_pos_thresh_and_chi_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection="(1,1,1)",
            PeakPositionThreshold=0.02,
            Chi2Threshold=0.5,
        )
        self.assertEqual(outws.rowCount(), 1)  # det 0 and 1 should be in pos thresh but only det 0 in chi2 thresh

        # alpha
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5,))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90,))))
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("Intensity"), np.array((1.0,)) / scat_power)

    def eval_arrays(self, arr1, arr2, thresh=0.001):
        self.assertTrue(np.all(np.abs(arr1 - arr2) < thresh))


if __name__ == "__main__":
    unittest.main()

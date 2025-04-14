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
    AlgorithmManager,
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
    def setUp(self):
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

    def tearDown(self):
        AnalysisDataService.clear()

    def eval_arrays(self, arr1, arr2, thresh=0.001):
        self.assertTrue(np.all(np.abs(arr1 - arr2) < thresh))

    def setup_missing_column(self, column_name):
        # override PeakParameter Table
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn(column_name)

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
        alg.setProperty("PeakParameterWorkspace", peak_param_table)
        alg.setProperty("OutputWorkspace", "outws")
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["PeakParameterWorkspace"], f"PeakParameterWorkspace must have column: '{column_name}'")

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

    def test_if_no_peak_param_table_default_intensity_used(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            OutputWorkspace="outws",
        )
        self.assertEqual(outws.rowCount(), 4)
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-42.5, -45, -47.5, -54.74))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90, 90, 60))))
        # intensity
        self.eval_arrays(outws.column("Intensity"), np.array((1.0, 1.0, 1.0, 1.0)))

    def test_chi2_not_required_if_threshold_is_set_to_zero(self):
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn("chi2")

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
        alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("Chi2Threshold", 0.0)
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 0)

    def test_x0_not_required_if_threshold_is_set_to_zero(self):
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn("X0")

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
        alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("PeakPositionThreshold", 0.0)
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 0)

    def test_suitable_hkl_can_be_parsed(self):
        valid_hkl_strings = ("1,1,1", "(1,0,0)", "[1,2,3]")
        for hkl in valid_hkl_strings:
            alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
            alg.initialize()
            alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
            alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
            alg.setProperty("OutputWorkspace", "outws")
            alg.setProperty("Reflection", hkl)
            issues = alg.validateInputs()
            self.assertEqual(len(issues), 0)

    # failure cases

    def test_error_if_no_intensity_on_peak_parameter_workspace(self):
        self.setup_missing_column("I")

    def test_error_if_no_x0_on_peak_parameter_workspace(self):
        self.setup_missing_column("X0")

    def test_error_if_no_chi2_on_peak_parameter_workspace_and_thresh_is_given(self):
        self.setup_missing_column("chi2")

    def test_error_if_hkl_given_but_no_sample(self):
        CreateWorkspace(DataX=[0.0, 1.0], DataY=[1.0, 2.0, 3.0, 4.0], NSpec=4, OutputWorkspace="bad_ws")

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("bad_ws"))
        alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("Reflection", "(1,1,1)")
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["InputWorkspace"], "If reflection is specified: InputWorkspace must have a sample")

    def test_error_if_hkl_given_but_no_crystal_structure(self):
        CreateWorkspace(DataX=[0.0, 1.0], DataY=[1.0, 2.0, 3.0, 4.0], NSpec=4, OutputWorkspace="bad_ws")
        sample_shape = """
        <cylinder id="A">
          <centre-of-bottom-base x="0" y="0" z="0.05" />
          <axis x="0" y="0" z="-1" />
          <radius val="0.1" />
          <height val="0.1" />
        </cylinder>
        """
        SetSample("bad_ws", Geometry={"Shape": "CSG", "Value": sample_shape})

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("bad_ws"))
        alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("Reflection", "(1,1,1)")
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["InputWorkspace"], "If reflection is specified: InputWorkspace sample must have a CrystalStructure")

    def test_error_if_hkl_cannot_be_parsed(self):
        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
        alg.setProperty("PeakParameterWorkspace", AnalysisDataService.retrieve("PeakParameterWS"))
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("Reflection", "1 and 1 and 1")
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["Reflection"], "Problem parsing hkl string, ensure H,K,L are separated by commas")


if __name__ == "__main__":
    unittest.main()

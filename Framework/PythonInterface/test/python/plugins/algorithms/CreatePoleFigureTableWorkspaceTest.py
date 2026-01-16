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
from mantid.kernel import V3D


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

        self.default_alphas = np.deg2rad(np.array((-42.5, -45, -47.5, -54.74)))
        self.default_betas = np.deg2rad(np.array((90, 90, 90, 60)))

    def tearDown(self):
        AnalysisDataService.clear()

    def eval_arrays(self, arr1, arr2, thresh=0.005):
        self.assertTrue(np.all(np.abs(arr1 - arr2) < thresh))

    def setup_missing_column(self, column_name, readout="I"):
        # override PeakParameter Table
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn(column_name)

        alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
        alg.initialize()
        alg.setProperty("InputWorkspace", AnalysisDataService.retrieve("ws"))
        alg.setProperty("PeakParameterWorkspace", peak_param_table)
        alg.setProperty("OutputWorkspace", "outws")
        alg.setProperty("ReadoutColumn", readout)
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
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:2])

        # beta
        # both det 0 and det 1 are in plane, beta = 90 degrees from Y
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_include_spectrum_info_true_adds_columns_and_values_match_filtered_indices(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            IncludeSpectrumInfo=True,
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        data_ws = outws.column("DataWorkspace")
        ws_index = outws.column("WorkspaceIndex")

        # DataWorkspace should be the name of the input workspace used to create the table
        self.assertEqual(data_ws, ["ws", "ws"])
        # WorkspaceIndex should be spec inds in the input workspace
        self.assertEqual(ws_index, [0, 1])

    def test_include_spectrum_info_true_all_spectra_included_indices_0_to_3(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Chi2Threshold=5,  # allow all chi2
            IncludeSpectrumInfo=True,
        )
        self.assertEqual(outws.rowCount(), 4)

        data_ws = outws.column("DataWorkspace")
        ws_index = outws.column("WorkspaceIndex")

        self.assertEqual(data_ws, ["ws", "ws", "ws", "ws"])
        self.assertEqual(ws_index, [0, 1, 2, 3])

    def test_flipping_td_negates_alphas(self):
        # alpha is taken from rd, if td is inverted the q vectors are now at -alpha rather than alpha
        ax_transform = np.array(((1, 0, 0), (0, 1, 0), (0, 0, -1))).reshape(-1)
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", AxesTransform=ax_transform
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        self.eval_arrays(outws.column("Alpha"), -self.default_alphas[:2])

        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_flipping_rd_adds_pi_to_alphas(self):
        # alpha is taken from rd, if rd is inverted the q vectors are now at alpha +/- pi rather than alpha
        ax_transform = np.array(((-1, 0, 0), (0, 1, 0), (0, 0, 1))).reshape(-1)
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", AxesTransform=ax_transform
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        self.eval_arrays(outws.column("Alpha"), -(self.default_alphas[:2] + np.pi))

        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_flipping_nd_does_nothing(self):
        # negative normal directions are always inverted so expect the same result
        ax_transform = np.array(((1, 0, 0), (0, -1, 0), (0, 0, 1))).reshape(-1)
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", AxesTransform=ax_transform
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:2])

        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_providing_identity_does_nothing(self):
        # negative normal directions are always inverted so expect the same result
        ax_transform = np.array(((1, 0, 0), (0, 1, 0), (0, 0, 1))).reshape(-1)
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", AxesTransform=ax_transform
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:2])

        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_rotating_rd_and_td_pi_by_4_about_nd_adds_pi_by_4_to_alphas(self):
        # alpha is taken from rd, if rd and td are rotated pi/4 about nd, the Q vectors are rotated pi/4 relative to rd
        ir2 = 1 / np.sqrt(2)
        ax_transform = np.array(((ir2, 0, ir2), (0, 1, 0), (-ir2, 0, ir2))).reshape(-1)
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", AxesTransform=ax_transform
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:2] + (np.pi / 4))

        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # intensity
        # det 0 has 1.0, det 1 has 1.1
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1)))

    def test_alg_with_different_readout_column(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", ReadoutColumn="chi2"
        )
        self.assertEqual(outws.rowCount(), 2)  # default args should have only chi2 < 2 (det 0 and det 1)

        # alpha
        # det 1 along X axis, det 0 is 5 degrees towards +Z
        # det 1 Q is at -45 degrees from X axis (it points towards -Z)
        # det 0 Q is at -42.5 degrees
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:2])

        # beta
        # both det 0 and det 1 are in plane, beta = 90 degrees from Y
        self.eval_arrays(outws.column("Beta"), self.default_betas[:2])

        # final column should be chi2
        # det 0 has 0, det 1 has 1
        self.eval_arrays(outws.column("chi2"), np.array((0.0, 1.0)))

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
        self.eval_arrays(outws.column("Alpha"), self.default_alphas + (np.pi / 4))
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas)
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1, 1.2, 1.3)))

    def test_alg_with_chi_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", PeakParameterWorkspace="PeakParameterWS", OutputWorkspace="outws", Chi2Threshold=5
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # det 0 Q is (1/sqrt2,0,1/sqrt2) but det 3 Q is (0.5,0.5,1/sqrt2)
        # det 3 alpha is -54.74 degs
        self.eval_arrays(outws.column("Alpha"), self.default_alphas)
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas)
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1, 1.2, 1.3)))

    def test_alg_with_hkl(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
            PeakPositionThreshold=0.0,
            Chi2Threshold=0.0,
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # det 0 Q is (1/sqrt2,0,1/sqrt2) but det 3 Q is (0.5,0.5,1/sqrt2)
        # det 3 alpha is -54.74 degs
        self.eval_arrays(outws.column("Alpha"), self.default_alphas)
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas)
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1, 1.2, 1.3)) / scat_power)

    def test_alg_with_hkl_and_pos_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
            PeakPositionThreshold=0.01,
            Chi2Threshold=0.0,
        )
        self.assertEqual(outws.rowCount(), 1)  # only det 0 should be within range

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:1])
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:1])
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("I"), np.array((1.0,)) / scat_power)

    def test_alg_with_hkl_and_pos_thresh_not_adjust_for_scatt_power_if_flag_false(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
            PeakPositionThreshold=0.01,
            Chi2Threshold=0.0,
            ApplyScatteringPowerCorrection=False,
        )
        self.assertEqual(outws.rowCount(), 1)  # only det 0 should be within range

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:1])
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:1])
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.0,)))

    def test_alg_with_hkl_and_pos_thresh_and_chi_thresh(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
            PeakPositionThreshold=0.02,
            Chi2Threshold=0.5,
        )
        self.assertEqual(outws.rowCount(), 1)  # det 0 and 1 should be in pos thresh but only det 0 in chi2 thresh

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[:1])
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[:1])
        # intensity
        scat_power = 13.58
        self.eval_arrays(outws.column("I"), np.array((1.0,)) / scat_power)

    def test_alg_with_no_hkl_that_pos_thresh_applied_from_mean_x0(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            PeakPositionThreshold=0.01,
            Chi2Threshold=0.0,
        )
        self.assertEqual(outws.rowCount(), 2)  # x0s are [3.14, 3.15, 3.16, 3.17] mean is 3.155

        # alpha
        self.eval_arrays(outws.column("Alpha"), self.default_alphas[1:3])
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas[1:3])
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.1, 1.2)))

    def test_if_no_peak_param_table_default_intensity_used(self):
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws", OutputWorkspace="outws", Reflection=(1, 1, 1), ApplyScatteringPowerCorrection=True
        )
        self.assertEqual(outws.rowCount(), 4)
        self.eval_arrays(outws.column("Alpha"), self.default_alphas)
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas)
        # intensity should not be corrected for scattering power even though flag is true (as it is by default)
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.0, 1.0, 1.0)))

    def test_chi2_not_required_if_threshold_is_set_to_zero(self):
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn("chi2")

        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
            Chi2Threshold=0.0,
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 0)

    def test_x0_not_required_if_threshold_is_set_to_zero(self):
        peak_param_table = AnalysisDataService.retrieve("PeakParameterWS")
        peak_param_table.removeColumn("X0")

        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
            PeakPositionThreshold=0.0,
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 0)

    def test_suitable_hkl_can_be_parsed(self):
        valid_hkl_strings = ((1, 0, 0), [1, 2, 3])
        for hkl in valid_hkl_strings:
            alg = _init_alg(
                InputWorkspace=AnalysisDataService.retrieve("ws"),
                PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
                OutputWorkspace="outws",
                Reflection=hkl,
            )
            issues = alg.validateInputs()
            self.assertEqual(len(issues), 0)

    def _setup_offset_rod(self, offset):
        offset_rod_shape = """
                        <cylinder id="A">
                          <centre-of-bottom-base x="0.125" y="0" z="0" />
                          <axis x="-1" y="0" z="0" />
                          <radius val="0.1" />
                          <height val="0.25" />
                        </cylinder>
                        """
        # here the sample is a 25cm rod with centre of mass at the origin
        SetSample("ws", Geometry={"Shape": "CSG", "Value": offset_rod_shape})
        xtal = CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.05")
        ws = AnalysisDataService.retrieve("ws")
        ws.sample().setCrystalStructure(xtal)
        # offset the centre of mass
        comp_info = ws.componentInfo()
        comp_info.setPosition(comp_info.sample(), V3D(*offset))

    def test_sample_offset_using_set_pos_with_same_beam_centre(self):
        sample_offset = [0.075, 0.0, 0.0]
        self._setup_offset_rod(sample_offset)

        # for the situation where the beam is still defining a gauge vol at (0,0,0) we want UseSamplePosition=False
        # as the Q vectors should still be defined from this point
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            PeakPositionThreshold=0.0,
            Chi2Threshold=0.0,
            ScatteringVolumePosition=(0, 0, 0),
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # det 0 Q is (1/sqrt2,0,1/sqrt2) but det 3 Q is (0.5,0.5,1/sqrt2)
        # det 3 alpha is -54.74 degs
        self.eval_arrays(outws.column("Alpha"), self.default_alphas)
        # beta
        self.eval_arrays(outws.column("Beta"), self.default_betas)
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1, 1.2, 1.3)))

    def test_offset_shape_fully_illuminated(self):
        sample_offset = [0.075, 0.0, 0.0]
        self._setup_offset_rod(sample_offset)

        # for the situation where the rod is fully illuminated we want UseSamplePosition=True
        # as the Q vectors should be approximated from the COM, which is now offset
        outws = CreatePoleFigureTableWorkspace(
            InputWorkspace="ws",
            PeakParameterWorkspace="PeakParameterWS",
            OutputWorkspace="outws",
            PeakPositionThreshold=0.0,
            Chi2Threshold=0.0,
            ScatteringVolumePosition=sample_offset,
        )
        self.assertEqual(outws.rowCount(), 4)  # all should have suitable chi2

        # alpha
        # qi is now different so Q and alpha and beta all change accordingly
        self.eval_arrays(outws.column("Alpha"), np.deg2rad(np.array((-44.742, -47.23, -49.72, -57.70))))
        # beta
        self.eval_arrays(outws.column("Beta"), np.deg2rad(np.array((90, 90, 90, 59.13))))
        # intensity
        self.eval_arrays(outws.column("I"), np.array((1.0, 1.1, 1.2, 1.3)))

    # failure cases
    def test_error_if_num_spectra_does_not_match_param_ws_rows(self):
        peak_param_table = CreateEmptyTableWorkspace(OutputWorkspace="BadPeakParameterWS")
        for col in ("I", "X0", "chi2"):
            peak_param_table.addColumn("double", col)
        for i in range(5):
            peak_param_table.addRow([1.0, 1.0, 1.0])  # values don't matter
        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("BadPeakParameterWS"),
            OutputWorkspace="outws",
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(
            issues["PeakParameterWorkspace"], "PeakParameterWorkspace must have same number of rows as Input Workspace has spectra"
        )

    def test_error_if_no_intensity_on_peak_parameter_workspace_and_that_is_readout(self):
        self.setup_missing_column("I", "I")

    def test_error_if_no_x0_on_peak_parameter_workspace(self):
        self.setup_missing_column("X0")

    def test_error_if_no_chi2_on_peak_parameter_workspace_and_thresh_is_given(self):
        self.setup_missing_column("chi2")

    def test_error_if_hkl_given_but_no_sample(self):
        CreateWorkspace(DataX=[0.0, 1.0], DataY=[1.0, 2.0, 3.0, 4.0], NSpec=4, OutputWorkspace="bad_ws")

        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("bad_ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["InputWorkspace"], "If reflection is specified: InputWorkspace sample must have a CrystalStructure")

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

        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("bad_ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
            Reflection=(1, 1, 1),
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues["InputWorkspace"], "If reflection is specified: InputWorkspace sample must have a CrystalStructure")

    def test_error_if_bad_transform_matrix_given(self):
        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
            AxesTransform=np.ones(9),
        )
        issues = alg.validateInputs()
        self.assertEqual(len(issues), 1)
        self.assertEqual(
            issues["AxesTransform"],
            "Algorithm currently expects axes transforms to volume-preserving, as such the determinant must equal 1 (or -1)",
        )

    def test_error_if_hkl_cannot_be_parsed(self):
        alg = _init_alg(
            InputWorkspace=AnalysisDataService.retrieve("ws"),
            PeakParameterWorkspace=AnalysisDataService.retrieve("PeakParameterWS"),
            OutputWorkspace="outws",
        )
        self.assertRaises(ValueError, alg.setProperty, "Reflection", "1 and 1 and 1")


def _init_alg(**kwargs):
    alg = AlgorithmManager.create("CreatePoleFigureTableWorkspace")
    alg.initialize()
    for prop, value in kwargs.items():
        alg.setProperty(prop, value)
    return alg


if __name__ == "__main__":
    unittest.main()

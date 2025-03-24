# ruff: noqa: E741  # Ambiguous variable name
import unittest
from unittest.mock import patch, call
from numpy import allclose
from mantid.simpleapi import (
    LoadEmptyInstrument,
    AnalysisDataService,
    CreatePeaksWorkspace,
    CreateMDWorkspace,
    SetUB,
    IndexPeaks,
    CreateSampleWorkspace,
    SetSample,
    TransformHKL,
    IntegratePeaksMD,
    FakeMDEventData,
    SetGoniometer,
)
from mantid.dataobjects import Workspace2D
from mantid.kernel import V3D
from Diffraction.single_crystal.base_sx import BaseSX
import tempfile
import shutil
from os import path
import numpy as np
from scipy.spatial.transform import Rotation

base_sx_path = "Diffraction.single_crystal.base_sx"


class BaseSXTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty")
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

    def test_retrieve(self):
        self.assertTrue(isinstance(BaseSX.retrieve("empty"), Workspace2D))  # ADS name of self.ws is "empty"
        self.assertTrue(isinstance(BaseSX.retrieve(self.ws), Workspace2D))

    def test_remove_duplicate_peaks_by_hkl(self):
        peaks = self._make_peaks_HKL(hs=[1.15, 1.01, 0.95, 1.05, 2], wsname="peaks1")

        BaseSX.remove_duplicate_peaks_by_hkl(peaks)

        self.assertEqual(2, peaks.getNumberPeaks())
        self.assertEqual(1.01, peaks.getPeak(0).getH())
        self.assertEqual(2, peaks.getPeak(1).getH())

    def test_remove_duplicate_peaks_by_qlab(self):
        peaks = self._make_peaks_qsample(qxs=[1.15, 1.01, 0.95, 1.05, 2], wsname="peaks2")

        BaseSX.remove_duplicate_peaks_by_qlab(peaks, q_tol=0.06)

        self.assertEqual(4, peaks.getNumberPeaks())
        # assert rough agreement as when add a peak in Q it moves it to the nearest detector
        for ipk, qx in enumerate([2, 1.15, 1.05, 0.95]):
            self.assertAlmostEqual(qx, peaks.getPeak(ipk).getQLabFrame()[0], delta=0.01)

    def test_remove_forbidden_peaks(self):
        peaks = self._make_peaks_HKL(hs=[1, 2], ls=[2], wsname="peaks3")

        BaseSX.remove_forbidden_peaks(peaks, "F d -3 m")

        self.assertEqual(1, peaks.getNumberPeaks())
        self.assertEqual(2, peaks.getPeak(0).getH())

    def test_remove_peaks_near_powder_line(self):
        peaks = self._make_peaks_HKL(wsname="peaks4")

        BaseSX.remove_peaks_near_powder_line(peaks, phase="Al")

        self.assertEqual(2, peaks.getNumberPeaks())
        self.assertEqual(0, peaks.getPeak(0).getH())
        self.assertEqual(1, peaks.getPeak(1).getH())

    def test_remove_non_integrated_peaks(self):
        peaks = self._make_peaks_qsample(wsname="peaks5")
        peaks.getPeak(0).setIntensity(1.0)
        peaks.getPeak(0).setSigmaIntensity(1.0)

        peaks = BaseSX.remove_non_integrated_peaks(peaks)  # workspace replaced in ADS so return output table

        self.assertEqual(1, peaks.getNumberPeaks())
        self.assertEqual(0.0, peaks.getPeak(0).getQLabFrame()[0])

    def test_remove_non_integrated_peaks_min_intens_over_sigma(self):
        peaks = self._make_peaks_qsample(wsname="peaks5")
        peaks.getPeak(0).setIntensity(1.0)
        peaks.getPeak(0).setSigmaIntensity(1.0)

        peaks = BaseSX.remove_non_integrated_peaks(peaks, min_intens_over_sigma=3)

        self.assertEqual(0, peaks.getNumberPeaks())

    def test_remove_non_indexed_peaks(self):
        peaks = self._make_peaks_HKL(hs=[1.15, 2], wsname="peaks6")
        IndexPeaks(peaks, Tolerance=0.1)  # will not index first peak

        peaks = BaseSX.remove_unindexed_peaks(peaks)  # workspace replaced in ADS so return output table

        self.assertEqual(1, peaks.getNumberPeaks())
        self.assertEqual(2, peaks.getPeak(0).getH())

    def test_get_xunit(self):
        self.assertEqual(BaseSX.get_xunit(self.ws), "TOF")

    @patch(base_sx_path + ".mantid.CropWorkspace")
    @patch(base_sx_path + ".mantid.ConvertUnits")
    def test_crop_ws_with_ws_in_target_unit(self, mock_conv_units, mock_crop):
        xmin, xmax = 0.5, 1.0
        BaseSX.crop_ws(self.ws, xmin=xmin, xmax=xmax, xunit="TOF")

        mock_crop.assert_called_once_with(InputWorkspace="empty", OutputWorkspace="empty", XMin=xmin, XMax=xmax, EnableLogging=False)
        mock_conv_units.assert_not_called()

    @patch(base_sx_path + ".mantid.CropWorkspace")
    @patch(base_sx_path + ".mantid.ConvertUnits")
    def test_crop_ws_with_ws_not_in_target_unit(self, mock_conv_units, mock_crop):
        target_unit = "Wavelength"
        xmin, xmax = 0.5, 1.0
        BaseSX.crop_ws(self.ws, xmin=xmin, xmax=xmax, xunit=target_unit)

        mock_crop.assert_called_once_with(InputWorkspace="empty", OutputWorkspace="empty", XMin=xmin, XMax=xmax, EnableLogging=False)
        mock_conv_units.assert_has_calls(
            [
                call(InputWorkspace="empty", OutputWorkspace="empty", Target=target_unit, EnableLogging=False),
                call(InputWorkspace="empty", OutputWorkspace="empty", Target="TOF", EnableLogging=False),
            ]
        )

    def test_has_sample(self):
        # test with no sample
        self.assertFalse(BaseSX.has_sample(self.ws))

        # test with sample set
        ws = CreateSampleWorkspace()
        SetSample(ws, Material={"ChemicalFormula": "V0.95-Nb0.05", "SampleNumberDensity": 0.0722})
        self.assertTrue(BaseSX.has_sample(ws))

    def test_get_radius(self):
        peaks = self._make_peaks_HKL(hs=[1], wsname="peaks7")
        ispec = self.ws.getIndicesFromDetectorIDs(peaks.column("DetID"))[0]

        self.assertAlmostEqual(0.01020, BaseSX.get_radius(peaks.getPeak(0), self.ws, ispec, scale=1), delta=1e-4)

    @patch(base_sx_path + ".mantid.IntegratePeaksMD")
    def test_integrate_peaks_md(self, mock_integrate):
        BaseSX.integrate_peaks_MD("md", "pks_in", "pks_out", IntegrateIfOnEdge=True)  # overwrite a default value
        mock_integrate.assert_called_once_with(
            InputWorkspace="md",
            PeaksWorkspace="pks_in",
            OutputWorkspace="pks_out",
            IntegrateIfOnEdge=True,
            UseOnePercentBackgroundCorrection=False,
        )

    def test_make_UB_consistent(self):
        peaks_ref = self._make_peaks_HKL(hs=[-1], wsname="peaks8")
        peaks = self._make_peaks_HKL(hs=[-1], wsname="peaks9")
        TransformHKL(peaks, HKLTransform="0,1,0,1,0,0,0,0,-1", FindError=False)

        BaseSX.make_UB_consistent(peaks_ref, peaks)

        self.assertTrue(
            allclose(peaks_ref.sample().getOrientedLattice().getUB().tolist(), peaks.sample().getOrientedLattice().getUB().tolist())
        )

    def test_plot_integrated_peaks_MD(self):
        # make fake dataset
        ws = CreateMDWorkspace(
            Dimensions="3",
            Extents="-5,5,-5,5,-5,5",
            Names="H,K,L",
            Units="r.l.u.,r.l.u.,r.l.u.",
            Frames="HKL,HKL,HKL",
            SplitInto="2",
            SplitThreshold="50",
        )
        # generate fake data at (0,1,0)
        FakeMDEventData(ws, EllipsoidParams="1e+03,0,1,0,1,0,0,0,1,0,0,0,1,0.01,0.04,0.02,1", RandomSeed="3873875")
        # create peak at (0,1,0) and integrate
        peaks = self._make_peaks_HKL(hs=[1], ks=[1, 6], ls=[1], wsname="peaks_md")
        peaks_int = IntegratePeaksMD(
            InputWorkspace=ws,
            PeakRadius=0.6,
            BackgroundInnerRadius=0.6,
            BackgroundOuterRadius=0.8,
            PeaksWorkspace=peaks,
            OutputWorkspace="peaks_int_sphere",
            Ellipsoid=False,
            UseOnePercentBackgroundCorrection=False,
        )

        # plot
        out_file = path.join(self._test_dir, "out_plot_MD.pdf")
        BaseSX.plot_integrated_peaks_MD(ws, peaks_int, out_file, nbins_max=21, extent=1.5, log_norm=True)

        # check output file saved
        self.assertTrue(path.exists(out_file))

    def test_optimize_goniometer_axis_fix_angles_True_apply_False(self):
        phis = [0, 15, 45]
        pk_ws_list, new_axis = self._setup_peaks_for_goniometer_optimisation(phis)
        R_nominal = pk_ws_list[-1].getPeak(0).getGoniometerMatrix().copy()

        axes, angles = BaseSX.optimize_goniometer_axis(pk_ws_list, iaxis=1, euler_axes="yz", fix_angles=True)

        self.assertTrue(V3D(*new_axis).angle(V3D(*axes[1])) < 1e-3)
        self.assertTrue(np.allclose(angles[:, 1], phis))
        self.assertTrue(np.allclose(R_nominal, pk_ws_list[-1].getPeak(0).getGoniometerMatrix()))

    def test_optimize_goniometer_axis_fix_angles_True_apply_True(self):
        phis = [0, 15, 45]
        pk_ws_list, new_axis = self._setup_peaks_for_goniometer_optimisation(phis)
        R_nominal = pk_ws_list[-1].getPeak(0).getGoniometerMatrix().copy()

        axes, angles = BaseSX.optimize_goniometer_axis(pk_ws_list, iaxis=1, euler_axes="yz", fix_angles=True, apply=True)

        self.assertTrue(V3D(*new_axis).angle(V3D(*axes[1])) < 1e-3)
        self.assertTrue(np.allclose(angles[:, 1], phis))
        self.assertFalse(np.allclose(R_nominal, pk_ws_list[-1].getPeak(0).getGoniometerMatrix()))

    def test_optimize_goniometer_axis_fix_angles_False(self):
        phis = [0, 15, 45]
        pk_ws_list, new_axis = self._setup_peaks_for_goniometer_optimisation(phis)

        axes, angles = BaseSX.optimize_goniometer_axis(pk_ws_list, iaxis=1, euler_axes="yz", fix_angles=False)

        self.assertTrue(V3D(*new_axis).angle(V3D(*axes[1])) < 1e-3)
        self.assertTrue(np.allclose(angles[:, 1], [0.0631, 17, 47], atol=1e-3))

    def test_find_consistent_ub(self):
        ub = np.diag([0.25, 0.5, 0.1])
        gonio_angle = 3
        offset = 0.5  # angle to offset nominal goniometer rotation
        ub_rot = Rotation.from_rotvec([0, gonio_angle, 0], degrees=True).as_matrix() @ ub
        peaks_ref = self._make_peaks_HKL(hs=[1], ks=range(1, 6), ls=[1], wsname="peaks_ref", ub=ub)
        peaks_rot = self._make_peaks_HKL(hs=[1], ks=range(1, 6), ls=[1], wsname="peaks_rot", ub=ub_rot)

        # reset the UB to the unrotated one and set the goniometer matrix at a slightly different angle
        SetUB(peaks_rot, ub=ub)
        SetGoniometer(peaks_rot, Axis0=f"{gonio_angle + offset},0,1,0,1")
        [pk.setGoniometerMatrix(peaks_rot.run().getGoniometer().getR()) for pk in BaseSX.retrieve(peaks_rot)]

        BaseSX.find_consistent_ub(peaks_ref, peaks_rot)

        # expect UB to be rotated by 1 degree around Y
        u_expected = Rotation.from_rotvec([0, -offset, 0], degrees=True).as_matrix()
        u_found = peaks_rot.sample().getOrientedLattice().getU()
        difference_angle = Rotation.from_matrix(u_found.T @ u_expected).magnitude()
        self.assertTrue(np.isclose(difference_angle, 0))

    # --- helper funcs ---

    def _make_peaks_HKL(self, hs=None, ks=[0], ls=[1], wsname="peaks_hkl", ub=np.diag([0.25, 0.25, 0.1])):
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace=wsname)
        SetUB(peaks, UB=ub)
        if hs is None:
            hs = range(3)
        for h in hs:
            for k in ks:
                for l in ls:
                    try:
                        peaks.addPeak(peaks.createPeakHKL([h, k, l]))
                    except ValueError:
                        pass

        return peaks

    def _make_peaks_qsample(self, qxs=None, qys=[0], qzs=[1], wsname="peaks_qsamp"):
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace=wsname)
        if qxs is None:
            qxs = range(3)
        for qx in qxs:
            for qy in qys:
                for qz in qzs:
                    peaks.addPeak(peaks.createPeakQSample([qx, qy, qz]))
        return peaks

    def _setup_peaks_for_goniometer_optimisation(self, phis=[0, 15, 45]):
        b = np.diag([0.25, 0.5, 0.1])  # B matrix (of UB) - here assume U = I
        # calibrated goniometer axis to be found - not quite along X
        new_axis = np.array([1, 0, 0.05])
        new_axis = new_axis / np.linalg.norm(new_axis)

        # create peaks and set goniometer using nominal axis along X (as well as vertical)
        omegas = [0, 10, 10]  # around vertical
        pk_ws_list = []
        for iphi, phi in enumerate(phis):
            peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, OutputWorkspace=f"peaks_{iphi}", NumberOfPeaks=1)
            SetGoniometer(peaks, Axis0=f"{omegas[iphi]},0,1,0,1", Axis1=f"{phi},1,0,0,1")
            R_nominal = peaks.run().getGoniometer().getR().copy()
            peaks.getPeak(0).setGoniometerMatrix(R_nominal)  # normally would be automatically on peak
            # set UB corresponding to rotation of phi+2 degrees around the calibrated axis
            this_phi = phi + 2 if not np.isclose(phi, 0) else 0
            this_R = (
                Rotation.from_rotvec([0, omegas[iphi], 0], degrees=True).as_matrix()
                @ Rotation.from_rotvec(this_phi * new_axis, degrees=True).as_matrix()
            )
            this_u = R_nominal.T @ this_R
            SetUB(Workspace=peaks, UB=this_u @ b)
            pk_ws_list.append(peaks)
        return pk_ws_list, new_axis


if __name__ == "__main__":
    unittest.main()

import unittest
from unittest.mock import MagicMock, patch, call
import tempfile
import shutil
from os import path
from mantid.simpleapi import (
    LoadEmptyInstrument,
    AnalysisDataService,
    CreatePeaksWorkspace,
    AddPeak,
    MoveInstrumentComponent,
    CloneWorkspace,
    SaveParameterFile,
    SetUB,
    ClearUB,
    HasUB,
    ClearCache,
    SetSample,
)
from Diffraction.single_crystal.sxd import SXD
from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE

sxd_path = "Diffraction.single_crystal.sxd"


class SXDTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ClearCache(InstrumentCache=True)
        cls.ws = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="empty")
        axis = cls.ws.getAxis(0)
        axis.setUnit("TOF")
        cls.peaks = cls._make_peaks_detids(cls, wsname="peaks")
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()
        shutil.rmtree(cls._test_dir)

    def setUp(self):
        self.sxd = SXD()

    def tearDown(self):
        ClearCache(InstrumentCache=True)

    # --- test BaseSX methods (parent class with abstract methods) that require class instantiation ---

    @patch(sxd_path + ".BaseSX.retrieve")
    def test_get_ws(self, mock_retrieve):
        runno = 1234
        wsname = "wsname"
        self.sxd.runs = {str(runno): {"ws": wsname}}
        mock_retrieve.side_effect = lambda name: make_mock_ws_with_name(name)

        self.assertEqual(wsname, self.sxd.get_ws_name(runno))
        self.assertIsNone(self.sxd.get_ws_name(101))

    @patch(sxd_path + ".BaseSX.retrieve")
    def test_get_md(self, mock_retrieve):
        runno = 1234
        mdname = "mdname"
        self.sxd.runs = {str(runno): {"MD": mdname}}
        mock_retrieve.side_effect = lambda name: make_mock_ws_with_name(name)

        self.assertEqual(mdname, self.sxd.get_md_name(runno))
        self.assertIsNone(self.sxd.get_md_name(101))

    def test_get_peaks_no_integration_type(self):
        runno = 1234
        pkname = "peaks"
        self.sxd.runs = {str(runno): {"found": pkname}}

        self.assertEqual(pkname, self.sxd.get_peaks_name(runno, PEAK_TYPE.FOUND))
        self.assertIsNone(self.sxd.get_peaks_name(101, PEAK_TYPE.FOUND))

    def test_get_peaks_with_integration_type(self):
        runno = 1234
        pkname = "peaks"
        self.sxd.runs = {str(runno): {"found_int_MD_opt": pkname}}

        self.assertEqual(pkname, self.sxd.get_peaks_name(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS))

    def test_set_mc_abs_nevents(self):
        self.assertEqual(1200, self.sxd.n_mcevents)
        self.sxd.set_mc_abs_nevents(5000)
        self.assertEqual(5000, self.sxd.n_mcevents)

    def test_set_van_ws(self):
        self.sxd.set_van_ws(self.ws)
        self.assertEqual("empty", self.sxd.van_ws)  # stores ws name

    def test_set_ws(self):
        # test runs is initially empty
        self.assertEqual({}, self.sxd.runs)
        # set first run
        runno = 1234
        self.sxd.set_ws(runno, self.ws)
        self.assertEqual("empty", self.sxd.get_ws_name(runno))

    def test_set_peaks_no_integration_type(self):
        runno = 1234
        self.sxd.set_ws(runno, self.ws)

        self.sxd.set_peaks(runno, self.peaks, PEAK_TYPE.FOUND)

        self.assertEqual("peaks", self.sxd.get_peaks_name(runno, PEAK_TYPE.FOUND))

    def test_set_peaks_with_integration_type(self):
        runno = 1234
        self.sxd.set_ws(runno, self.ws)  # to init dict

        self.sxd.set_peaks(runno, self.peaks, PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS)

        self.assertEqual("peaks", self.sxd.get_peaks_name(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS))

    def test_set_md(self):
        runno = 1234
        self.sxd.set_ws(runno, self.ws)

        self.sxd.set_md(runno, self.ws)

        self.assertEqual("empty", self.sxd.get_md_name(runno))  # stores ws name

    def test_set_goniometer_axes(self):
        self.sxd.set_goniometer_axes([0, 1, 0, 1], [1, 1, 0, -1])
        self.assertListEqual(["0,1,0,1", "1,1,0,-1"], self.sxd.gonio_axes)

    @patch(sxd_path + ".mantid.IntegratePeaksMD")
    def test_integrate_peaks_MD_optimal_radius(self, mock_int_md):
        ws_md = "ws_md"
        peaks = self._make_peaks_detids(detids=[2016, 2016, 10209, 10209], tofs=2 * [5000, 10000], wsname="peaks3")
        mock_int_md.side_effect = lambda *args, **kwargs: CloneWorkspace(kwargs["PeaksWorkspace"], OutputWorkspace="integrated")
        int_md_kwargs = {"IntegrateIfOnEdge": True, "Ellipsoid": True}

        out = self.sxd.integrate_peaks_MD_optimal_radius(ws_md, peaks, out_peaks="out", ws=self.ws, **int_md_kwargs)

        self.assertEqual(4, out.getNumberPeaks())
        self.assertEqual(3, mock_int_md.call_count)  # 2 peaks had similar radius and were integrated together
        radius = [mock_int_md.call_args_list[icall].kwargs["PeakRadius"] for icall in range(mock_int_md.call_count)]
        radius.sort()
        for icall, expected_radius in enumerate([0.0948, 0.1148, 0.2348]):
            self.assertAlmostEqual(radius[icall], expected_radius, delta=1e-3)
            for kwarg in int_md_kwargs:
                self.assertTrue(mock_int_md.call_args_list[icall].kwargs[kwarg])

    @patch(sxd_path + ".BaseSX.retrieve")
    @patch(sxd_path + ".mantid.SaveReflections")
    @patch(sxd_path + ".mantid.SaveNexus")
    def test_save_peak_table(self, mock_save_nxs, mock_save_ref, mock_retrieve):
        fmt = "SHELX"
        self._call_save_peak_table(mock_retrieve, save_format=fmt, SplitFiles=False)

        fpath = path.join(self._test_dir, "peaks_" + fmt)
        mock_save_nxs.assert_called_once_with(InputWorkspace=self.peaks.name(), Filename=fpath + ".nxs")
        mock_save_ref.assert_called_once_with(InputWorkspace=self.peaks.name(), Filename=fpath + ".int", Format=fmt, SplitFiles=False)

    @patch(sxd_path + ".BaseSX.retrieve")
    @patch(sxd_path + ".mantid.SaveReflections")
    @patch(sxd_path + ".mantid.SaveNexus")
    def test_save_peak_table_save_nxs_False(self, mock_save_nxs, mock_save_ref, mock_retrieve):
        fmt = "SHELX"
        self._call_save_peak_table(mock_retrieve, save_format=fmt, save_nxs=False, SplitFiles=False)

        fpath = path.join(self._test_dir, "peaks_" + fmt)
        mock_save_nxs.assert_not_called()
        mock_save_ref.assert_called_once_with(InputWorkspace=self.peaks.name(), Filename=fpath + ".int", Format=fmt, SplitFiles=False)

    @patch(sxd_path + ".mantid.SaveReflections")
    @patch(sxd_path + ".mantid.SaveNexus")
    def test_save_all_peaks(self, mock_save_nxs, mock_save_ref):
        self.sxd.runs = dict()
        for runno in range(1234, 1236):
            self.sxd.runs[str(runno)] = {"ws": self.ws.name(), "found_int_MD_opt": self.peaks.name()}
        SetUB(self.peaks, UB="0.25,0,0,0,0.25,0,0,0,0.1")

        pk_type, int_type = PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS
        fmt = "SHELX"

        self.sxd.save_all_peaks(pk_type, int_type, self._test_dir, fmt, MinIntensOverSigma=3)

        all_peaks_name = "1234-1235_found_int_MD_opt"
        fpath = path.join(self._test_dir, f"{all_peaks_name}_{fmt}")
        mock_save_nxs.assert_called_once_with(InputWorkspace=all_peaks_name, Filename=fpath + ".nxs")
        mock_save_ref.assert_called_once_with(InputWorkspace=all_peaks_name, Filename=fpath + ".int", Format=fmt, MinIntensOverSigma=3)
        self.assertTrue(HasUB(Workspace=all_peaks_name))
        ClearUB(Workspace=self.peaks)

    def test_predict_peaks(self):
        # make peaks ws and set a UB
        peaks = self._make_peaks_detids(wsname="peaks_for_predict")
        SetUB(peaks, UB="0.25,0,0,0,0.25,0,0,0,0.1")
        runno = 1234
        self.sxd.runs = {str(runno): {"ws": self.ws.name(), "found": peaks}}
        for pk_type in [PEAK_TYPE.PREDICT, PEAK_TYPE.PREDICT_SAT]:
            self.sxd.predict_peaks(run=runno, peak_type=pk_type)
            pred_peaks = self.sxd.get_peaks(runno, pk_type)
            self.assertEqual(pred_peaks.name(), peaks.name() + "_" + pk_type.value)

    @patch(sxd_path + ".SXD.set_md")
    @patch(sxd_path + ".BaseSX.convert_ws_to_MD")
    def test_convert_to_md_sets_ub_if_HKL(self, mock_conv_md, mock_set_md):
        self.peaks = self._make_peaks_detids(wsname="peaks_for_md")
        SetUB(self.peaks, UB="0.25,0,0,0,0.25,0,0,0,0.1")
        runno = 1234
        wsname = self.ws.name()
        self.sxd.runs = {str(runno): {"ws": wsname, "found": self.peaks.name()}}

        self.sxd.convert_to_MD(run=runno, frame="HKL")

        mock_conv_md.assert_called_with(wsname, wsname + "_MD", "HKL")
        mock_set_md.assert_called_with(runno, wsname + "_MD")
        # assert UB is set for HKL conversion
        self.assertTrue(HasUB(Workspace=self.ws))
        # delete UB to restore ws to initial state
        for ws in [self.ws, self.peaks]:
            ClearUB(Workspace=ws)

    def test_set_sample_with_material(self):
        self.sxd.set_sample(
            Geometry={"Shape": "CSG", "Value": self.sxd.sphere_shape}, Material={"ChemicalFormula": "C2 H4", "SampleNumberDensity": 0.02}
        )

        self.assertEqual(self.sxd.sample_dict["Material"]["NumberDensityUnit"], "Formula Units")

    def test_set_sample_without_material(self):
        self.sxd.set_sample(Geometry={"Shape": "CSG", "Value": self.sxd.sphere_shape})

        self.assertFalse("Material" in self.sxd.sample_dict)

    def test_calc_absorption_weighted_path_lengths(self):
        # make workspace with sample
        ws_with_sample = CloneWorkspace(self.ws)
        SetSample(
            ws_with_sample,
            Geometry={"Shape": "CSG", "Value": self.sxd.sphere_shape},
            Material={"ChemicalFormula": "C2 H4", "SampleNumberDensity": 0.02, "NumberDensityUnit": "Formula Units"},
        )
        # make peaks workspace and set intensity to 1
        peaks_to_correct = CloneWorkspace(self.peaks)
        peaks_to_correct.getPeak(0).setIntensity(1.0)
        # store in class
        runno = 1234
        self.sxd.set_ws(runno, ws_with_sample)
        self.sxd.set_peaks(runno, peaks_to_correct, PEAK_TYPE.FOUND)

        self.sxd.calc_absorption_weighted_path_lengths(PEAK_TYPE.FOUND)

        self.assertAlmostEqual(peaks_to_correct.getPeak(0).getIntensity(), 8.7690, delta=1e-4)

    #  --- methods specific to SXD class ---

    @patch("Diffraction.single_crystal.base_sx.mantid.SetGoniometer")
    @patch(sxd_path + ".SXD.set_ws")
    @patch(sxd_path + ".SXD.load_run")
    @patch(sxd_path + ".SXD._divide_workspaces")
    @patch(sxd_path + ".mantid.DeleteWorkspace")
    @patch(sxd_path + ".mantid.ConvertUnits")
    def test_process_data_with_gonio_angle_strings(self, mock_conv, mock_del, mock_divide, mock_load, mock_set_ws, mock_set_gonio):
        sxd = SXD(vanadium_runno=1, empty_runno=2)
        sxd.set_goniometer_axes([0, 1, 0, 1], [1, 0, 0, 0])
        sxd.van_ws = "van"
        mock_load.return_value = "wsname"
        runnos = range(3, 6)

        sxd.process_data(runnos, "log1", "log2")  # log names passed to SetGoniometer

        self.assertEqual(mock_set_gonio.call_count, len(runnos))
        expected_calls = len(runnos) * [call(Workspace="wsname", EnableLogging=False, Axis0="log1,0,1,0,1", Axis1="log2,1,0,0,0")]
        mock_set_gonio.assert_has_calls(expected_calls)

    @patch("Diffraction.single_crystal.base_sx.mantid.SetGoniometer")
    @patch(sxd_path + ".SXD.set_ws")
    @patch(sxd_path + ".SXD.load_run")
    @patch(sxd_path + ".SXD._divide_workspaces")
    @patch(sxd_path + ".mantid.DeleteWorkspace")
    @patch(sxd_path + ".mantid.ConvertUnits")
    def test_process_data_with_gonio_angle_sequences(self, mock_conv, mock_del, mock_divide, mock_load, mock_set_ws, mock_set_gonio):
        sxd = SXD(vanadium_runno=1, empty_runno=2)
        sxd.set_goniometer_axes([0, 1, 0, 1], [1, 0, 0, 0])
        sxd.van_ws = "van"
        mock_load.return_value = "wsname"
        log1_vals = [10, 20, 30]
        log2_vals = [40, 50, 60]
        runnos = range(3, 6)

        sxd.process_data(runnos, log1_vals, log2_vals)  # log names passed to SetGoniometer

        self.assertEqual(mock_set_gonio.call_count, len(runnos))
        expected_calls = []
        for irun in range(len(runnos)):
            expected_calls.append(
                call(Workspace="wsname", EnableLogging=False, Axis0=f"{log1_vals[irun]},0,1,0,1", Axis1=f"{log2_vals[irun]},1,0,0,0")
            )
        mock_set_gonio.assert_has_calls(expected_calls)

    def test_remove_remove_peaks_on_detector_edge(self):
        nedges = [1, 2, 3]
        npks = [3, 2, 1]
        for npk, nedge in zip(npks, nedges):
            peaks = self._make_peaks_detids([4222, 8189, 8056, 6657, 6113], wsname="peaks1")
            self.sxd.remove_peaks_on_detector_edge(peaks, nedge=nedge)
            self.assertEqual(npk, peaks.getNumberPeaks())

    def test_get_radius(self):
        peaks = self._make_peaks_detids([4222], wsname="peaks2")
        ispec = self.ws.getIndicesFromDetectorIDs(peaks.column("DetID"))[0]

        self.assertAlmostEqual(0.020582, self.sxd.get_radius(peaks.getPeak(0), self.ws, ispec, scale=1), delta=5e-3)

    def test_apply_calibration_xml(self):
        # clone workspaces
        ws_clone1 = CloneWorkspace(self.ws)
        ws_clone2 = CloneWorkspace(self.ws)
        peaks2 = CloneWorkspace(self.peaks)
        # move bank1 component to (0,0,0) and store new location in calibration xml file
        MoveInstrumentComponent(Workspace=ws_clone1, ComponentName="bank1", RelativePosition=False, EnableLogging=False)
        xml_path = path.join(self._test_dir, "test_calib.xml")
        SaveParameterFile(Workspace=ws_clone1, Filename=xml_path, LocationParameters=True)
        # set workspaces in sxd object
        self.sxd.set_ws("0", ws_clone1)
        self.sxd.set_ws("1", ws_clone2)
        self.sxd.set_peaks("1", peaks2)

        self.sxd.apply_calibration_xml(xml_path)

        for ws in [ws_clone1, ws_clone2, peaks2]:
            # check bank1 moved in all workspaces
            self.assertAlmostEqual(0.0, ws.getInstrument().getComponentByName("bank1").getPos()[0], delta=1e-10)

    def test_undo_calibration_xml(self):
        # clone workspaces
        ws3 = LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace="SXD_undo_cal_test")
        # move bank1 component to (0,0,0)
        MoveInstrumentComponent(Workspace=ws3, ComponentName="bank1", RelativePosition=False, EnableLogging=False)
        peaks3 = CreatePeaksWorkspace(InstrumentWorkspace=ws3, NumberOfPeaks=0, OutputWorkspace="peaks3")

        SXD.undo_calibration(ws3, peaks_ws=peaks3)
        for ws in [ws3, peaks3]:
            # check bank1 is no longer at 0 (back at original position)
            self.assertAlmostEqual(0.13, ws.getInstrument().getComponentByName("bank1").getPos()[0], delta=1e-2)

    # --- helper funcs ---

    def _make_peaks_detids(self, detids=[6113], tofs=None, wsname="peaks_detids"):
        if tofs is None:
            tofs = len(detids) * [10000]
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace=wsname)
        for idet, detid in enumerate(detids):
            AddPeak(PeaksWorkspace=peaks, RunWorkspace=self.ws, DetectorID=detid, TOF=tofs[idet], EnableLogging=False)
        return peaks

    def _call_save_peak_table(self, mock_retrieve, **kwargs):
        runno = 1234
        self.sxd.runs = {str(runno): {"ws": self.ws.name(), "found_int_MD_opt": self.peaks.name()}}
        mock_retrieve.side_effect = lambda name: make_mock_ws_with_name(name)

        self.sxd.save_peak_table(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS, self._test_dir, **kwargs)


def make_mock_ws_with_name(name):
    mock_ws = MagicMock()
    mock_ws.name.return_value = name
    return mock_ws


if __name__ == "__main__":
    unittest.main()

import unittest
from unittest.mock import MagicMock, patch
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
)
from Diffraction.single_crystal.sxd import SXD
from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE

sxd_path = "Diffraction.single_crystal.sxd"


class SXDTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
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
        for icall, radius in enumerate([0.1021, 0.1319, 0.2519]):
            self.assertAlmostEqual(radius, mock_int_md.call_args_list[icall].kwargs["PeakRadius"], delta=1e-3)
            for kwarg in int_md_kwargs:
                self.assertTrue(mock_int_md.call_args_list[icall].kwargs[kwarg])

    @patch(sxd_path + ".BaseSX.retrieve")
    @patch(sxd_path + ".mantid.SaveReflections")
    @patch(sxd_path + ".mantid.SaveNexus")
    def test_save_peak_table(self, mock_save_nxs, mock_save_ref, mock_retrieve):
        runno = 1234
        self.sxd.runs = {str(runno): {"ws": self.ws.name(), "found_int_MD_opt": self.peaks.name()}}
        pk_type, int_type = PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS
        fmt = "SHELX"
        mock_retrieve.side_effect = lambda name: make_mock_ws_with_name(name)

        self.sxd.save_peak_table(runno, pk_type, int_type, self._test_dir, fmt)

        fpath = path.join(self._test_dir, "peaks_" + fmt)
        mock_save_nxs.assert_called_once_with(InputWorkspace=self.peaks.name(), Filename=fpath + ".nxs")
        mock_save_ref.assert_called_once_with(InputWorkspace=self.peaks.name(), Filename=fpath + ".int", Format=fmt, SplitFiles=False)

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

    #  --- methods specific to SXD class ---

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

        self.assertAlmostEqual(0.020582, self.sxd.get_radius(peaks.getPeak(0), self.ws, ispec, scale=1), delta=1e-6)

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

    # --- helper funcs ---

    def _make_peaks_detids(self, detids=[6113], tofs=None, wsname="peaks_detids"):
        if tofs is None:
            tofs = len(detids) * [10000]
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=self.ws, NumberOfPeaks=0, OutputWorkspace=wsname)
        for idet, detid in enumerate(detids):
            AddPeak(PeaksWorkspace=peaks, RunWorkspace=self.ws, DetectorID=detid, TOF=tofs[idet], EnableLogging=False)
        return peaks


def make_mock_ws_with_name(name):
    mock_ws = MagicMock()
    mock_ws.name.return_value = name
    return mock_ws


if __name__ == "__main__":
    unittest.main()

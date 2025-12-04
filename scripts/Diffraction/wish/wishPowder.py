import mantid.simpleapi as mantid
from mantid.kernel import logger
from mantid.api import AnalysisDataService as ADS
from mantid.dataobjects import EventWorkspace
from itertools import cycle
import numpy as np
from enum import Enum
from os import path


class SAMPENV(Enum):
    STICK = "candlestick"
    CRYO = "WISHcryo"


class FRAME(Enum):
    SF = 110000  # max. TOF in raw data as integer
    DF = 206000


WAVELENGTH_MIN = 0.7
WAVELENGTH_CROPPING = {FRAME.SF: {"XMin": WAVELENGTH_MIN, "XMax": 10.35}, FRAME.DF: {"XMin": WAVELENGTH_MIN, "XMax": 20.0}}
TOF_MIN = 6000
TOF_CROPPING = {FRAME.SF: {"XMin": TOF_MIN, "XMax": 99900}, FRAME.DF: {"XMin": TOF_MIN, "XMax": 199900}}
DF_PROMPTPULSE = {"XMin": TOF_CROPPING[FRAME.DF]["XMax"], "XMax": 106000}
DSPAC_FOC_CROPPING = {
    "XMins": np.array([0.8, 0.5, 0.5, 0.4, 0.35]),
    "XMaxs": np.array([53.3, 13.1, 7.77, 5.86, 4.99]),
}  # for SF (multiply max x2 for DF)


class WishPowder:
    def __init__(
        self,
        calib_dir,
        user_dir,
        sample_env=SAMPENV.STICK,
        per_detector_van=False,
        van_runno=None,
        empty_runno=None,
        default_ext="raw",
        sum_equivalent_banks=False,
        subtract_empty_from_sample=False,
    ):
        self.calib_dir = calib_dir  # includes cycle
        self.per_detector_van = per_detector_van
        self.default_ext = default_ext
        self.sum_equivalent_banks = sum_equivalent_banks
        self.sample_env = sample_env
        self.user_dir = user_dir
        self.van_runno = van_runno
        self.empty_runno = empty_runno
        self.subtract_empty_from_sample = subtract_empty_from_sample
        self.grp_ws_fpath = path.join(self.calib_dir, "WISH_banks.xml")
        self.van_sample = {
            "Material": {"ChemicalFormula": "V", "AttenuationXSection": 4.8756, "ScatteringXSection": 5.16, "SampleNumberDensity": 0.07118},
            "Geometry": {"Shape": "Cylinder", "Height": 6.0, "Radius": 0.15, "Center": [0.0, 0.0, 0.0]},
        }
        self.sample = {"Geometry": {"Shape": "Cylinder", "Center": [0.0, 0.0, 0.0]}}
        self.cylinder_abs_kwargs = {"NumberOfSlices": 10, "NumberOfAnnuli": 10, "NumberOfWavelengthPoints": 25, "ExpMethod": "Normal"}
        self.van_ws = None
        self.empty_ws = None
        self.grp_ws = None
        self.frame = FRAME.SF
        self.van_smooth_npts = 40

    def set_sample_property(self, fieldname, **kwargs):
        if fieldname not in self.sample:
            self.sample[fieldname] = kwargs
        else:
            self.sample[fieldname].update(kwargs)

    def _get_van_filepath(self, *args, **kwargs):
        return self._get_filepath_in_calib_dir(self._get_van_filename(), *args, **kwargs)

    def _get_van_filename(self):
        wsname_van = self._get_van_wsname()
        return f"{wsname_van}.nxs"

    def _get_van_wsname(self):
        suffix = "" if self.per_detector_van else "_foc"
        return f"vana_{self.sample_env.value}_{self.frame.name}{suffix}"

    def _get_mask_filepath(self):
        return self._get_filepath_in_calib_dir("mask.xml")

    def _get_cal_filepath(self):
        return self._get_filepath_in_calib_dir("calibration.cal")

    def _get_filepath_in_calib_dir(self, filename, check_exists=True):
        fpath = path.join(self.calib_dir, filename)
        if not check_exists or path.isfile(fpath):
            return fpath
        else:
            return None

    @staticmethod
    def _get_frame(ws):
        # get from chopper 1 frequency (10 for DF, 20 for SF)
        freq = WishPowder.retrieve(ws).run().getPropertyAsSingleValueWithTimeAveragedMean("fr_chop1_tgt")
        if abs(freq - 10) < 1:
            return FRAME.DF
        return FRAME.SF

    def load(self, runno, ext=None, out_wsname=None):
        if ext is None:
            ext = self.default_ext
        # what about multiple runs being summed
        fname = f"WISH{runno:08d}"
        wsname = fname if out_wsname is None else out_wsname
        ws = mantid.Load(Filename=f"{fname}.{ext}", OutputWorkspace=wsname)
        self.frame = self._get_frame(ws)
        if isinstance(ws, EventWorkspace):
            mantid.Rebin(InputWorkspace=wsname, OutputWorkspace=wsname, Params="-0.00063")
            mantid.ConvertToMatrixWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname)
        # apply calibration
        mantid.ApplyDiffCal(InstrumentWorkspace=wsname, CalibrationFile=self._get_cal_filepath())
        # mask
        self.apply_mask_to_ws(wsname)
        # crop data
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, **TOF_CROPPING[self.frame])
        if self.frame == FRAME.DF:
            mantid.MaskBins(InputWorkspace=wsname, OutputWorkspace=wsname, **DF_PROMPTPULSE)
        # crop in wavelength
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength", Emode="Elastic")
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, **WAVELENGTH_CROPPING[self.frame])
        # normalise by monitor
        self.normalise_by_monitor(wsname)
        return wsname

    def normalise_by_monitor(self, wsname):
        wsname_mon = self._process_monitor(wsname)
        mantid.NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspace=wsname_mon)
        mantid.NormaliseToMonitor(
            InputWorkspace=wsname,
            OutputWorkspace=wsname,
            MonitorWorkspace=wsname_mon,
            IntegrationRangeMin=WAVELENGTH_CROPPING[self.frame]["XMin"],
            IntegrationRangeMax=WAVELENGTH_CROPPING[self.frame]["XMax"],
        )
        mantid.ReplaceSpecialValues(
            InputWorkspace=wsname, OutputWorkspace=wsname, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0
        )
        mantid.DeleteWorkspace(wsname_mon)
        # remove monitors from workspace
        _, mon = mantid.ExtractMonitors(InputWorkspace=wsname, DetectorWorkspace=wsname, MonitorWorkspace="__tmp")
        mantid.DeleteWorkspace(mon)

    def correct_for_attenuation(self, wsname):
        ws_trans = mantid.CylinderAbsorption(InputWorkspace=wsname, OutputWorkspace=f"{wsname}_transmission", **self.cylinder_abs_kwargs)
        mantid.Divide(LHSWorkspace=wsname, RHSWorkspace=ws_trans, OutputWorkspace=wsname)
        mantid.DeleteWorkspace(ws_trans)

    def process_data(self, runnos, ext=None):
        for runno in runnos:
            wsname = self.load(runno, ext)
            if self.subtract_empty_from_sample:
                ws_empty = self.get_empty_ws()
                if ws_empty:
                    mantid.Minus(LHSWorkspace=wsname, RHSWorkspace=ws_empty, OutputWorkspace=wsname)
                else:
                    logger.warning("Empty workspace not subtracted as no workspace found and no empty_runno provided.")
            # correct for attenuation
            if "Material" in self.sample:
                mantid.SetSample(InputWorkspace=wsname, **self.sample())
                self.correct_for_attenuation(wsname)
            else:
                logger.warning("No sample defined - attenuation correction has been skipped")
            wsname_foc = self.focus(wsname)  # includes vanadium correction
            # crop in d
            self._crop_focussed_in_dspac(wsname_foc)
            # convert to TOF
            mantid.ConvertUnits(InputWorkspace=wsname_foc, OutputWorkspace=wsname_foc, Target="TOF")
            # save
            self.save_focussed_data(wsname_foc)
            if self.sum_equivalent_banks:
                wsname_foc_summed = self._combine_focussed_banks(wsname_foc)
                self.save_focussed_data(wsname_foc_summed)
            mantid.DeleteWorkspace(wsname)

    def save_focussed_data(self, wsname):
        save_fpath = path.join(self.user_dir, wsname)
        mantid.SaveNexus(InputWorkspace=wsname, Filename=save_fpath + ".nxs")
        mantid.SaveGSS(InputWorkspace=wsname, Filename=save_fpath + ".gss", Append=False, SplitFiles=False)
        mantid.SaveFocusedXYE(InputWorkspace=wsname, Filename=save_fpath + ".dat", Append=False, SplitFiles=False)

    def apply_mask_to_ws(self, wsname):
        if self._get_mask_filepath() is None:
            self.mask_tube_edges(wsname)  # apply default mask to tube ends
        else:
            self.mask_ws = self._get_mask_ws(wsname)
            mantid.MaskDetectors(wsname, MaskedWorkspace=self.mask_ws)

    @staticmethod
    def mask_tube_edges(ws, nedge=16):
        mantid.MaskBTP(Workspace=ws, Pixel=f"1-{nedge},{512 - nedge}-512")

    def _get_mask_ws(self, wsname):
        if self.mask_ws is None:
            self.mask_ws = mantid.LoadMask(
                Instrument="WISH", InputFile=self._get_mask_filepath(), ReferenceWorkspace=wsname, OutputWorkspace=wsname + "_mask"
            )
        return self.mask_ws

    def get_group_ws(self, ws):
        if self.grp_ws is None:
            self.grp_ws = mantid.LoadDetectorsGroupingFile(InputFile=self.grp_ws_fpath, InputWorkspace=ws, OutputWorkspace="wish_grp_ws")
        return self.grp_ws

    def get_empty_ws(self):
        if not self.empty_ws:
            if self.empty_runno is None:
                return None
            self.empty_ws = self.create_empty()  # need empty_runno
        return self.empty_ws

    def get_van_ws(self):
        if self.van_ws:
            return self.van_ws

        van_filepath = self._get_van_filepath()
        if van_filepath is not None:
            self.van_ws = mantid.Load(van_filepath, OutputWorkspace=self._get_van_wsname())
        elif self.van_runno:
            self.van_ws = self.create_vanadium()
        else:
            raise ValueError("No vanadium runno or saved workspace provided.")
        return self.van_ws

    def focus(self, wsname):
        van_ws = self.get_van_ws()
        # correct by vanadium
        if self.per_detector_van:
            if ADS.retrieve(wsname).getAxis(0).getUnit().unitID() != "Wavelength":
                mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength")
            mantid.Divide(LHSWorkspace=wsname, RHSWorkspace=van_ws, OutputWorkspace=wsname)
            mantid.ReplaceSpecialValues(
                InputWorkspace=wsname, OutputWorkspace=wsname, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0
            )
            self.retrieve(wsname).setDistribution(False)  # for compatibility with DiffractionFocussing
        wsname_foc = self._focus_ws(wsname)
        if not self.per_detector_van:
            # divide by focussed vanadium (in d-spacing)
            mantid.Divide(LHSWorkspace=wsname_foc, RHSWorkspace=van_ws, OutputWorkspace=wsname)
            mantid.ReplaceSpecialValues(
                InputWorkspace=wsname_foc, OutputWorkspace=wsname_foc, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0
            )
        return wsname_foc

    def _focus_ws(self, wsname):
        grp_ws = self.get_group_ws(wsname)
        if ADS.retrieve(wsname).getAxis(0).getUnit().unitID() != "dSpacing":
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="dSpacing")
        wsname_foc = f"{wsname}_foc"
        mantid.DiffractionFocussing(InputWorkspace=wsname, OutputWorkspace=wsname_foc, GroupingWorkspace=grp_ws)
        mantid.ApplyDiffCal(InstrumentWorkspace=wsname_foc, ClearCalibration=True)  # to preserve existing behaviour
        return wsname_foc

    def create_empty(self):
        return self.load(self.empty_runno)

    def create_vanadium(self):
        wsname_van = self.load(self.van_runno, out_wsname=self._get_van_wsname())
        ws_empty = self.get_empty_ws()
        if ws_empty:
            mantid.Minus(LHSWorkspace=wsname_van, RHSWorkspace=ws_empty, OutputWorkspace=wsname_van)
        else:
            logger.warning("Empty workspace not subtracted from vanadium as no empty_runno provided.")
        # correct for attenuation
        mantid.SetSample(InputWorkspace=wsname_van, **self.van_sample)
        self.correct_for_attenuation(wsname_van)
        if self.per_detector_van:
            self._replace_data_with_wavelength_focussed(wsname_van)
            mantid.SaveNexus(InputWorkspace=wsname_van, Filename=self._get_van_filepath(check_exists=False))
        else:
            # focus
            wsname_van_foc = self._focus_ws(wsname_van)
            mantid.SmoothData(InputWorkspace=wsname_van_foc, OutputWorkspace=wsname_van_foc, NPoints=self.van_smooth_npts)
            # mantid.StripVanadiumPeaks ?
            # save (in d-spacing)
            mantid.SaveNexus(InputWorkspace=wsname_van_foc, Filename=self._get_van_filepath(check_exists=False))
            mantid.DeleteWorkspace(wsname_van)
            mantid.RenameWorkspace(InputWorkspace=wsname_van_foc, OutputWorkspace=wsname_van)
        self.van_ws = wsname_van
        return self.van_ws

    @staticmethod
    def _replace_data_with_wavelength_focussed(wsname, ngroups=10):
        ws = ADS.retrieve(wsname)
        si = ws.spectrumInfo()
        tth = np.array([si.twoTheta(ii) for ii in range(ws.getNumberHistograms())])
        tth_edges = np.linspace(tth.min(), tth.max() + 1, ngroups + 1)  # tth bin edges
        for igroup in range(ngroups):
            ispecs = np.flatnonzero(np.logical_and(tth >= tth_edges[igroup], tth < tth_edges[igroup + 1]))
            ws_specs = mantid.ExtractSpectra(
                InputWorkspace=wsname, OutputWorkspace=wsname + "_specs", WorkspaceIndexList=ispecs, EnableLogging=False
            )
            # rebin spectra in group to match so can be summed
            [ws_specs.applyBinEdgesFromAnotherWorkspace(ws_specs, 0, ispec) for ispec in range(1, ws_specs.getNumberHistograms())]
            ws_specs = mantid.SumSpectra(InputWorkspace=ws_specs, OutputWorkspace=ws_specs.name(), EnableLogging=False)
            for ispec in ispecs:
                ispec = int(ispec)
                ws_specs.applyBinEdgesFromAnotherWorkspace(ws, ispec, 0)
                scale = ws.readY(ispec).sum() / ws_specs.readY(0).sum()  # assume no error on this
                ws.setY(ispec, ws_specs.readY(0) * scale)
                ws.setE(ispec, ws_specs.readE(0) * scale)
        mantid.DeleteWorkspace(ws_specs)

    @staticmethod
    def _process_monitor(wsname, nbreaks=70, npts_smooth=40):
        wsname_mon = wsname + "_mon"
        mantid.ExtractSingleSpectrum(InputWorkspace=wsname, OutputWorkspace=wsname_mon, WorkspaceIndex=3)
        mantid.ConvertToDistribution(wsname_mon)
        # mask bragg edges
        for xmin, xmax in [(4.57, 4.76), (3.87, 4.12), (2.75, 2.91), (2.24, 2.50)]:
            mantid.MaskBins(InputWorkspace=wsname_mon, OutputWorkspace=wsname_mon, XMin=xmin, XMax=xmax)
        # fit background
        mantid.SplineBackground(InputWorkspace=wsname_mon, OutputWorkspace=wsname_mon, WorkspaceIndex=0, NCoeff=nbreaks)
        mantid.SmoothData(InputWorkspace=wsname_mon, OutputWorkspace=wsname_mon, NPoints=npts_smooth)
        mantid.ConvertFromDistribution(wsname_mon)
        return wsname_mon

    def _crop_focussed_in_dspac(self, ws_foc):
        ws_foc = WishPowder.retrieve(ws_foc)
        if ws_foc.getAxis(0).getUnit().unitID() != "dSpacing":
            ws_foc = mantid.ConvertUnits(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), Target="dSpacing")
        xmins = cycle(DSPAC_FOC_CROPPING["XMins"])
        xmaxs = DSPAC_FOC_CROPPING["XMaxs"]
        if self.frame == FRAME.DF:
            xmaxs = 2 * xmaxs
        xmaxs = cycle(xmaxs)
        for ispec in range(ws_foc.getNumberHistograms()):
            ws_foc = mantid.MaskBins(
                InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), XMin=0, XMax=next(xmins), InputWorkspaceIndexSet=ispec
            )
            ws_foc = mantid.MaskBins(
                InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), XMin=next(xmaxs), XMax=100, InputWorkspaceIndexSet=ispec
            )

    @staticmethod
    def _combine_focussed_banks(wsname_foc):
        ws_foc_1to5 = mantid.ExtractSpectra(InputWorkspace=wsname_foc, OutputWorkspace=f"{wsname_foc}_1to5", WorkspaceIndexList=range(0, 5))
        ws_foc_6to10 = mantid.ExtractSpectra(
            InputWorkspace=wsname_foc, OutputWorkspace=f"{wsname_foc}_6to10", WorkspaceIndexList=range(5, 10)
        )
        # ensure bin edges in equivalent banks are the same (could be different due to calibration)
        ws_foc_6to10 = mantid.RebinToWorkspace(
            WorkspaceToRebin=ws_foc_6to10, WorkspaceToMatch=ws_foc_1to5, OutputWorkspace=ws_foc_6to10.name()
        )
        ws_foc_summed = f"{wsname_foc}_summed"
        mantid.Plus(LHSWorkspace=ws_foc_1to5, RHSWorkspace=ws_foc_6to10, OutputWorkspace=ws_foc_summed)
        mantid.DeleteWorkspaces([ws_foc_1to5, ws_foc_6to10])
        return ws_foc_summed

    @staticmethod
    def retrieve(ws):
        if isinstance(ws, str):
            return ADS.retrieve(ws)
        else:
            return ws

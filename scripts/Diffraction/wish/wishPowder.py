# Set of routines to normalise WISH data- new look Mantid with mantidsimple removed
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
        subtract_empty=False,
    ):
        self.calib_dir = calib_dir  # includes cycle
        self.per_detector_van = per_detector_van
        self.default_ext = default_ext
        self.sum_equivalent_banks = sum_equivalent_banks
        self.sample_env = sample_env
        self.user_dir = user_dir
        self.van_runno = van_runno
        self.empty_runno = empty_runno
        self.subtract_empty = subtract_empty
        self.grp_ws_fpath = path.join(self.calib_dir, "WISH_banks.xml")
        self.van_sample = {
            "Material": {"ChemicalFormula": "V", "AttenuationXSection": 4.8756, "ScatteringXSection": 5.16, "SampleNumberDensity": 0.07118},
            "Geometry": {"Shape": "Cylinder", "Height": 6.0, "Radius": 0.15, "Center": [0.0, 0.0, 0.0]},
        }
        self.sample = {"Geometry": {"Shape": "Cylinder", "Center": [0.0, 0.0, 0.0]}}
        self.cylinder_abs_kwargs = {"NumberOfSlices": 10, "NumberOfAnnuli": 10, "NumberOfWavelengthPoints": 25, "ExpMethod": "Normal"}
        self.van_smooth_npts = 40  # use same for empty - really

    def set_sample_property(self, fieldname, **kwargs):
        if fieldname not in self.sample:
            self.sample[fieldname] = kwargs
        else:
            self.sample[fieldname].update(kwargs)

    def _get_van_filepath(self, *args, **kwargs):
        return self._get_filepath_in_calib_dir(self._get_van_filename(), *args, **kwargs)

    def _get_van_filename(self):
        suffix = "" if self.per_detector_van else "_foc"
        return f"vana_{self.sample_env.value}{suffix}.nxs"

    def _get_mask_filepath(self):
        return self._get_filepath_in_calib_dir("mask.xml")

    def _get_cal_filepath(self):
        return self._get_filepath_in_calib_dir("calibration.xml")

    def _get_filepath_in_calib_dir(self, filename, check_exists=True):
        fpath = path.join(self.calib_dir, filename)
        if not check_exists or path.isfile(fpath):
            return fpath
        else:
            return None

    def load(self, runno, ext=None):
        if ext is None:
            ext = self.default_ext
        # what about multiple runs being summed
        wsname = f"WISH{runno:08d}"
        ws = mantid.Load(Filename=f"{wsname}.{ext}", OutputWorkspace=wsname, LoadMonitors=True)
        if isinstance(ws, EventWorkspace):
            mantid.Rebin(InputWorkspace=wsname, OutputWorkspace=wsname, Params="6000,-0.00063,110000")
            mantid.ConvertToMatrixWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname)
        # apply calibration
        mantid.ApplyDiffCal(InstrumentWorkspace=wsname, CalibrationFile=self._get_cal_filepath())
        # mask
        self.apply_mask_to_ws(wsname)
        # crop prompt pulse in TOF single-frame
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=6000, XMax=99000)
        # crop in wavelength
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength", Emode="Elastic")
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=0.7, XMax=10.35)
        # normalise by monitor
        self.normalise_by_monitor(wsname)
        return wsname

    def normalise_by_monitor(self, wsname):
        wsname_mon = self._proccess_monitor(wsname)
        mantid.NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspace=wsname_mon)
        mantid.NormaliseToMonitor(
            InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspace=wsname_mon, IntegrationRangeMin=0.7, IntegrationRangeMax=10.35
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
            if self.subtract_empty:
                mantid.Minus(LHSWorkspace=wsname, RHSWorkspace=self.get_empty_ws(), OutputWorkspace=wsname)
            # correct for attenuation
            if "Material" in self.sample:
                mantid.SetSample(InputWorkspace=wsname, **self.sample())
                self.correct_for_attenuation(wsname)
            else:
                logger.warning("No sample defined - attenuation correction has been skipped")
            wsname_foc = self.focus(wsname)  # includes vanadium correction
            # crop in d
            self._crop_focussed_in_dspac(wsname_foc)
            if self.sum_equivalent_banks:
                self._combine_focussed_banks(wsname_foc)
            # convert to TOF
            mantid.ConvertUnits(InputWorkspace=wsname_foc, OutputWorkspace=wsname_foc, Target="TOF")
            # save
            self.save_focussed_data(wsname_foc)
            mantid.DeleteWorkspace(wsname)

    def save_focussed_data(self, wsname):
        save_fpath = path.join(self.user_dir, wsname)
        mantid.SaveNexus(InputWorkspace=wsname, Filename=save_fpath + ".nxs")
        mantid.SaveGSS(InputWorkspace=wsname, Filename=save_fpath + ".gss")
        mantid.SaveFocusedXYE(InputWorkspace=wsname, Filename=save_fpath + ".dat")

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
            self.empty_ws = self.create_empty()  # need empty_runno
        return self.empty_ws

    def get_van_ws(self):
        if self.van_ws:
            return self.van_ws
        van_filepath = self._get_van_filepath()
        if van_filepath is not None:
            self.van_ws = mantid.Load(van_filepath)
        elif self.van_runno:
            self.van_ws = self.create_vanadium()
        else:
            raise ValueError("No vanadium runno or saved workspace provided.")
        return self.van_ws

    def focus(self, wsname):
        van_ws = self.get_van_ws()
        if ADS.retrieve(wsname).getAxis(0).getUnit().unitID() != "Wavelength":
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength")
        # correct by vanadium
        if self.per_detector_van:
            mantid.Divide(LHSWorkspace=wsname, RHSWorkspace=van_ws, OutputWorkspace=wsname)
            mantid.ReplaceSpecialValues(
                InputWorkspace=wsname, OutputWorkspace=wsname, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0
            )
        wsname_foc = self._focus_ws(wsname)
        if not self.per_detector:
            # divide by focussed vanadium (in d-spacing)
            mantid.Divide(LHSWorkspace=wsname_foc, RHSWorkspace=van_ws, OutputWorkspace=wsname)
            mantid.ReplaceSpecialValues(
                InputWorkspace=wsname_foc, OutputWorkspace=wsname_foc, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0
            )

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
        wsname_van = self.load(self.van_runno)
        # subtract empty
        mantid.Minus(LHSWorkspace=wsname_van, RHSWorkspace=self.get_empty_ws(), OutputWorkspace=wsname_van)
        # correct for attenuation
        mantid.SetSample(**self.van_sample)
        self.correct_for_attenuation(self, wsname_van)
        if self.per_detector_van:
            self._replace_data_with_wavelength_focussed(wsname_van)
            mantid.SaveNexus(InputWorkspace=wsname_van, Filename=self._get_van_filepath(check_exists=False))
            return wsname_van
        else:
            # focus
            wsname_van_foc = self._focus_ws(wsname_van)
            # mantid.SmoothData(InputWorkspace=wsname_van_foc, OutputWorkspace=wsname_van_foc, NPoints=self.van_smooth_npts)
            # mantid.StripVanadiumPeaks ?
            # save (in d-spacing)
            mantid.SaveNexus(InputWorkspace=wsname_van_foc, Filename=self._get_van_filepath(check_exists=False))
            mantid.DeleteWorkspace(wsname_van)
            return wsname_van_foc

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

    @staticmethod
    def _crop_focussed_in_dspac(ws_foc):
        xmins = cycle([0.8, 0.5, 0.5, 0.4, 0.35])
        xmaxs = cycle([53.3, 13.1, 7.77, 5.86, 4.99])
        for ispec in range(WishPowder.retreive(ws_foc).getNumberHistograms()):
            ws_foc = mantid.MaskBins(
                InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), XMin=0, XMax=next(xmins), InputWorkspaceIndexSet=ispec
            )
            ws_foc = mantid.MaskBins(
                InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(), XMin=next(xmaxs), XMax=100, InputWorkspaceIndexSet=ispec
            )

    @staticmethod
    def _combine_focussed_banks(ws_foc):
        ws_foc_1to5 = mantid.ExtractSpectra(InputWorkspace=ws_foc, OutputWorkspace=f"{ws_foc.name()}_1to5", WorkspaceIndexList=range(0, 5))
        ws_foc_6to10 = mantid.ExtractSpectra(
            InputWorkspace=ws_foc, OutputWorkspace=f"{ws_foc.name()}_6to10", WorkspaceIndexList=range(5, 10)
        )
        # ensure bin edges in equivalent banks are the same (could be different due to calibration)
        ws_foc_6to10 = mantid.RebinToWorkspace(
            WorkspaceToRebin=ws_foc_6to10, WorkspaceToMatch=ws_foc_1to5, OutputWorkspace=ws_foc_6to10.name()
        )
        mantid.Plus(LHSWorkspace=ws_foc_1to5, RHSWorkspace=ws_foc_6to10, OutputWorkspace=ws_foc.name())

    @staticmethod
    def retrieve(ws):
        if isinstance(ws, str):
            return ADS.retrieve(ws)
        else:
            return ws

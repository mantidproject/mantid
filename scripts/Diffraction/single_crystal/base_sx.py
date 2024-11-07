# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence, Optional
import numpy as np
from enum import Enum
import mantid.simpleapi as mantid
from mantid.api import FunctionFactory, AnalysisDataService as ADS, IMDEventWorkspace, IPeaksWorkspace, IPeak
from mantid.kernel import logger, SpecialCoordinateSystem
from plugins.algorithms.FindGoniometerFromUB import getSignMaxAbsValInCol
from mantid.geometry import CrystalStructure, SpaceGroupFactory, ReflectionGenerator, ReflectionConditionFilter, PeakShape
from os import path
from json import loads as json_loads
from matplotlib.pyplot import subplots, close
from matplotlib.patches import Circle
from matplotlib.colors import LogNorm
from matplotlib.backends.backend_pdf import PdfPages
from abc import ABC, abstractmethod
from scipy.spatial.transform import Rotation
from scipy.optimize import minimize
from functools import reduce


class PEAK_TYPE(Enum):
    FOUND = "found"
    PREDICT = "predicted"
    PREDICT_SAT = "predicted_sat"


class INTEGRATION_TYPE(Enum):
    MD = "int_MD"
    MD_OPTIMAL_RADIUS = "int_MD_opt"
    SKEW = "int_skew"
    SHOEBOX = "int_shoebox"
    PROFILE = "int_profile"


class BaseSX(ABC):
    def __init__(self, vanadium_runno: Optional[str] = None, file_ext: str = ".raw", scale_integrated: bool = False):
        self.runs = dict()
        self.van_runno = vanadium_runno
        self.van_ws = None
        self.gonio_axes = None
        self.sample_dict = None
        self.n_mcevents = 1200
        self.file_ext = file_ext  # file extension
        self.scale_integrated = scale_integrated

    # --- decorator to apply to all runs is run=None ---
    def default_apply_to_all_runs(func):
        def apply(self, *args, **kwargs):
            if "run" in kwargs and kwargs["run"] is not None:
                func(self, *args, **kwargs)
            else:
                for run in self.runs.keys():
                    func(self, *args, **kwargs, run=run)

        return apply

    # --- getters ---

    def get_ws(self, run):
        try:
            return BaseSX.retrieve(self.runs[str(run)]["ws"])
        except:
            return None

    def get_ws_name(self, run):
        ws = self.get_ws(run)
        if ws is not None:
            return ws.name()

    def get_md(self, run):
        try:
            return BaseSX.retrieve(self.runs[str(run)]["MD"])
        except:
            return None

    def get_md_name(self, run):
        ws = self.get_md(run)
        if ws is not None:
            return ws.name()

    def get_peaks(self, run, peak_type, integration_type=None):
        if integration_type is None:
            fieldname = peak_type.value
        else:
            fieldname = "_".join([peak_type.value, integration_type.value])
        try:
            return BaseSX.retrieve(self.runs[str(run)][fieldname])
        except:
            return None

    def get_peaks_name(self, run, peak_type, integration_type=None):
        ws = self.get_peaks(run, peak_type, integration_type)
        if ws is not None:
            return ws.name()

    @staticmethod
    def has_sample(ws):
        return BaseSX.retrieve(ws).sample().getMaterial().numberDensity > 1e-15

    # --- setters ---

    def set_van_ws(self, van_ws):
        self.van_ws = BaseSX.retrieve(van_ws).name()

    def set_ws(self, run, ws):
        run_str = str(run)
        if run_str not in self.runs:
            self.runs[run_str] = {"ws": BaseSX.retrieve(ws).name()}
        else:
            self.runs[run_str]["ws"] = BaseSX.retrieve(ws).name()

    def set_peaks(self, run, peaks, peak_type=PEAK_TYPE.FOUND, integration_type=None):
        if integration_type is None:
            fieldname = peak_type.value
        else:
            fieldname = "_".join([peak_type.value, integration_type.value])
        self.runs[str(run)][fieldname] = BaseSX.retrieve(peaks).name()

    def set_md(self, run, ws_md):
        self.runs[str(run)]["MD"] = BaseSX.retrieve(ws_md).name()

    def set_sample(self, **kwargs):
        default_material = {"NumberDensityUnit": "Formula Units"}
        if "Material" in kwargs:
            # if no material in kwargs then setting this would produce an error in SetSample later
            kwargs["Material"] = {**default_material, **kwargs["Material"]}
        self.sample_dict = kwargs

    def set_goniometer_axes(self, *args):
        """
        :param axes: collection of axes - e.g. ([x,y,z,hand], [x,y,z,hand]) - in same order to be passed to SetSample
        """
        # convert to strings
        self.gonio_axes = [",".join([str(elem) for elem in axis]) for axis in args]

    def set_mc_abs_nevents(self, nevents):
        self.n_mcevents = nevents

    # --- abstract methods ---

    @abstractmethod
    def process_data(self, runs: Sequence[str], *args):
        raise NotImplementedError()

    @abstractmethod
    def load_run(self, runno):
        raise NotImplementedError()

    @abstractmethod
    def process_vanadium(self):
        raise NotImplementedError()

    # --- methods ---

    def delete_run_data(self, run, del_MD=False):
        run = str(run)
        fields = ["ws", "MD"] if del_MD else ["ws"]
        for field in fields:
            if self.runs[run][field]:
                mantid.DeleteWorkspace(self.runs[run][field])
                self.runs[run][field] = None

    def _set_goniometer_on_ws(self, ws, angles):
        """
        :param ws: workspace or name
        :param angles: list of angles or motor names
        :return:
        """
        if self.gonio_axes is not None:
            axis_dict = {f"Axis{iax}": ",".join([str(angles[iax]), self.gonio_axes[iax]]) for iax in range(len(angles))}
            mantid.SetGoniometer(Workspace=ws, EnableLogging=False, **axis_dict)

    @staticmethod
    def convert_ws_to_MD(ws, md_name=None, frame="Q (lab frame)"):
        if md_name is None:
            md_name = BaseSX.retrieve(ws).name() + "_MD"
        xunit = BaseSX.get_xunit(ws)  # get initial xunit
        BaseSX._normalise_by_bin_width_in_k(ws)  # normalise by bin-width in K = 2pi/lambda
        wsMD = mantid.ConvertToDiffractionMDWorkspace(
            InputWorkspace=ws,
            OutputWorkspace=md_name,
            LorentzCorrection=True,
            OneEventPerBin=False,
            OutputDimensions=frame,
            EnableLogging=False,
        )
        BaseSX._normalise_by_bin_width_in_k(ws, undo=True)  # normalise by bin-width in K = 2pi/lambda
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target=xunit, EnableLogging=False)
        mantid.DeleteWorkspace("PreprocessedDetectorsWS")
        return wsMD

    @default_apply_to_all_runs
    def convert_to_MD(self, run=None, frame="Q (lab frame)"):
        wsname = self.get_ws_name(run)
        md_name = wsname + "_MD"
        if frame == "HKL" and not BaseSX.retrieve(wsname).sample().hasOrientedLattice():
            peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
            if peaks is not None and peaks.sample().hasOrientedLattice():
                mantid.SetUB(wsname, UB=peaks.sample().getOrientedLattice().getUB())
            else:
                logger.error(f"No UB specified for {wsname} or found peaks - cannot transform to HKL.")
                return
        BaseSX.convert_ws_to_MD(wsname, md_name, frame)
        self.set_md(run, md_name)

    @default_apply_to_all_runs
    def load_isaw_ub(self, isaw_file: str, run=None, tol=0.15):
        if not path.exists(isaw_file):
            logger.error(f"Invalid path {isaw_file} to ISAW UB file")
            return
        ws = self.get_ws_name(run)
        try:
            mantid.LoadIsawUB(InputWorkspace=ws, Filename=isaw_file)
            peaks = self.get_peaks_name(run, PEAK_TYPE.FOUND)
            if peaks is not None:
                mantid.LoadIsawUB(InputWorkspace=peaks, Filename=isaw_file)
                mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)
        except:
            logger.error(
                f"LoadIsawUB failed for run {run} - check file contains valid ISAW format UB and the U matrix is a proper rotation matrix."
            )

    def find_ub_using_lattice_params(self, global_B, tol=0.15, **kwargs):
        if global_B:
            peaks = [self.get_peaks_name(run, PEAK_TYPE.FOUND) for run in self.runs.keys()]
            mantid.FindGlobalBMatrix(PeakWorkspaces=peaks, Tolerance=tol, **kwargs)
        else:
            for run in self.runs.keys():
                peaks = self.get_peaks_name(run, PEAK_TYPE.FOUND)
                mantid.FindUBUsingLatticeParameters(PeaksWorkspace=peaks, Tolerance=tol, **kwargs)
                mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)

    @default_apply_to_all_runs
    def calculate_U_matrix(self, run=None, **kwargs):
        mantid.CalculateUMatrix(PeaksWorkspace=self.get_peaks_name(run, PEAK_TYPE.FOUND), **kwargs)

    @default_apply_to_all_runs
    def calibrate_sample_pos(self, tol=0.15, run=None):
        peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
        mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)
        mantid.OptimizeCrystalPlacement(
            PeaksWorkspace=peaks,
            ModifiedPeaksWorkspace=peaks,
            FitInfoTable=run + "_sample_pos_fit_info",
            AdjustSampleOffsets=True,
            OptimizeGoniometerTilt=True,
            MaxSamplePositionChangeMeters=0.01,
            MaxIndexingError=tol,
        )
        mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)

    @default_apply_to_all_runs
    def predict_peaks(self, peak_type=PEAK_TYPE.PREDICT, run=None, **kwargs):
        input_ws = None
        ws = self.get_ws(run)
        peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
        if peaks is not None and peaks.sample().hasOrientedLattice():
            input_ws = peaks
        elif ws is not None and ws.sample().hasOrientedLattice():
            input_ws = ws
        if input_ws is not None:
            out_peaks_name = "_".join([input_ws.name(), peak_type.value])
            if peak_type == PEAK_TYPE.PREDICT:
                mantid.PredictPeaks(InputWorkspace=input_ws, OutputWorkspace=out_peaks_name, EnableLogging=False, **kwargs)
            else:
                pred_peaks = self.get_peaks(run, PEAK_TYPE.PREDICT)
                if pred_peaks is not None and pred_peaks.sample().hasOrientedLattice():
                    mantid.PredictFractionalPeaks(Peaks=pred_peaks, FracPeaks=out_peaks_name, EnableLogging=False, **kwargs)
            self.set_peaks(run, out_peaks_name, peak_type)

    @default_apply_to_all_runs
    def calc_absorption_weighted_path_lengths(self, peak_type, int_type=None, run=None, **kwargs):
        """
        Method to calculate tbar for each peak (saved in a column of the table) and optionally apply an attenuation
        correction to the integrated intensity of each peak. By default the correction will be applied if
        self.scale_integrated = True - which is the case for SXD but not WISH, but can be overridden by supplying
        keyword ApplyCorrection.
        :param peak_type: PEAK_TYPE Enum to identify peak table to use
        :param int_type: INTEGRATION_TYPE Enum to identify peak table to use
        :param run: run number to identify peak table to use (if not supplied default will be to apply to all runs)
        :param kwargs: keyword arguments passed to AddAbsorptionWeightedPathLengths algorithm
        """
        default_kwargs = {"ApplyCorrection": self.scale_integrated, "EventsPerPoint": 1500, "MaxScatterPtAttempts": 7500}
        kwargs = {**default_kwargs, **kwargs}
        ws = self.get_ws(run)
        mantid.SetBeam(ws, Geometry={"Shape": "Slit", "Width": self.beam_width, "Height": self.beam_height})
        peaks = self.get_peaks(run, peak_type, int_type)
        mantid.CopySample(InputWorkspace=ws, OutputWorkspace=peaks, CopyEnvironment=False)
        mantid.AddAbsorptionWeightedPathLengths(InputWorkspace=peaks, **kwargs)

    @staticmethod
    def get_back_to_back_exponential_func(pk, ws, ispec):
        tof = pk.getTOF()
        func = FunctionFactory.Instance().createPeakFunction("BackToBackExponential")
        func.setParameter("X0", tof)  # set centre
        func.setMatrixWorkspace(ws, ispec, 0.0, 0.0)  # calc A,B,S based on peak cen
        return func

    @staticmethod
    def convert_dTOF_to_dQ_for_peak(dtof, pk):
        tof = pk.getTOF()
        modQ = 2 * np.pi / pk.getDSpacing()
        return (modQ / tof) * dtof

    @staticmethod
    def get_radius(pk, ws, ispec, scale):
        func = BaseSX.get_back_to_back_exponential_func(pk, ws, ispec)
        dtof = scale / func.getParameterValue("B")
        return BaseSX.convert_dTOF_to_dQ_for_peak(dtof, pk)

    @staticmethod
    def integrate_peaks_MD(wsMD, peaks, out_peaks, **kwargs):
        default_kwargs = {"IntegrateIfOnEdge": False, "UseOnePercentBackgroundCorrection": False}
        kwargs = {**default_kwargs, **kwargs}
        peaks_int = mantid.IntegratePeaksMD(
            InputWorkspace=wsMD,
            PeaksWorkspace=peaks,
            OutputWorkspace=out_peaks,
            **kwargs,
        )
        return peaks_int

    def integrate_peaks_MD_optimal_radius(self, wsMD, peaks, out_peaks, dq=0.01, scale=5, ws=None, **kwargs):
        # note this is not a static method so that the static `get_radius` can be overridden
        peaks = BaseSX.retrieve(peaks)
        use_empty_inst = ws is None
        if use_empty_inst:
            ws = mantid.LoadEmptyInstrument(
                InstrumentName=peaks.getInstrument().getFullName(), OutputWorkspace="empty", EnableLogging=False
            )
            axis = ws.getAxis(0)
            axis.setUnit("TOF")
        else:
            ws = BaseSX.retrieve(ws)
            mantid.ConvertUnits(
                InputWorkspace=ws, OutputWorkspace=ws, Target="TOF", EnableLogging=False
            )  # needs to be in TOF for setting B
        ispecs = ws.getIndicesFromDetectorIDs(peaks.column("DetID"))
        rads = [self.get_radius(pk, ws, ispecs[ipk], scale) for ipk, pk in enumerate(peaks)]
        bin_edges = np.arange(min(rads), max(rads) + dq, dq)
        ibins = np.digitize(rads, bin_edges[:-1])
        peaks_int = mantid.CloneWorkspace(InputWorkspace=peaks, OutputWorkspace=out_peaks, EnableLogging=False)
        mantid.DeleteTableRows(TableWorkspace=peaks_int, Rows=list(range(peaks_int.getNumberPeaks())), EnableLogging=False)
        for ibin in np.unique(ibins):
            rad = bin_edges[ibin]
            ipks = np.where(ibins == ibin)[0]
            irows_to_del = set(range(peaks.getNumberPeaks())) - set(ipks)
            peaks_subset = mantid.CloneWorkspace(InputWorkspace=peaks, OutputWorkspace=peaks.name() + "_rad", EnableLogging=False)
            mantid.DeleteTableRows(TableWorkspace=peaks_subset, Rows=list(irows_to_del), EnableLogging=False)
            # integrate
            peaks_subset = BaseSX.integrate_peaks_MD(
                wsMD,
                peaks_subset.name(),
                peaks_subset.name(),
                PeakRadius=rad,
                BackgroundInnerRadius=rad,
                BackgroundOuterRadius=rad * (2 ** (1 / 3)),
                **kwargs,
            )
            peaks_int = mantid.CombinePeaksWorkspaces(
                LHSWorkspace=peaks_int, RHSWorkspace=peaks_subset, OutputWorkspace=peaks_int.name(), EnableLogging=False
            )
            mantid.DeleteWorkspace(peaks_subset, EnableLogging=False)
        if use_empty_inst:
            mantid.DeleteWorkspace(ws, EnableLogging=False)
        return peaks_int

    @staticmethod
    def integrate_peaks_skew(ws, peaks, out_peaks, **kwargs):
        peaks_int = mantid.IntegratePeaksSkew(InputWorkspace=ws, PeaksWorkspace=peaks, OutputWorkspace=out_peaks, **kwargs)
        return peaks_int

    @staticmethod
    def integrate_peaks_shoebox(ws, peaks, out_peaks, **kwargs):
        peaks_int = mantid.IntegratePeaksShoeboxTOF(InputWorkspace=ws, PeaksWorkspace=peaks, OutputWorkspace=out_peaks, **kwargs)
        return peaks_int

    @staticmethod
    def integrate_peaks_profile(ws, peaks, out_peaks, **kwargs):
        peaks_int = mantid.IntegratePeaks1DProfile(InputWorkspace=ws, PeaksWorkspace=peaks, OutputWorkspace=out_peaks, **kwargs)
        return peaks_int

    @default_apply_to_all_runs
    def integrate_data(self, integration_type, peak_type, tol=0, run=None, **kwargs):
        pk_table = self.get_peaks(run, peak_type)
        if peak_type == PEAK_TYPE.FOUND and tol > 0:
            mantid.IndexPeaks(PeaksWorkspace=pk_table, Tolerance=tol, RoundHKLs=True)
            BaseSX.remove_unindexed_peaks(pk_table)
        # integrate
        peak_int_name = "_".join([BaseSX.retrieve(pk_table).name(), integration_type.value])
        ws = self.get_ws(run)
        ws_md = self.get_md(run)
        if integration_type == INTEGRATION_TYPE.MD and ws_md is not None:
            BaseSX.integrate_peaks_MD(ws_md, pk_table, peak_int_name, **kwargs)
        elif integration_type == INTEGRATION_TYPE.MD_OPTIMAL_RADIUS and ws_md is not None:
            if ws is not None:
                kwargs["ws"] = ws  # use this workspace to avoid loading in empty
            self.integrate_peaks_MD_optimal_radius(ws_md, pk_table, peak_int_name, **kwargs)
        elif integration_type == INTEGRATION_TYPE.SHOEBOX:
            BaseSX.integrate_peaks_shoebox(ws, pk_table, peak_int_name, **kwargs)
        elif integration_type == INTEGRATION_TYPE.PROFILE:
            BaseSX.integrate_peaks_profile(ws, pk_table, peak_int_name, **kwargs)
        else:
            BaseSX.integrate_peaks_skew(ws, pk_table, peak_int_name, **kwargs)
        # store result
        self.set_peaks(run, peak_int_name, peak_type, integration_type)

    def save_peak_table(self, run, peak_type, integration_type, save_dir, save_format, run_ref=None, save_nxs=True, **kwargs):
        peaks = self.get_peaks_name(run, peak_type, integration_type)
        if run_ref is not None and run_ref != run:
            self.make_UB_consistent(self.get_peaks_name(run_ref, peak_type, integration_type), peaks)
        filepath = path.join(save_dir, "_".join([peaks, save_format])) + ".int"
        mantid.SaveReflections(InputWorkspace=peaks, Filename=filepath, Format=save_format, **kwargs)
        if save_nxs:
            mantid.SaveNexus(InputWorkspace=peaks, Filename=filepath[:-3] + "nxs")

    def save_all_peaks(self, peak_type, integration_type, save_dir, save_format, run_ref=None, save_nxs=True, **kwargs):
        runs = list(self.runs.keys())
        if len(runs) > 1:
            # get name for peak table from range of runs integrated
            min_ws = min(runs, key=lambda k: int("".join(filter(str.isdigit, k))))
            max_ws = max(runs, key=lambda k: int("".join(filter(str.isdigit, k))))
            all_peaks = f'{"-".join([min_ws, max_ws])}_{peak_type.value}_{integration_type.value}'
            # clone first peak table
            mantid.CloneWorkspace(
                InputWorkspace=self.get_peaks(runs[0], peak_type, integration_type), OutputWorkspace=all_peaks, EnableLogging=False
            )
            for run in runs[1:]:
                peaks = self.get_peaks(run, peak_type, integration_type)
                if run_ref is not None and run_ref != run:
                    self.make_UB_consistent(self.get_peaks(run_ref, peak_type, integration_type), peaks)
                mantid.CombinePeaksWorkspaces(LHSWorkspace=all_peaks, RHSWorkspace=peaks, OutputWorkspace=all_peaks)
            filepath = path.join(save_dir, "_".join([all_peaks, save_format])) + ".int"
            mantid.SaveReflections(InputWorkspace=all_peaks, Filename=filepath, Format=save_format, **kwargs)
            if save_nxs:
                mantid.SaveNexus(InputWorkspace=all_peaks, Filename=filepath[:-3] + "nxs")

    def _is_vanadium_processed(self):
        return self.van_ws is not None and ADS.doesExist(self.van_ws)

    @staticmethod
    def _normalise_by_bin_width_in_k(wsname, undo=False):
        ws = mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Momentum", EnableLogging=False)
        si = ws.spectrumInfo()
        for ispec in range(ws.getNumberHistograms()):
            if si.hasDetectors(ispec) and not si.isMonitor(ispec):
                # divide y and e by bin-width
                xedges = ws.readX(ispec)
                dx = np.diff(xedges)
                scale = dx if not undo else 1 / dx
                ws.setY(ispec, ws.readY(ispec) / scale)
                ws.setE(ispec, ws.readE(ispec) / scale)

    @staticmethod
    def _minus_workspaces(ws_lhs, ws_rhs):
        mantid.RebinToWorkspace(
            WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs, OutputWorkspace=ws_rhs, PreserveEvents=False, EnableLogging=False
        )
        mantid.Minus(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs, EnableLogging=False)
        mantid.ReplaceSpecialValues(
            InputWorkspace=ws_lhs,
            OutputWorkspace=ws_lhs,
            NaNValue=0,
            InfinityValue=0,
            BigNumberThreshold=1e15,
            SmallNumberThreshold=1e-15,
            EnableLogging=False,
        )

    @staticmethod
    def _divide_workspaces(ws_lhs, ws_rhs):
        mantid.RebinToWorkspace(
            WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs, OutputWorkspace=ws_rhs, PreserveEvents=False, EnableLogging=False
        )
        mantid.Divide(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs, EnableLogging=False)
        mantid.ReplaceSpecialValues(
            InputWorkspace=ws_lhs,
            OutputWorkspace=ws_lhs,
            NaNValue=0,
            InfinityValue=0,
            BigNumberThreshold=1e15,
            SmallNumberThreshold=1e-15,
            EnableLogging=False,
        )

    @staticmethod
    def make_UB_consistent(ws_ref, ws):
        # compare U matrix to perform TransformHKL to preserve indexing
        U_ref = BaseSX.retrieve(ws_ref).sample().getOrientedLattice().getU()
        U = BaseSX.retrieve(ws).sample().getOrientedLattice().getU()
        # find transform required  ( U_ref = U T^-1) - see TransformHKL docs for details
        transform = np.linalg.inv(getSignMaxAbsValInCol(np.matmul(np.linalg.inv(U), U_ref)))
        try:
            mantid.TransformHKL(PeaksWorkspace=ws, HKLTransform=transform, FindError=True, EnableLogging=False)
        except ValueError:
            # probably not enough peaks to optimize UB to find error on lattice parameters (will be set to 0)
            mantid.TransformHKL(PeaksWorkspace=ws, HKLTransform=transform, FindError=False, EnableLogging=False)

    @staticmethod
    def crop_ws(ws, xmin, xmax, xunit="Wavelength"):
        wsname = BaseSX.retrieve(ws).name()
        ws_xunit = BaseSX.get_xunit(ws)
        if not ws_xunit == xunit:
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target=xunit, EnableLogging=False)
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=xmin, XMax=xmax, EnableLogging=False)
        if not ws_xunit == xunit:
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target=ws_xunit, EnableLogging=False)

    @staticmethod
    def find_sx_peaks(ws, bg=None, nstd=None, out_peaks=None, **kwargs):
        default_kwargs = {"XResolution": 200, "PhiResolution": 2, "TwoThetaResolution": 2}
        kwargs = {**default_kwargs, **kwargs}  # will overwrite default with provided if duplicate keys
        ws = BaseSX.retrieve(ws)
        if out_peaks is None:
            out_peaks = ws.name() + "_peaks"
        if bg is None:
            ymaxs = np.max(ws.extractY(), axis=1)  # max in each spectrum
            avg = np.median(ymaxs)
            std = (np.percentile(ymaxs, 90) - avg) / 1.2815
            bg = avg + nstd * std
        elif nstd is None:
            return
        # get unit to convert back to after peaks found
        xunit = BaseSX.get_xunit(ws)
        if not xunit == "TOF":
            ws = mantid.ConvertUnits(
                InputWorkspace=ws, OutputWorkspace=ws.name(), Target="TOF", EnableLogging=False
            )  # FindSXPeaks requires TOF
        # extract y data (to use to determine to determine threshold)
        mantid.FindSXPeaks(
            InputWorkspace=ws,
            PeakFindingStrategy="AllPeaks",
            AbsoluteBackground=bg,
            ResolutionStrategy="AbsoluteResolution",
            OutputWorkspace=out_peaks,
            EnableLogging=False,
            **kwargs,
        )
        if not xunit == "TOF":
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target=xunit, EnableLogging=False)
        return out_peaks

    @staticmethod
    def get_xunit(ws):
        ws = BaseSX.retrieve(ws)
        xname = ws.getXDimension().name
        if xname == "Time-of-flight":
            return "TOF"
        else:
            return xname.replace("-", "")

    @staticmethod
    def remove_forbidden_peaks(peaks, hm_symbol):
        spgr = SpaceGroupFactory.createSpaceGroup(hm_symbol)
        iremove = []
        for ipk, pk in enumerate(BaseSX.retrieve(peaks)):
            if not spgr.isAllowedReflection(pk.getIntHKL()):
                iremove.append(ipk)
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove, EnableLogging=False)
        return len(iremove)

    @staticmethod
    def remove_peaks_near_powder_line(peaks, resolution=0.05, dmin=0.5, dmax=10, phase="Al", dlist=None, structure=None):
        if not dlist:
            xtal = None
            if structure:
                xtal = structure  # use CrystalStructure provided
            else:
                xtal_structures = {
                    "Cu": ["3.6149 3.6149 3.6149", "F m -3 m", "Cu 0 0 0 1.0 0.05"],
                    "Al": ["4.0495 4.0495 4.0495", "F m -3 m", "Al 0 0 0 1.0 0.05"],
                    "Si": ["5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.05"],
                    "Ti-alpha": ["2.934 2.934 4.703 90 90 120", "P 63/m m c", "Ti 1/3 2/3 0.25 1.0 0.05"],
                    "Ti-beta": ["3.2657 3.2657 3.2657", "I m -3 m", "Ti 0 0 0 1.0 0.05"],
                }
                xtal = CrystalStructure(*xtal_structures[phase])
            # generate list of reflections
            generator = ReflectionGenerator(xtal, ReflectionConditionFilter.StructureFactor)
            hkls = generator.getUniqueHKLs(dmin, dmax)
            dlist = generator.getDValues(hkls)
        # remove peaks within delta_d/d of the peaks in dlist
        peaks = BaseSX.retrieve(peaks)
        dspacings = np.array(peaks.column("DSpacing"))
        iremove = []
        for dpk in dlist:
            ipks = np.where(abs(dspacings - dpk) < resolution * dpk)[0]
            if ipks.size > 0:
                iremove.extend(ipks)
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=[int(irow) for irow in iremove], EnableLogging=False)
        return dlist  # list of dspacing of powder peaks

    @staticmethod
    def remove_duplicate_peaks_by_hkl(peaks):
        peaks = BaseSX.retrieve(peaks)
        hkl = np.array([peaks.column("h"), peaks.column("k"), peaks.column("l")])
        hkl_int = np.round(hkl)
        hkl_unq, idx, ncnts = np.unique(hkl_int, axis=1, return_inverse=True, return_counts=True)
        irows_to_del = []
        for iunq in np.where(ncnts > 1)[0]:
            # ignore unindexed peaks
            if hkl_unq[:, iunq].any():
                iduplicates = np.where(idx == iunq)[0].tolist()
                dhkl_sq = np.sum((hkl[:, iduplicates] - hkl_int[:, iduplicates]) ** 2, axis=0)
                iduplicates.pop(np.argmin(dhkl_sq))
                irows_to_del.extend(iduplicates)
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=irows_to_del, EnableLogging=False)

    @staticmethod
    def remove_duplicate_peaks_by_qlab(peaks, q_tol=0.05):
        """
        Will keep lowest dSpacing peak (often best approx. to peak centre)
        """
        peaks = BaseSX.retrieve(peaks)
        peaks = mantid.SortPeaksWorkspace(
            InputWorkspace=peaks, OutputWorkspace=peaks.name(), ColumnNameToSortBy="DSpacing", EnableLogging=False
        )
        irows_to_del = []
        for ipk in range(peaks.getNumberPeaks() - 1):
            pk = peaks.getPeak(ipk)
            pk_dSpacing = pk.getDSpacing()
            d_tol = q_tol * (pk_dSpacing**2) / (2 * np.pi)
            istep = 1
            while ipk + istep < peaks.getNumberPeaks() and abs(pk_dSpacing - peaks.getPeak(ipk + istep).getDSpacing()) <= d_tol:
                dQ = pk.getQLabFrame() - peaks.getPeak(ipk + istep).getQLabFrame()
                if dQ.norm() <= q_tol:
                    irows_to_del.append(ipk + istep)
                istep += 1
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=irows_to_del, EnableLogging=False)

    @staticmethod
    def remove_unindexed_peaks(peaks):
        """
        Will keep lowest dSpacing peak (often best approx. to peak centre)
        """
        return mantid.FilterPeaks(
            InputWorkspace=peaks,
            OutputWorkspace=BaseSX.retrieve(peaks).name(),
            FilterVariable="h^2+k^2+l^2",
            FilterValue=0,
            Operator=">",
            EnableLogging=False,
        )

    @staticmethod
    def remove_non_integrated_peaks(peaks):
        return mantid.FilterPeaks(
            InputWorkspace=peaks,
            OutputWorkspace=BaseSX.retrieve(peaks).name(),
            FilterVariable="Signal/Noise",
            FilterValue=0,
            Operator=">",
            EnableLogging=False,
        )

    @staticmethod
    def plot_integrated_peaks_MD(
        wsMD: IMDEventWorkspace,
        peaks: IPeaksWorkspace,
        filename: str,
        nbins_max: Optional[int] = 21,
        extent: Optional[float] = 1.5,
        log_norm: Optional[bool] = True,
    ):
        """
        Function to plot summary of IntegratePeaksMD integration comprising 3 colorfill plots per peak saved in a pdf.
        :param wsMD:  MD workspace to plot
        :param peaks: integrated peaks using IntegratePeaksMD
        :param filename: filename to store pdf output
        :param nbins: number of bins along major radius of ellipsoid
        :param extent: extent in units of largest outer background radius
        :param log_norm: use log normalisation in colorscale
        """
        wsMD = BaseSX.retrieve(wsMD)
        peaks = BaseSX.retrieve(peaks)

        # find appropriate getter for peak centre given MD frame
        frame = wsMD.getSpecialCoordinateSystem()
        frame_to_peak_centre_attr = "getQLabFrame"
        if frame == SpecialCoordinateSystem.QSample:
            frame_to_peak_centre_attr = "getQSampleFrame"
        elif frame == SpecialCoordinateSystem.HKL:
            frame_to_peak_centre_attr = "getHKL"

        # loop over peaks and plot
        try:
            with PdfPages(filename) as pdf:
                for ipk, pk in enumerate(peaks):
                    peak_shape = pk.getPeakShape()
                    if peak_shape.shapeName().lower() == "none":
                        continue
                    ws_cut, radii, bg_inner_radii, bg_outer_radii, box_lengths, imax = BaseSX._bin_MD_around_peak(
                        wsMD, pk, peak_shape, nbins_max, extent, frame_to_peak_centre_attr
                    )

                    fig, axes = subplots(1, 3, figsize=(12, 4), subplot_kw={"projection": "mantid"})
                    for iax, ax in enumerate(axes):
                        dims = list(range(3))
                        dims.pop(iax)
                        # plot slice
                        im = ax.imshow(ws_cut.getSignalArray().sum(axis=iax)[:, ::-1].T)  # reshape to match sliceviewer
                        im.set_extent([-1, 1, -1, 1])  # so that ellipsoid is a circle
                        if log_norm and not np.allclose(im.get_array(), 0):
                            im.set_norm(LogNorm())
                        # plot peak position
                        ax.plot(0, 0, "xr")
                        # plot peak representation (circle given extents)
                        patch = Circle((0, 0), radii[imax] / box_lengths[imax], facecolor="none", edgecolor="r", ls="--")
                        ax.add_patch(patch)
                        # add background shell
                        if not np.allclose(bg_outer_radii, 0.0):
                            patch = Circle(
                                (0, 0), bg_outer_radii[imax] / box_lengths[imax], facecolor="none", edgecolor=3 * [0.7]
                            )  # outer radius
                            ax.add_patch(patch)
                            patch = Circle(
                                (0, 0), bg_inner_radii[imax] / box_lengths[imax], facecolor="none", edgecolor=3 * [0.7], ls="--"
                            )  # inner radius
                            ax.add_patch(patch)
                        # format axes
                        ax.set_xlim(-1, 1)
                        ax.set_ylim(-1, 1)
                        ax.set_aspect("equal")
                        ax.set_xlabel(ws_cut.getDimension(dims[0]).name)
                        ax.set_ylabel(ws_cut.getDimension(dims[1]).name)
                    fig.suptitle(
                        f"{ipk} ({','.join(str(np.round(pk.getHKL(), 2))[1:-1].split())})"
                        f"  $I/\\sigma$={np.round(pk.getIntensityOverSigma(), 2)}\n"
                        rf"$\lambda$={np.round(pk.getWavelength(), 2)} $\AA$; "
                        rf"$2\theta={np.round(np.degrees(pk.getScattering()), 1)}^\circ$; "
                        rf"d={np.round(pk.getDSpacing(), 2)} $\AA$"
                    )
                    fig.tight_layout()
                    pdf.savefig(fig)
                    close(fig)
        except OSError:
            raise RuntimeError(
                f"OutputFile ({filename}) could not be opened - please check it is not open by "
                f"another programme and that the user has permission to write to that directory."
            )

    @staticmethod
    def _bin_MD_around_peak(
        wsMD: IMDEventWorkspace, pk: IPeak, peak_shape: PeakShape, nbins_max: int, extent: float, frame_to_peak_centre_attr: str
    ):
        """
        Bin MD workspace in peak region with projection axes given by the axes of the ellipsoid shape
        :param wsMD: MD workspace (with 3 dims)
        :param pk: Peak object that has been integrated
        :param peak_shape: shape of integrated peak (spherical, ellipsoid or none)
        :param nbins_max: number of bins along the major axis of the ellipsoid
        :param extent: extent of output MD workspace along each dimension in units of the outer background radius
        :param frame_to_peak_centre_attr: name of attribute to return peak centre in appropriate frame
        :return ws_cut: MD workspace in peak region
        :return radii: radii of the ellipsoid peak region
        :return bg_inner_radii: inner background radii of the ellipsoid peak region
        :return bg_outer_radii: outer background radii of the ellipsoid peak region
        :return box_lengths: length of each dimension in the MD workspace
        :return: imax: index of dimension with largest radius
        """
        ndims = wsMD.getNumDims()
        radii, bg_inner_radii, bg_outer_radii, evecs, translation = BaseSX._get_ellipsoid_params_from_peak(peak_shape, ndims)
        imax = np.argmax(radii)
        # calc center in frame of ellipsoid axes
        cen = getattr(pk, frame_to_peak_centre_attr)()
        cen = np.matmul(evecs.T, np.array(cen) + translation)
        # get extents
        box_lengths = bg_outer_radii if not np.allclose(bg_outer_radii, 0.0) else radii
        box_lengths = box_lengths * extent
        extents = np.vstack((cen - box_lengths, cen + box_lengths))
        # get nbins along each axis
        nbins = np.array([int(nbins_max * radii[iax] / radii[imax]) for iax in range(ndims)])
        # ensure minimum of 3 bins inside radius along each dim
        min_nbins_in_radius = 3
        nbins_in_radius = np.min(nbins * radii / box_lengths)  # along most coarsely binned dimension
        if nbins_in_radius < min_nbins_in_radius:
            nbins = nbins * min_nbins_in_radius / nbins_in_radius
        # call BinMD
        ws_cut = mantid.BinMD(
            InputWorkspace=wsMD,
            AxisAligned=False,
            BasisVector0=r"Q$_0$,unit," + ",".join(np.array2string(evecs[:, 0], precision=6).strip("[]").split()),
            BasisVector1=r"Q$_1$,unit," + ",".join(np.array2string(evecs[:, 1], precision=6).strip("[]").split()),
            BasisVector2=r"Q$_2$,unit," + ",".join(np.array2string(evecs[:, 2], precision=6).strip("[]").split()),
            OutputExtents=extents.flatten(order="F"),
            OutputBins=nbins.astype(int),
            EnableLogging=False,
            StoreInADS=False,
        )
        return ws_cut, radii, bg_inner_radii, bg_outer_radii, box_lengths, imax

    @staticmethod
    def _get_ellipsoid_params_from_peak(peak_shape: PeakShape, ndims: int):
        """
        Extract ellipsoid parameters (eigenvectors, radii etc.) from PeakShape object
        :param peak_shape: PeakShape object of a integrated peak
        :param ndims: number of dimensions in the MD workspace
        :return radii: array of ellipsoid radii (3 sigma) defining peak region
        :return bg_inner_radii: array of inner radii for ellipsoid background shell
        :return bg_outer_radii: array of outer radii for ellipsoid background shell
        :return evecs: ndims x ndims array of eignevectors - each column corresponds to an ellipsoid axis
        :return translation: translation of the ellipsoid center in the coordinates/frame of the MD workspace integrated
        """
        shape_info = json_loads(peak_shape.toJSON())
        if peak_shape.shapeName().lower() == "spherical":
            BaseSX._convert_spherical_representation_to_ellipsoid(shape_info)
        # get radii
        radii = np.array([shape_info[f"radius{iax}"] for iax in range(ndims)])
        bg_inner_radii = np.array([shape_info[f"background_inner_radius{iax}"] for iax in range(ndims)])
        bg_outer_radii = np.array([shape_info[f"background_outer_radius{iax}"] for iax in range(ndims)])
        # eignevectors of ellipsoid
        evecs = np.zeros((ndims, ndims))
        for iax in range(ndims):
            evec = np.array([float(elem) for elem in shape_info[f"direction{iax}"].split()])
            evecs[:, iax] = evec / np.linalg.norm(evec)
        # get translation in frame of wsMD
        translation = np.array([shape_info[f"translation{iax}"] for iax in range(ndims)])
        return radii, bg_inner_radii, bg_outer_radii, evecs, translation

    @staticmethod
    def _convert_spherical_representation_to_ellipsoid(shape_info: dict):
        """
        Update shape_info dict of a spherical peak shape to have same keys/fields as an ellipsoid
        :param shape_info: dictionary of JSON shape representation
        """
        # copied from mantidqt.widgets.sliceviewer.peaksviewer.representation.ellipsoid - can't import here though
        # convert shape_info dict from sphere to ellipsoid for plotting
        for key in ["radius", "background_inner_radius", "background_outer_radius"]:
            shape_info[f"{key}{0}"] = shape_info.pop(key) if key in shape_info else 0.0
            for idim in [1, 2]:
                shape_info[f"{key}{idim}"] = shape_info[f"{key}{0}"]
        # add axes along basis vecs of frame and set 0 translation
        shape_info.update(
            {
                "direction0": "1 0 0",
                "direction1": "0 1 0",
                "direction2": "0 0 1",
                "translation0": 0.0,
                "translation1": 0.0,
                "translation2": 0.0,
            }
        )

    @staticmethod
    def optimize_goniometer_axis(pk_ws_list: list, iaxis: int, euler_axes: str = "yz", fix_angles: bool = True, apply: bool = False):
        # extract axes and angles from goniometer
        gonio = BaseSX.retrieve(pk_ws_list[0]).run().getGoniometer()
        gonio_axes = [gonio.getAxis(iax)["rotationaxis"] * gonio.getAxis(iax)["sense"] for iax in range(gonio.getNumberAxes())]
        gonio_angles = np.zeros((len(pk_ws_list), len(gonio_axes)))
        for iws, peaks in enumerate(pk_ws_list):
            gonio = BaseSX.retrieve(peaks).run().getGoniometer()
            gonio_angles[iws, :] = [gonio.getAxis(iax)["angle"] for iax in range(gonio.getNumberAxes())]

        # get all U matrices rotated by goniometer
        umats = [BaseSX.retrieve(peaks).sample().getOrientedLattice().getU() for peaks in pk_ws_list]
        umats_rot = [BaseSX.retrieve(peaks).getPeak(0).getGoniometerMatrix() @ umats[irun] for irun, peaks in enumerate(pk_ws_list)]
        # use run with minimum rotation around iaxis as reference (zero is ideal)
        iref = np.argmin(abs(gonio_angles[:, iaxis]))
        umat_ref = umats[iref]
        # optimise goniometer axis (and optionally the goniometer angles)
        pguess = len(euler_axes) * [0]
        if not fix_angles:
            pguess.extend(gonio_angles[:, iaxis])

        result = minimize(
            BaseSX._optimize_goniometer_axis_cost_function,
            pguess,
            args=(umats_rot, umat_ref, gonio_axes, gonio_angles, euler_axes, iaxis),
            method="Nelder-Mead",
        )

        if not result.success:
            logger.error("Failed to optimise goniometer axis - please check UB matrices on the workspaces.")
        else:
            # update gonio axes and angles
            gonio_axes[iaxis] = Rotation.from_euler(euler_axes, result.x[: len(euler_axes)], degrees=True).apply(gonio_axes[iaxis])
            if not fix_angles:
                gonio_angles[:, iaxis] = result.x[len(euler_axes) :]
            # get average U matrix accross all runs (using new goniometer)
            if apply:
                for iws, peaks in enumerate(pk_ws_list):
                    gonio_rots = [
                        Rotation.from_rotvec(axis * gonio_angles[iws, iax], degrees=True).as_matrix() for iax, axis in enumerate(gonio_axes)
                    ]
                    R = reduce(lambda x, y: x @ y, gonio_rots)
                    [pk.setGoniometerMatrix(R) for pk in BaseSX.retrieve(peaks)]
                    # adjust U matrix for optimimised goniometer
                    BaseSX.retrieve(peaks).sample().getOrientedLattice().setU(R.T @ umats_rot[iws])
        return gonio_axes, gonio_angles

    @staticmethod
    def _optimize_goniometer_axis_cost_function(p, umats_rot, umat_ref, gonio_axes, gonio_angles, euler_axes, iaxis):
        # rotate goniometer axis being optimised
        euler_angles = p[: len(euler_axes)]
        these_gonio_axes = gonio_axes.copy()
        these_gonio_axes[iaxis] = Rotation.from_euler(euler_axes, euler_angles, degrees=True).apply(these_gonio_axes[iaxis])
        these_gonio_angles = gonio_angles.copy()
        if len(p) > len(euler_axes):
            these_gonio_angles[:, iaxis] = p[len(euler_axes) :]  # overwrite with angles provided
        # add up total angle difference in rotation to get from predicted to observed U for all runs
        angle_sum = 0
        for irun, urot in enumerate(umats_rot):
            # caluclate predicted goniometer rotation
            gonio_rots = [
                Rotation.from_rotvec(axis * these_gonio_angles[irun, iax], degrees=True).as_matrix()
                for iax, axis in enumerate(these_gonio_axes)
            ]
            urot_predict = reduce(lambda x, y: x @ y, gonio_rots) @ umat_ref
            # add rotation angle between prediced and observed U
            angle_sum = angle_sum + Rotation.from_matrix(urot_predict @ urot.T).magnitude()
        return angle_sum

    @staticmethod
    def find_consistent_ub(peaks_ref, peaks, hkl_tol=0.15, hm_symbol=None, fix_alatt=True):
        sample = BaseSX.retrieve(peaks_ref).sample()
        if not sample.hasOrientedLattice():
            logger.error("Reference workspace must have a UB.")
            return
        mantid.SetUB(Workspace=peaks, UB=sample.getOrientedLattice().getUB(), EnableLogging=False)
        nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=hkl_tol, RoundHKLs=True, CommonUBForAll=True, EnableLogging=False)
        if hm_symbol is not None:
            nremoved = BaseSX.remove_forbidden_peaks(peaks, hm_symbol)  # check doesn't remove unindexed peaks
            nindexed = nindexed - nremoved
        if nindexed < 2:
            logger.error("Reference UB must index at least 2 valid peaks.")
            return
        # optimise UB
        if fix_alatt:
            # get lattice parameters from ws given UB
            latt = BaseSX.retrieve(peaks_ref).sample().getOrientedLattice()
            alatt = {param: getattr(latt, param)() for param in ("a", "b", "c", "alpha", "beta", "gamma")}
            mantid.CalculateUMatrix(PeaksWorkspace=peaks, **alatt)
        else:
            mantid.FindUBUsingIndexedPeaks(PeaksWorkspace=peaks, Tolerance=hkl_tol)

    @staticmethod
    def retrieve(ws):
        if isinstance(ws, str):
            return ADS.retrieve(ws)
        else:
            return ws

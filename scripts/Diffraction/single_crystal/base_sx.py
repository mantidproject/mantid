from typing import Sequence, Optional
import numpy as np
from enum import Enum
import mantid.simpleapi as mantid
from mantid.api import FunctionFactory, AnalysisDataService as ADS
from FindGoniometerFromUB import getSignMaxAbsValInCol
from mantid.geometry import CrystalStructure, SpaceGroupFactory, ReflectionGenerator, ReflectionConditionFilter
from os import path

from abc import ABC  # , abstractmethod


class PEAK_TYPE(Enum):
    FOUND = "found"
    PREDICT = "predicted"
    PREDICT_SAT = "predicted_sat"


class INTEGRATION_TYPE(Enum):
    MD = "int_MD"
    MD_OPTIMAL_RADIUS = "int_MD_opt"
    SKEW = "int_skew"


class BaseSX(ABC):
    def __init__(self, vanadium_runno: str):
        self.runs = dict()
        self.van_runno = vanadium_runno
        self.van_ws = None
        self.spgr = None
        self.gonio_axes = None
        self.sample_dict = None
        self.n_mcevents = 1200
        self.b2b_radius_scale = 5

    # --- getters ---

    def get_ws(self, run):
        try:
            return self.runs[str(run)]["ws"]
        except:
            return None

    def get_md(self, run):
        try:
            return self.runs[str(run)]["MD"]
        except:
            return None

    def get_peaks(self, run, peak_type, integration_type=None):
        if integration_type is None:
            fieldname = peak_type.value
        else:
            fieldname = "_".join([peak_type.value, integration_type.value])
        try:
            return self.runs[str(run)][fieldname]
        except:
            return None

    @staticmethod
    def has_sample(ws):
        return BaseSX.retrieve(ws).sample().getMaterial().numberDensity < 1e-15

    # --- setters ---

    def set_van_ws(self, van_ws):
        self.van_ws = BaseSX.retrieve(van_ws).name()

    def set_peaks(self, run, peaks, peak_type=PEAK_TYPE.FOUND, integration_type=None):
        if integration_type is None:
            fieldname = peak_type.value
        else:
            fieldname = "_".join([peak_type.value, integration_type.value])
        self.runs[str(run)][fieldname] = BaseSX.retrieve(peaks).name()

    def set_spacegroup(self, hm_symbol):
        self.spgr = SpaceGroupFactory.createSpaceGroup(hm_symbol)

    def set_sample(self, **kwargs):
        self.sample_dict = kwargs

    def set_goniometer_axes(self, *args):
        """
        :param axes: collection of axes - e.g. ([x,y,z,hand], [x,y,z,hand]) - in same order to be passed to SetSample
        """
        # convert to strings
        self.gonio_axes = [",".join([str(elem) for elem in axis]) for axis in args]

    def set_mc_abs_nevents(self, nevents):
        self.n_mcevents = nevents

    def set_b2b_radius_scale(self, scale):
        self.b2b_radius_scale = scale

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
            mantid.SetGoniometer(Workspace=ws, **axis_dict)

    # @abstractmethod()
    # def process_data(
    #     self,
    #     runs: Sequence[str],
    #     gonio_angles=None,
    # ):
    #     pass

    @staticmethod
    def convert_ws_to_MD(wsname, md_name=None, frame="Q (lab frame)"):
        if md_name is None:
            md_name = wsname + "_MD"
        xunit = BaseSX.get_xunit(wsname)  # get initial xunit
        BaseSX._normalise_by_bin_width_in_k(wsname)  # normalise by bin-width in K = 2pi/lambda
        wsMD = mantid.ConvertToDiffractionMDWorkspace(
            InputWorkspace=wsname, OutputWorkspace=md_name, LorentzCorrection=True, OneEventPerBin=False, OutputDimensions=frame
        )
        BaseSX._normalise_by_bin_width_in_k(wsname, undo=True)  # normalise by bin-width in K = 2pi/lambda
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target=xunit)
        return wsMD

    def convert_to_MD(self, runs=None, frame="Q (lab frame)"):
        if runs is None:
            runs = self.runs.keys()
        for run in runs:
            wsname = self.runs[str(run)]["ws"]
            md_name = wsname + "_MD"
            BaseSX.convert_ws_to_MD(wsname, md_name, frame)
            self.runs[str(run)]["MD"] = md_name

    def load_isaw_ub(self, isaw_files: Sequence[str], runs: Optional[Sequence[str]] = None, tol=0.15):
        if runs is None:
            runs = self.runs.keys()
        if len(isaw_files) == 1 and runs is None:
            isaw_files = len(runs) * isaw_files  # i.e. use same file for all runs
        if len(isaw_files) == len(runs):
            for run, isaw_file in zip(runs, isaw_files):
                ws = self.get_ws(run)
                try:
                    mantid.LoadIsawUB(InputWorkspace=ws, Filename=isaw_file)
                    peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
                    if peaks is not None:
                        mantid.LoadIsawUB(InputWorkspace=peaks, Filename=isaw_file)
                        mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)
                except:
                    print("LoadIsawUB failed for run " + run)

    def find_ub_using_lattice_params(self, global_B, tol=0.15, **kwargs):
        if global_B:
            peaks = [self.get_peaks(run, PEAK_TYPE.FOUND) for run in self.runs.keys()]
            mantid.FindGlobalBMatrix(PeakWorkspaces=peaks, Tolerance=tol, **kwargs)
        else:
            for run, data in self.runs.items():
                peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
                mantid.FindUBUsingLatticeParameters(PeaksWorkspace=peaks, Tolerance=tol, **kwargs)
                mantid.IndexPeaks(PeaksWorkspace=peaks, Tolerance=tol, RoundHKLs=True)

    def calc_U_matrix(self, *args, **kwargs):
        for run, data in self.runs.items():
            mantid.CalculateUMatrix(PeaksWorkspace=data["found_pks"], *args, **kwargs)

    def calibrate_sample_pos(self, tol=0.15, runs=None):
        if runs is None:
            runs = self.runs.keys()
        for run in self.runs:
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

    def predict_peaks(self, peak_type=PEAK_TYPE.PREDICT, runs=None, **kwargs):
        if runs is None:
            runs = self.runs.keys()
        for run in runs:
            input_ws = None
            ws = self.get_ws(run)
            peaks = self.get_peaks(run, PEAK_TYPE.FOUND)
            if peaks is not None and BaseSX.retrieve(peaks).sample().hasOrientedLattice():
                input_ws = peaks
            elif ws is not None and BaseSX.retrieve(ws).sample().hasOrientedLattice():
                input_ws = ws
            if input_ws is None:
                continue  # skip
            out_peaks_name = "_".join([BaseSX.retrieve(input_ws).name(), peak_type.value])
            if peak_type == PEAK_TYPE.PREDICT:
                mantid.PredictPeaks(InputWorkspace=input_ws, OutputWorkspace=out_peaks_name, **kwargs)
            else:
                mantid.PredictFractionalPeaks(InputWorkspace=input_ws, OutputWorkspace=out_peaks_name, **kwargs)
            self.set_peaks(run, out_peaks_name, peak_type)

    @staticmethod
    def get_radius(pk, ws, ispec, scale, useB=True):
        print("scale = ", scale, "\tuseB = ", useB)
        TOF = pk.getTOF()
        func = FunctionFactory.Instance().createPeakFunction("BackToBackExponential")
        func.setParameter("X0", TOF)  # set centre
        func.setMatrixWorkspace(ws, ispec, 0.0, 0.0)  # calc A,B,S based on peak cen
        if useB:
            dTOF = scale / func.getParameterValue("B")
        else:
            dTOF = scale * func.fwhm()
        # convert dTOF -> dQ
        modQ = 2 * np.pi / pk.getDSpacing()
        radius = (modQ / TOF) * dTOF
        return radius

    # @abstractmethod
    # def mask_detector_edges(ws):
    #     pass  # must be implemented in subclass

    @staticmethod
    def integrate_peaks_MD(wsMD, peaks, out_peaks, **kwargs):
        peaks_int = mantid.IntegratePeaksMD(
            InputWorkspace=wsMD,
            PeaksWorkspace=peaks,
            OutputWorkspace=out_peaks,
            IntegrateIfOnEdge=False,
            UseOnePercentBackgroundCorrection=False,
            **kwargs,
        )
        return peaks_int

    @staticmethod
    def integrate_peaks_MD_optimal_radius(wsMD, peaks, out_peaks, dq=0.01, scale=5, useB=True, ws=None, **kwargs):
        peaks = BaseSX.retrieve(peaks)
        use_empty_inst = ws is None
        if use_empty_inst:
            ws = mantid.LoadEmptyInstrument(InstrumentName=peaks.getInstrument().getFullName(), OutputWorkspace="empty")
        else:
            ws = BaseSX.retrieve(ws)
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="TOF")  # needs to be in TOF for setting B
        ws = BaseSX.retrieve(ws)
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="TOF")  # needs to be in TOF for setting B
        ispecs = ws.getIndicesFromDetectorIDs(peaks.column("DetID"))
        rads = [BaseSX.get_radius(pk, ws, ispecs[ipk], scale, useB) for ipk, pk in enumerate(peaks)]
        bin_edges = np.arange(min(rads), max(rads) + dq, dq)
        ibins = np.digitize(rads, bin_edges[:-1])
        peaks_int = mantid.CloneWorkspace(InputWorkspace=peaks, OutputWorkspace=out_peaks)
        mantid.DeleteTableRows(TableWorkspace=peaks_int, Rows=list(range(peaks_int.getNumberPeaks())))
        for ibin in np.unique(ibins):
            rad = bin_edges[ibin]
            ipks = np.where(ibins == ibin)[0]
            irows_to_del = set(range(peaks.getNumberPeaks())) - set(ipks)
            peaks_subset = mantid.CloneWorkspace(InputWorkspace=peaks, OutputWorkspace=peaks.name() + "_rad")
            mantid.DeleteTableRows(TableWorkspace=peaks_subset, Rows=list(irows_to_del))
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
            peaks_int = mantid.CombinePeaksWorkspaces(LHSWorkspace=peaks_int, RHSWorkspace=peaks_subset, OutputWorkspace=peaks_int.name())
            mantid.DeleteWorkspace(peaks_subset)
            if use_empty_inst:
                mantid.DeleteWorkspace(ws)
        return peaks_int

    @staticmethod
    def integrate_peaks_skew(ws, peaks, out_peaks, **kwargs):
        peaks_int = mantid.IntegratePeaksSkew(InputWorkspace=ws, PeaksWorkspace=peaks, OutputWorkspace=out_peaks, **kwargs)
        return peaks_int

    def integrate_data(self, integration_type, peak_type, tol=0, runs=None, **kwargs):
        if runs is None:
            runs = self.runs.keys()
        for run in runs:
            data = self.runs[str(run)]
            pk_table = data[peak_type.value]
            if peak_type == PEAK_TYPE.FOUND and tol > 0:
                mantid.IndexPeaks(PeaksWorkspace=pk_table, Tolerance=tol, RoundHKLs=True)
                mantid.FilterPeaks(
                    InputWorkspace=pk_table, OutputWorkspace=pk_table, FilterVariable="h^2+k^2+l^2", FilterValue=0, Operator=">"
                )
            # integrate
            peak_int_name = "_".join([BaseSX.retrieve(pk_table).name(), integration_type.value])
            if integration_type == INTEGRATION_TYPE.MD and data["MD"] is not None:
                BaseSX.integrate_peaks_MD(data["MD"], pk_table, peak_int_name, **kwargs)
            elif integration_type == INTEGRATION_TYPE.MD_OPTIMAL_RADIUS and data["MD"] is not None:
                # if possible get original workspace
                if data["ws"] is not None and ADS.doesExist(data["ws"]):
                    BaseSX.integrate_peaks_MD_optimal_radius(data["MD"], pk_table, peak_int_name, ws=data["ws"], **kwargs)
                else:
                    BaseSX.integrate_peaks_MD_optimal_radius(data["MD"], pk_table, peak_int_name, **kwargs)
            else:
                BaseSX.integrate_peaks_skew(data["ws"], pk_table, peak_int_name, **kwargs)
            # store result
            self.set_peaks(run, peak_int_name, peak_type, integration_type)

    def save_integrated_peaks(self, integration_type, peak_type, save_dir: str, save_format: str, make_consistent=True):
        fieldname = "_".join([peak_type.value, integration_type.value])
        all_integrated_peaks = mantid.CreatePeaksWorkspace(InstrumentWorkspace=self.van_ws, NumberOfPeaks=0)
        # get first run - use as reference UB and copy sample
        ws_ref = next(iter(self.runs.keys()))
        for run, data in self.runs.items():
            if make_consistent:
                # ensure indexing is consistent
                self.make_UB_consistent(self.runs[ws_ref][fieldname], data[fieldname])
            # save reflections
            filepath = path.join(save_dir, "_".join([data[fieldname], save_format])) + ".int"
            mantid.SaveReflections(InputWorkspace=data[fieldname], Filename=filepath, Format=save_format, SplitFiles=False)
            mantid.SaveNexus(InputWorkspace=data[fieldname], Filename=filepath[:-3] + "nxs")
            # append to combined table
            all_integrated_peaks = mantid.CombinePeaksWorkspaces(LHSWorkspace=all_integrated_peaks, RHSWorkspace=data[fieldname])
        if len(self.runs) > 1:
            # copy lattice and UB from last run
            mantid.CopySample(
                InputWorkspace=data[fieldname],
                OutputWorkspace=all_integrated_peaks,
                CopyName=False,
                CopyMaterial=False,
                CopyEnvironment=False,
                CopyShape=False,
            )
            # save with run range in filename
            min_ws = min(self.runs.keys(), key=lambda k: int("".join(filter(str.isdigit, k))))
            max_ws = max(self.runs.keys(), key=lambda k: int("".join(filter(str.isdigit, k))))
            filename = "WISH000" + "-".join([min_ws, max_ws]) + data[fieldname][len(min_ws) + 7 :]
            filepath = path.join(save_dir, "_".join([filename, save_format]) + ".int")
            mantid.SaveReflections(InputWorkspace=all_integrated_peaks, Filename=filepath, Format=save_format, SplitFiles=False)
            mantid.SaveNexus(InputWorkspace=all_integrated_peaks, Filename=filepath[:-3] + "nxs")
            mantid.RenameWorkspace(InputWorkspace=all_integrated_peaks, OutputWorkspace=filename)
        else:
            mantid.DeleteWorkspace(all_integrated_peaks)

    # @abstractmethod
    # def load_run(runno):
    #     pass

    def _is_vanadium_processed(self):
        return self.van_ws is not None and ADS.doesExist(self.van_ws)

    @staticmethod
    def _normalise_by_bin_width_in_k(wsname, undo=False):
        ws = mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Momentum")
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
        mantid.RebinToWorkspace(WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs, OutputWorkspace=ws_rhs, PreserveEvents=False)
        mantid.Minus(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs)
        mantid.ReplaceSpecialValues(
            InputWorkspace=ws_lhs, OutputWorkspace=ws_lhs, NaNValue=0, InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=1e-15
        )

    @staticmethod
    def _divide_workspaces(ws_lhs, ws_rhs):
        mantid.RebinToWorkspace(WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs, OutputWorkspace=ws_rhs, PreserveEvents=False)
        mantid.Divide(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs)
        mantid.ReplaceSpecialValues(
            InputWorkspace=ws_lhs, OutputWorkspace=ws_lhs, NaNValue=0, InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=1e-15
        )

    @staticmethod
    def make_UB_consistent(ws_ref, ws):
        # compare U matrix to perform TransformHKL to preserve indexing
        U_ref = BaseSX.retrieve(ws_ref).sample().getOrientedLattice().getU()
        U = BaseSX.retrieve(ws).sample().getOrientedLattice().getU()
        # find transform required  ( U_ref = U T^-1) - see TransformHKL docs for details
        transform = np.linalg.inv(getSignMaxAbsValInCol(np.matmul(np.linalg.inv(U), U_ref)))
        mantid.TransformHKL(PeaksWorkspace=ws, HKLTransform=transform, FindError=False)

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
        if not ws.getXDimension().name == "TOF":
            ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target="TOF")  # FindSXPeaks requires TOF
        # extract y data (to use to determine to determine threshold)
        mantid.FindSXPeaks(
            InputWorkspace=ws,
            PeakFindingStrategy="AllPeaks",
            AbsoluteBackground=bg,
            ResolutionStrategy="AbsoluteResolution",
            OutputWorkspace=out_peaks,
            **kwargs,
        )
        if not ws.getXDimension().name == "TOF":
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target=xunit)
        return out_peaks

    @staticmethod
    def get_xunit(ws):
        ws = BaseSX.retrieve(ws)
        xname = ws.getXDimension().name
        if xname == "Time-of-flight":
            return "TOF"
        else:
            return xname.replace("-", "")

    def remove_forbidden_peaks(self, peaks):
        if self.spgr is None:
            return
        iremove = []
        for ipk, pk in enumerate(BaseSX.retrieve(peaks)):
            if not self.spgr.isAllowedReflection(pk.getIntHKL()):
                iremove.append(ipk)
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)

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
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        return dlist  # list of dspacing of powder peaks

    @staticmethod
    def remove_duplicate_peaks_by_hkl(peaks, hkl_tol=0.05):
        peaks = BaseSX.retrieve(peaks)
        mantid.IndexPeaks(peaks, Tolerance=hkl_tol, RoundHKLs=False)
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
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=irows_to_del)

    @staticmethod
    def remove_duplicate_peaks_by_qlab(peaks, q_tol=0.05):
        """
        Will keep lowest dSpacing peak (often best approx. to peak centre)
        """
        peaks = BaseSX.retrieve(peaks)
        peaks = mantid.SortPeaksWorkspace(InputWorkspace=peaks, OutputWorkspace=peaks.name(), ColumnNameToSortBy="DSpacing")
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
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=irows_to_del)

    @staticmethod
    def retrieve(ws):
        if isinstance(ws, str):
            return ADS.retrieve(ws)
        else:
            return ws

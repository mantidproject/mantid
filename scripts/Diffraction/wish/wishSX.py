from typing import Sequence, Optional
import numpy as np
from enum import Enum
import mantid.simpleapi as mantid
from mantid.api import FunctionFactory, AnalysisDataService as ADS
from FindGoniometerFromUB import getSignMaxAbsValInCol
from mantid.geometry import CrystalStructure, ReflectionGenerator, ReflectionConditionFilter
from os import path


class PEAK_TYPE(Enum):
    FOUND = 'found'
    PREDICT = 'predicted'
    PREDICT_SAT = 'predicted_sat'


class INTEGRATION_TYPE(Enum):
    MD = 'int_MD'
    MD_OPTIMAL_RADIUS = 'int_MD_opt'
    SKEW = 'int_skew'


class WishSX:
    def __init__(self,
                 vanadium_runno: str):
        self.runs = dict()
        self.van_runno = vanadium_runno
        self.van_ws = None
        self.sphere_shape = '''<sphere id="sphere">
                               <centre x="0.0"  y="0.0" z="0.0" />
                               <radius val="0.0025"/>
                               </sphere>'''  # sphere radius 2.5mm
        self.xtal_structure = None

    def set_van_ws(self, van_ws):
        self.van_ws = van_ws

    def set_run_data(self, run, data_dict):
        self.runs[str(run)] = data_dict

    def set_xtal_structure(self, *args, **kwargs):
        self.xtal_structure = CrystalStructure(*args, **kwargs)

    def delete_run_data(self, run, del_MD=False):
        run = str(run)
        fields = ['ws', 'MD'] if del_MD else ['ws']
        for field in fields:
            if self.runs[run][field]:
                mantid.DeleteWorkspace(self.runs[run][field])
                self.runs[run][field] = None

    def process_data(self,
                     runs: Sequence[str],
                     sample_dict: Optional[dict] = None,
                     nEventsMC=1200,
                     gonio_motor="wccr",
                     gonio_axis=None,
                     gonio_angles=None):

        if not self._is_vanadium_processed():
            return

        for irun, run in enumerate(runs):
            wsname = self.load_run(run)
            # set goniometer
            if gonio_axis is not None:
                mantid.SetGoniometer(Workspace=wsname, Axis0=",".join([str(gonio_angles[irun]), goino_axis]))
            elif gonio_motor is not None and WishSX.retrieve(wsname).run().hasProperty(gonio_motor):
                wccr = ADS.retrieve(wsname).run().getPropertyAsSingleValueWithTimeAveragedMean(gonio_motor)
                mantid.SetGoniometer(Workspace=wsname, Axis0=str(wccr) + ',0,1,0,1')  # vertical
            # normalise by vanadium
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target='Wavelength')
            self._divide_workspaces(wsname, self.van_ws)
            # set sample geometry and material
            if sample_dict:
                mantid.SetSample(wsname, **sample_dict)
                # correct for attenuation
                if "<sphere" in ADS.retrieve(wsname).sample().getShape().getShapeXML():
                    transmission = mantid.SphericalAbsorption(InputWorkspace=wsname,
                                                              OutputWorkspace='transmission')
                else:
                    transmission = mantid.MonteCarloAbsorption(InputWorkspace=wsname,
                                                               OutputWorkspace='transmission', EventsPerPoint=nEventsMC)
                self._divide_workspaces(wsname, transmission)
                mantid.DeleteWorkspace(transmission)
            mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target='TOF')
            # save results in dictionary
            self.runs[str(run)] = {'ws': wsname, 'MD': None, 'found': None, 'predicted': None,
                                   'predicted_sat': None}

    @staticmethod
    def convert_ws_to_MD(wsname, md_name=None, frame='Q (lab frame)'):
        if md_name is None:
            md_name = wsname + '_MD'
        xunit = WishSX.get_xunit(wsname)  # get initial xunit
        WishSX._normalise_by_bin_width(wsname)  # normalise by bin-width in K = 2pi/lambda
        wsMD = mantid.ConvertToDiffractionMDWorkspace(InputWorkspace=wsname, OutputWorkspace=md_name,
                                                      LorentzCorrection=True, OneEventPerBin=False,
                                                      OutputDimensions=frame)
        WishSX._normalise_by_bin_width(wsname, undo=True)  # normalise by bin-width in K = 2pi/lambda
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target=xunit)
        return wsMD

    def convert_to_MD(self, runs=None, frame='Q (lab frame)'):
        if runs is None:
            runs = self.runs.keys()
        for run in runs:
            wsname = self.runs[str(run)]['ws']
            md_name = wsname + '_MD'
            WishSX.convert_ws_to_MD(wsname, md_name, frame)
            self.runs[str(run)]['MD'] = md_name

    def load_isaw_ub(self, isaw_files: Sequence[str], runs: Optional[Sequence[str]] = None, tol=0.15):
        if runs is None:
            runs = self.runs.keys()
        if len(isaw_files) == 1 and runs is None:
            isaw_files = len(runs) * isaw_files  # i.e. use same file for all runs
        if len(isaw_files) == len(runs):
            for run, isaw_file in zip(runs, isaw_files):
                try:
                    mantid.LoadIsawUB(InputWorkspace=self.runs[run]['ws'], Filename=isaw_file)
                    if self.runs[run]['found_pks'] is not None:
                        mantid.LoadIsawUB(InputWorkspace=self.runs[run]['found_pks'], Filename=isaw_file)
                        mantid.IndexPeaks(PeaksWorkspace=self.runs[run]['found_pks'], Tolerance=tol, RoundHKLs=True)
                except:
                    print('LoadIsawUB failed for run ' + run)

    def find_ub_using_lattice_params(self, global_B, tol=0.15, **kwargs):
        if global_B:
            mantid.FindGlobalBMatrix(PeakWorkspaces=[data['found_pks'] for data in self.runs.values()],
                                     Tolerance=tol, **kwargs)
        else:
            for run, data in self.runs.items():
                mantid.FindUBUsingLatticeParameters(PeaksWorkspace=data['found_pks'], Tolerance=tol, **kwargs)
                mantid.IndexPeaks(PeaksWorkspace=data['found_pks'], Tolerance=tol, RoundHKLs=True)

    def calc_U_matrix(self, *args, **kwargs):
        for run, data in self.runs.items():
            mantid.CalculateUMatrix(PeaksWorkspace=data['found_pks'], *args, **kwargs)

    def calibrate_sample_pos(self, tol=0.15):
        for run, data in self.runs.items():
            mantid.IndexPeaks(PeaksWorkspace=data['found_pks'], Tolerance=tol, RoundHKLs=True)
            mantid.OptimizeCrystalPlacement(PeaksWorkspace=data['found_pks'],
                                            ModifiedPeaksWorkspace=data['found_pks'],
                                            FitInfoTable=run + '_sample_pos_fit_info',
                                            AdjustSampleOffsets=True,
                                            OptimizeGoniometerTilt=True,
                                            MaxSamplePositionChangeMeters=0.01,
                                            MaxIndexingError=tol)
            mantid.IndexPeaks(PeaksWorkspace=data['found_pks'], Tolerance=tol, RoundHKLs=True)

    def predict_peaks(self, peak_type=PEAK_TYPE.PREDICT, **kwargs):
        for run, data in self.runs.items():
            ws = None
            if data['found'] is not None and WishSX.retrieve(data['found']).sample().hasOrientedLattice():
                ws = data['found']
            elif data['ws'] is not None and WishSX.retrieve(data['ws']).sample().hasOrientedLattice():
                ws = data['ws']
            if ws is None:
                continue  # skip
            out_peaks_name = '_'.join([WishSX.retrieve(ws).name(), peak_type.value])
            if peak_type == PEAK_TYPE.PREDICT:
                mantid.PredictPeaks(InputWorkspace=ws, OutputWorkspace=out_peaks_name, **kwargs)
            else:
                mantid.PredictFractionalPeaks(InputWorkspace=ws, OutputWorkspace=out_peaks_name, **kwargs)
            self.runs[run][peak_type.value] = out_peaks_name

    @staticmethod
    def get_radius(pk, ws, ispec, scale=5):
        TOF = pk.getTOF()
        func = FunctionFactory.Instance().createPeakFunction("BackToBackExponential")
        func.setParameter('X0', TOF)  # set centre
        func.setMatrixWorkspace(ws, ispec, 0.0, 0.0)  # calc A,B,S based on peak cen
        dTOF = scale / func.getParameterValue('B')
        # convert dTOF -> dQ
        modQ = 2 * np.pi / pk.getDSpacing()
        radius = (modQ / TOF) * dTOF
        return radius

    @staticmethod
    def mask_detector_edges(ws):
        # mask pixels on ends of tubes
        mantid.MaskBTP(Workspace=ws, Pixel='1-16,496-512')
        # only mask tubes on panels with edge facing beam in/out
        mantid.MaskBTP(Workspace=ws, Bank='5-6', Tube='152')
        mantid.MaskBTP(Workspace=ws, Bank='1,10', Tube='1')

    @staticmethod
    def integrate_peaks_MD(wsMD, peaks, out_peaks, **kwargs):
        WishSX.remove_peaks_on_edge(peaks)
        WishSX.mask_detector_edges(peaks)
        # integrate
        peaks_int = mantid.IntegratePeaksMD(InputWorkspace=wsMD, PeaksWorkspace=peaks, OutputWorkspace=out_peaks,
                                            MaskEdgeTubes=False, IntegrateIfOnEdge=False,
                                            UseOnePercentBackgroundCorrection=False,
                                            **kwargs)
        return peaks_int

    @staticmethod
    def integrate_peaks_MD_optimal_radius(wsMD, peaks, out_peaks, dq=0.01, scale=5, ws=None, **kwargs):
        peaks = WishSX.retrieve(peaks)
        use_empty_inst = ws is None
        if use_empty_inst:
            ws = mantid.LoadEmptyInstrument(InstrumentName='WISH', OutputWorkspace='empty')
        else:
            ws = WishSX.retrieve(ws)
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='TOF')  # needs to be in TOF for setting B
        ispecs = ws.getIndicesFromDetectorIDs(peaks.column('DetID'))
        rads = [WishSX.get_radius(pk, ws, ispecs[ipk], scale) for ipk, pk in enumerate(peaks)]
        bin_edges = np.arange(min(rads), max(rads) + dq, dq)
        ibins = np.digitize(rads, bin_edges[:-1])
        peaks_int = mantid.CreatePeaksWorkspace(InstrumentWorkspace=peaks, NumberOfPeaks=0, OutputWorkspace=out_peaks)
        for ibin in np.unique(ibins):
            rad = bin_edges[ibin]
            ipks = np.where(ibins == ibin)[0]
            irows_to_del = set(range(peaks.getNumberPeaks())) - set(ipks)
            peaks_subset = mantid.CloneWorkspace(InputWorkspace=peaks, OutputWorkspace=peaks.name() + '_rad')
            mantid.DeleteTableRows(TableWorkspace=peaks_subset, Rows=list(irows_to_del))
            # integrate
            peaks_subset = WishSX.integrate_peaks_MD(wsMD, peaks_subset.name(), peaks_subset.name(),
                                                     PeakRadius=rad, BackgroundInnerRadius=rad,
                                                     BackgroundOuterRadius=rad * (2 ** (1 / 3)), **kwargs)
            peaks_int = mantid.CombinePeaksWorkspaces(LHSWorkspace=peaks_int, RHSWorkspace=peaks_subset,
                                                      OutputWorkspace=peaks_int.name())
            mantid.DeleteWorkspace(peaks_subset)
            if use_empty_inst:
                mantid.DeleteWorkspace(ws)
        return peaks_int

    @staticmethod
    def integrate_peaks_skew(ws, peaks, out_peaks, **kwargs):
        WishSX.remove_peaks_on_edge(peaks)
        # integrate
        peaks_int = mantid.IntegratePeaksSkew(InputWorkspace=ws, PeaksWorkspace=peaks, OutputWorkspace=out_peaks,
                                              **kwargs)
        return peaks_int

    def integrate_data(self, integration_type, peak_type, tol=0, runs=None, **kwargs):
        if runs is None:
            runs = self.runs.keys()
        for run in runs:
            data = self.runs[str(run)]
            pk_table = data[peak_type.value]
            if peak_type == PEAK_TYPE.FOUND and tol > 0:
                mantid.IndexPeaks(PeaksWorkspace=pk_table, Tolerance=tol, RoundHKLs=True)
                mantid.FilterPeaks(InputWorkspace=pk_table, OutputWorkspace=pk_table,
                                   FilterVariable='h^2+k^2+l^2', FilterValue=0, Operator='>')
            # integrate
            peak_int_name = '_'.join([WishSX.retrieve(pk_table).name(), integration_type.value])
            if integration_type == INTEGRATION_TYPE.MD and data['MD'] is not None:
                WishSX.integrate_peaks_MD(data['MD'], pk_table, peak_int_name, **kwargs)
            elif integration_type == INTEGRATION_TYPE.MD_OPTIMAL_RADIUS and data['MD'] is not None:
                # if possible get original workspace
                if data['ws'] is not None and ADS.doesExist(data['ws']):
                    WishSX.integrate_peaks_MD_optimal_radius(data['MD'], pk_table, peak_int_name,
                                                             ws=data['ws'], **kwargs)
                else:
                    WishSX.integrate_peaks_MD_optimal_radius(data['MD'], pk_table, peak_int_name, **kwargs)
            else:
                WishSX.integrate_peaks_skew(data['ws'], pk_table, peak_int_name, **kwargs)
            # store result
            fieldname = '_'.join([peak_type.value, integration_type.value])
            self.runs[str(run)][fieldname] = peak_int_name

    def save_integrated_peaks(self, integration_type, peak_type, save_dir: str, save_format: str,
                              make_consistent=True):
        fieldname = '_'.join([peak_type.value, integration_type.value])
        all_integrated_peaks = mantid.CreatePeaksWorkspace(InstrumentWorkspace=self.van_ws, NumberOfPeaks=0)
        # get first run - use as reference UB and copy sample
        ws_ref = next(iter(self.runs.keys()))
        for run, data in self.runs.items():
            if make_consistent:
                # ensure indexing is consistent
                self.make_UB_consistent(self.runs[ws_ref][fieldname], data[fieldname])
            # save reflections
            filepath = path.join(save_dir, '_'.join([data[fieldname], save_format])) + '.int'
            mantid.SaveReflections(InputWorkspace=data[fieldname], Filename=filepath,
                                   Format=save_format, SplitFiles=False)
            mantid.SaveNexus(InputWorkspace=data[fieldname], Filename=filepath[:-3] + 'nxs')
            # append to combined table
            all_integrated_peaks = mantid.CombinePeaksWorkspaces(LHSWorkspace=all_integrated_peaks,
                                                                 RHSWorkspace=data[fieldname])
        if len(self.runs) > 1:
            # copy lattice and UB from last run
            mantid.CopySample(InputWorkspace=data[fieldname], OutputWorkspace=all_integrated_peaks,
                              CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False)
            # save with run range in filename
            min_ws = min(self.runs.keys(), key=lambda k: int("".join(filter(str.isdigit, k))))
            max_ws = max(self.runs.keys(), key=lambda k: int("".join(filter(str.isdigit, k))))
            filename = 'WISH000' + '-'.join([min_ws, max_ws]) + data[fieldname][len(min_ws) + 7:]
            filepath = path.join(save_dir, '_'.join([filename, save_format]) + '.int')
            mantid.SaveReflections(InputWorkspace=all_integrated_peaks, Filename=filepath,
                                   Format=save_format, SplitFiles=False)
            mantid.SaveNexus(InputWorkspace=all_integrated_peaks, Filename=filepath[:-3] + 'nxs')
            mantid.RenameWorkspace(InputWorkspace=all_integrated_peaks, OutputWorkspace=filename)
        else:
            mantid.DeleteWorkspace(all_integrated_peaks)

    def remove_forbidden_peaks(self, peak_type=PEAK_TYPE.FOUND):
        if self.xtal_structure is None:
            return
        for run, data in self.runs.items():
            pk_table = data[peak_type.value]
            iforbidden = []
            for ipk, pk in enumerate(pk_table):
                if not self.xtal_structure.getSpaceGroup().isAllowedReflection(pk.getIntHKL()):
                    iforbidden.append(ipk)
            mantid.DeleteTableRows(TableWorkspace=pk_table, Rows=iforbidden)

    def process_vanadium(self, npoints=301):
        # vanadium
        self.van_ws = self.load_run(self.van_runno)
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target='Wavelength')
        mantid.SmoothNeighbours(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Radius=3)
        mantid.SmoothData(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, NPoints=npoints)
        # correct vanadium for absorption
        mantid.SetSample(self.van_ws, Geometry={'Shape': 'CSG', 'Value': self.sphere_shape},
                         Material={'ChemicalFormula': 'V0.95-Nb0.05', 'SampleNumberDensity': 0.0722})
        vanadium_transmission = mantid.SphericalAbsorption(InputWorkspace=self.van_ws,
                                                           OutputWorkspace='vanadium_transmission')
        self._divide_workspaces(self.van_ws, vanadium_transmission)
        mantid.DeleteWorkspace(vanadium_transmission)

    @staticmethod
    def load_run(runno):
        wsname = 'WISH000' + str(runno)
        ws, mon = mantid.LoadRaw(Filename=wsname + '.raw', OutputWorkspace=wsname,
                                 LoadMonitors="Separate")
        mon_name = mon.name()
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=6000, XMax=99000)
        # replace empty bin errors with 1 count
        mantid.SetUncertainties(InputWorkspace=wsname, OutputWorkspace=wsname, SetError='oneIfZero')
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target='Wavelength')
        mantid.ConvertUnits(InputWorkspace=mon_name, OutputWorkspace=mon_name, Target='Wavelength')
        mantid.NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname,
                                  MonitorWorkspaceIndex=3, MonitorWorkspace=mon_name)
        mantid.ReplaceSpecialValues(InputWorkspace=wsname, OutputWorkspace=wsname,
                                    NaNValue=0, InfinityValue=0)
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=0.8)
        mantid.NormaliseByCurrent(InputWorkspace=wsname, OutputWorkspace=wsname)
        return wsname

    def _is_vanadium_processed(self):
        return self.van_ws is not None and ADS.doesExist(self.van_ws)

    @staticmethod
    def _normalise_by_bin_width(wsname, undo=False):
        ws = mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target='Momentum')
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
        mantid.RebinToWorkspace(WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs,
                                OutputWorkspace=ws_rhs, PreserveEvents=False)
        mantid.Minus(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs)
        mantid.ReplaceSpecialValues(InputWorkspace=ws_lhs, OutputWorkspace=ws_lhs,
                                    NaNValue=0, InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=1e-15)

    @staticmethod
    def _divide_workspaces(ws_lhs, ws_rhs):
        mantid.RebinToWorkspace(WorkspaceToRebin=ws_rhs, WorkspaceToMatch=ws_lhs,
                                OutputWorkspace=ws_rhs, PreserveEvents=False)
        mantid.Divide(LHSWorkspace=ws_lhs, RHSWorkspace=ws_rhs, OutputWorkspace=ws_lhs)
        mantid.ReplaceSpecialValues(InputWorkspace=ws_lhs, OutputWorkspace=ws_lhs,
                                    NaNValue=0, InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=1e-15)

    @staticmethod
    def make_UB_consistent(ws_ref, ws):
        # compare U matrix to perform TransformHKL to preserve indexing
        U_ref = WishSX.retrieve(ws_ref).sample().getOrientedLattice().getU()
        U = WishSX.retrieve(ws).sample().getOrientedLattice().getU()
        # find transform required  ( U_ref = U T^-1) - see TransformHKL docs for details
        transform = np.linalg.inv(getSignMaxAbsValInCol(np.matmul(np.linalg.inv(U), U_ref)))
        mantid.TransformHKL(PeaksWorkspace=ws, HKLTransform=transform, FindError=False)

    @staticmethod
    def find_sx_peaks(ws, bg, out_pk_wsname='peaks'):
        ws = WishSX.retrieve(ws)
        # get unit to convert back to after peaks found
        xunit = WishSX.get_xunit(ws)
        if not ws.getXDimension().name == 'TOF':
            ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(),
                                     Target='TOF')  # FindSXPeaks requires TOF
        # extract y data (to use to determine to detemrine threshold)
        mantid.FindSXPeaks(InputWorkspace=ws, PeakFindingStrategy='AllPeaks',
                           AbsoluteBackground=bg, ResolutionStrategy='AbsoluteResolution',
                           XResolution=500, PhiResolution=3, TwoThetaResolution=2,
                           OutputWorkspace=out_pk_wsname)
        WishSX.remove_peaks_on_edge(out_pk_wsname)
        if not ws.getXDimension().name == 'TOF':
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target=xunit)
        return out_pk_wsname

    @staticmethod
    def find_sx_peaks_bankwise(wsname, out_pk_wsname=None, nstd=8, nbunch=3,
                               lambda_min=1.25, lambda_max=7):
        ws = mantid.Rebunch(InputWorkspace=wsname, OutputWorkspace=wsname + '_rb', NBunch=nbunch)
        WishSX.mask_detector_edges(ws)
        # mask additional tubes at detector edge
        mantid.MaskBTP(Workspace=ws, Bank='5-6', Tube='76,151-152')
        mantid.MaskBTP(Workspace=ws, Bank='1,10', Tube='1-2,77')
        # Crop in wavelength
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='Wavelength')
        mantid.CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=lambda_min, XMax=lambda_max)
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='TOF')  # FindSXPeaks requries TOF
        # extract y data (to use to determine to determine threshold)
        if out_pk_wsname is None:
            out_pk_wsname = wsname + "_peaks"
        peaks = mantid.CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace=out_pk_wsname)
        for bank in range(1, 11):
            ispec_min, ispec_max = 19456 * np.array([bank - 1, bank])
            y = np.max(ws.extractY()[ispec_min:ispec_max, :], axis=1)
            avg = np.median(y)
            std = (np.percentile(y, 90) - avg) / 1.2815
            cutoff = avg + nstd * std
            # Run FindSXPeaks
            pks_bank = mantid.FindSXPeaks(InputWorkspace=ws, PeakFindingStrategy='AllPeaks',
                                          AbsoluteBackground=cutoff, ResolutionStrategy='AbsoluteResolution',
                                          XResolution=500, PhiResolution=3, TwoThetaResolution=2,
                                          StartWorkspaceIndex=int(ispec_min), EndWorkspaceIndex=int(ispec_max) - 1)
            peaks = mantid.CombinePeaksWorkspaces(LHSWorkspace=pks_bank, RHSWorkspace=peaks,
                                                  OutputWorkspace=out_pk_wsname, CombineMatchingPeaks=True,
                                                  Tolerance=0.05)
        mantid.DeleteWorkspace(pks_bank)
        mantid.DeleteWorkspace(ws)
        return out_pk_wsname

    @staticmethod
    def get_xunit(ws):
        ws = WishSX.retrieve(ws)
        xname = ws.getXDimension().name
        if xname == 'Time-of-flight':
            return "TOF"
        else:
            return xname.replace('-', "")

    @staticmethod
    def remove_peaks_on_edge(peaks, nedge_tube=2, nedge_pix=16):
        peaks = WishSX.retrieve(peaks)
        # filter peaks on edge of 0tubes
        row = np.array(peaks.column("Row"))
        iremove = np.where(np.logical_or(row < nedge_pix, row > 512 - nedge_pix))[0]
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        # filter out peaks on tubes at edge of detector banks
        col = np.array(peaks.column("Col"))
        bank = np.array([int(name[-2:]) for name in peaks.column("BankName")])
        iremove = np.where(np.logical_and(col < nedge_tube, bank == 1))[0]
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        iremove = np.where(np.logical_and(col > 152 - nedge_tube, bank == 5))[0]
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        iremove = np.where(np.logical_and(col < nedge_tube, bank == 10))[0]
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        iremove = np.where(np.logical_and(col > 152 - nedge_tube, bank == 6))[0]
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)

    @staticmethod
    def remove_peaks_near_powder_line(peaks, resolution=0.05, dmin=0.5, dmax=10, phase='Al', dlist=None,
                                      structure=None):
        if not dlist:
            xtal = None
            if structure:
                xtal = structure  # use CrystalStructure provided
            else:
                xtal_structures = {'Cu': ['3.6149 3.6149 3.6149', 'F m -3 m', 'Cu 0 0 0 1.0 0.05'],
                                   'Al': ['4.0495 4.0495 4.0495', 'F m -3 m', 'Al 0 0 0 1.0 0.05'],
                                   'Si': ["5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.05"],
                                   'Ti-alpha': ["2.934 2.934 4.703 90 90 120", "P 63/m m c",
                                                "Ti 1/3 2/3 0.25 1.0 0.05"],
                                   'Ti-beta': ["3.2657 3.2657 3.2657", "I m -3 m", "Ti 0 0 0 1.0 0.05"]}
                xtal = CrystalStructure(*xtal_structures[phase])
            # generate list of reflections
            generator = ReflectionGenerator(xtal, ReflectionConditionFilter.StructureFactor)
            hkls = generator.getUniqueHKLs(dmin, dmax)
            dlist = generator.getDValues(hkls)
        # remove peaks within delta_d/d of the peaks in dlist
        peaks = WishSX.retrieve(peaks)
        dspacings = np.array(peaks.column('DSpacing'))
        iremove = []
        for dpk in dlist:
            ipks = np.where(abs(dspacings - dpk) < resolution * dpk)[0]
            if ipks.size > 0:
                iremove.extend(ipks)
        mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)
        return dlist  # list of dspacing of powder peaks

    @staticmethod
    def remove_duplicate_peaks_by_hkl(peaks, hkl_tol=0.05):
        peaks = WishSX.retrieve(peaks)
        mantid.IndexPeaks(peaks, Tolerance=hkl_tol, RoundHKLs=False)
        hkl = np.array([peaks.column('h'), peaks.column('k'), peaks.column('l')])
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
        peaks = WishSX.retrieve(peaks)
        peaks = mantid.SortPeaksWorkspace(InputWorkspace=peaks, OutputWorkspace=peaks.name(),
                                          ColumnNameToSortBy='DSpacing')
        irows_to_del = []
        for ipk in range(peaks.getNumberPeaks() - 1):
            pk = peaks.getPeak(ipk)
            pk_dSpacing = pk.getDSpacing()
            d_tol = q_tol * (pk_dSpacing ** 2) / (2 * np.pi)
            istep = 1
            while ipk + istep < peaks.getNumberPeaks() and abs(
                    pk_dSpacing - peaks.getPeak(ipk + istep).getDSpacing()) <= d_tol:
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

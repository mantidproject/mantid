# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence
import numpy as np
import mantid.simpleapi as mantid
from mantid.kernel import logger
from Diffraction.single_crystal.base_sx import BaseSX, PEAK_TYPE, INTEGRATION_TYPE
from mantid.api import AnalysisDataService as ADS
from plugins.algorithms.FindGoniometerFromUB import getR
from os import path

tof_min = 700
tof_max = 18800


class SXD(BaseSX):
    def __init__(self, vanadium_runno=None, empty_runno=None, detcal_path=None, file_ext=".raw", scale_integrated=True):
        self.empty_runno = empty_runno
        self.detcal_path = detcal_path
        super().__init__(vanadium_runno, file_ext, scale_integrated)
        self.sphere_shape = """<sphere id="sphere">
                               <centre x="0.0"  y="0.0" z="0.0" />
                               <radius val="0.003"/>
                               </sphere>"""  # sphere radius 3mm  - used for vanadium and NaCl

    def process_data(self, runs: Sequence[str], *args):
        """
        Function to load and normalise a sequence of runs
        :param runs: sequence of run numbers (can be ints or string)
        :param args: goniometer angles - one positional arg for each goniometer axis/motor
        :return: workspace name of last run loaded

        Examples for providing goniometer angles (passed to SetGoniometer)
        e.g. using motor names for 2 axes defined sxd.set_goniometer_axes for 3 runs
        sxd.process_data(range(3), "wccr", "ewald_pos")
        e.g. using a sequence of angles for each motor
        sxd.process_data(range(3), [1,2,3], [4,5,6]])  # e.g. for the first run wccr=1 and ewald_pos=4
        """
        gonio_angles = args
        for irun, run in enumerate(runs):
            wsname = self.load_run(run, self.file_ext)
            # set goniometer
            if self.gonio_axes is not None:
                if len(gonio_angles) != len(self.gonio_axes):
                    logger.warning("No goniometer will be applied as the number of goniometer angles doesn't match the number of axes set.")
                elif isinstance(gonio_angles[0], str):
                    self._set_goniometer_on_ws(wsname, gonio_angles)
                else:
                    if len(gonio_angles[0]) == len(runs):
                        # gonio_angles is a list of individual or tuple motor angles for each run
                        self._set_goniometer_on_ws(wsname, [angles[irun] for angles in gonio_angles])
                    else:
                        logger.warning(
                            "No goniometer will be applied as the number of goniometer angles for each motor doesn't "
                            "match the number of runs."
                        )
            # normalise by vanadium
            self._divide_workspaces(wsname, self.van_ws)  # van_ws has been converted to TOF
            # set sample (must be done after gonio to rotate shape) and correct for attenuation
            if self.sample_dict is not None:
                mantid.SetSample(wsname, EnableLogging=False, **self.sample_dict)
                if not self.scale_integrated:
                    mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength", EnableLogging=False)
                    if "<sphere" in ADS.retrieve(wsname).sample().getShape().getShapeXML():
                        transmission = mantid.SphericalAbsorption(
                            InputWorkspace=wsname, OutputWorkspace="transmission", EnableLogging=False
                        )
                    else:
                        transmission = mantid.MonteCarloAbsorption(
                            InputWorkspace=wsname, OutputWorkspace="transmission", EventsPerPoint=self.n_mcevents, EnableLogging=False
                        )
                    self._divide_workspaces(wsname, transmission)
                    mantid.DeleteWorkspace(transmission)
                    mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="TOF", EnableLogging=False)
            # save results in dictionary
            self.set_ws(run, wsname)
        return wsname

    def load_run(self, runno, file_ext=".raw"):
        wsname = "SXD" + str(runno)
        mantid.Load(Filename=wsname + self.file_ext, OutputWorkspace=wsname, EnableLogging=False)
        if self.detcal_path is not None:
            mantid.LoadParameterFile(Workspace=wsname, Filename=self.detcal_path, EnableLogging=False)
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=tof_min, XMax=tof_max, EnableLogging=False)
        mantid.NormaliseByCurrent(InputWorkspace=wsname, OutputWorkspace=wsname, EnableLogging=False)
        return wsname

    def _replace_spectra_with_focussed_bank(self, wsname, bank_grouping_ws, ngroups):
        # important to focus in TOF not d-spacing (lambda would be better but not compatible with Diffractionfocussing)!
        ws = SXD.retrieve(wsname)
        banks_foc = mantid.GroupDetectors(
            InputWorkspace=ws,
            OutputWorkspace="grouped",
            IgnoreGroupNumber=False,
            CopyGroupingFromWorkspace=bank_grouping_ws,
            EnableLogging=False,
        )
        for igrp in range(ngroups):
            ybank = banks_foc.readY(igrp)
            ebank = banks_foc.readE(igrp)
            ybank_sum = ybank.sum()
            # get detector IDs in raw ws that contribute to focused spectrum
            foc_spec = banks_foc.getSpectrum(igrp)
            ispecs = ws.getIndicesFromDetectorIDs(list(foc_spec.getDetectorIDs()))
            for ispec in ispecs:
                # assume lambda dep. efficiency differs only by const scale factor between detectors on same bank
                # also assume that bg counts are subject to same efficiency
                scale = ws.readY(int(ispec)).sum() / ybank_sum
                ws.setY(int(ispec), scale * ybank)
                ws.setE(int(ispec), scale * ebank)
        mantid.DeleteWorkspace("grouped")

    def process_vanadium(self):
        # load empty and vanadium
        empty_ws = self.load_run(self.empty_runno, self.file_ext)
        self.van_ws = self.load_run(self.van_runno, self.file_ext)
        # create grouping file per bank
        bank_grouping_ws, _, ngroups = mantid.CreateGroupingWorkspace(
            InputWorkspace=self.van_ws, GroupDetectorsBy="bank", OutputWorkspace="bank_groups", EnableLogging=False
        )
        self._minus_workspaces(self.van_ws, empty_ws)
        self._replace_spectra_with_focussed_bank(self.van_ws, bank_grouping_ws, ngroups)
        # correct vanadium for absorption
        mantid.SetSample(
            self.van_ws,
            Geometry={"Shape": "CSG", "Value": self.sphere_shape},
            Material={"ChemicalFormula": "V0.95-Nb0.05", "SampleNumberDensity": 0.0722},
            EnableLogging=False,
        )
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target="Wavelength", EnableLogging=False)
        vanadium_transmission = mantid.SphericalAbsorption(
            InputWorkspace=self.van_ws, OutputWorkspace="vanadium_transmission", EnableLogging=False
        )
        self._divide_workspaces(self.van_ws, vanadium_transmission)
        mantid.DeleteWorkspace(vanadium_transmission, EnableLogging=False)
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target="TOF", EnableLogging=False)

    @staticmethod
    def flip_bank_1(ws):
        wsname = SXD.retrieve(ws).name()
        mantid.MoveInstrumentComponent(
            Workspace=wsname, ComponentName="bank1", RelativePosition=False, EnableLogging=False
        )  # move to origin
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", Y=1, Angle=37.5, EnableLogging=False)
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", X=1, Y=1, Angle=180, EnableLogging=False)
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", X=1, Angle=-37.5, EnableLogging=False)
        mantid.MoveInstrumentComponent(
            Workspace=wsname, ComponentName="bank1", X=0.136971, Z=-0.178505, RelativePosition=False, EnableLogging=False
        )

    @staticmethod
    def calibrate_sxd_panels(ws, peaks, save_dir, tol=0.15, idf_filename="SXD_Definition_19_1.xml", **kwargs):
        """
        Calibrate SXD panels panels and apply the calibration to the workspace and peaks workspace.
        This is an iterative process in which the global translation of the detectors with respect to the sample
        is optimized and then the relative positions of the banks are optimized independently, before optimizing both
        the position and rotation of each bank independently. Note L1 is not optimized.
        :param ws: workspace or name (or None) - if None then empty workspace is loaded and deleted
        :param peaks: peak table with a UB
        :param save_dir: for xml file containing calibration
        :param tol: for indexing (good to pick a relatively large tol as lots of peaks are required in each bank)
        :param idf_filename: name of .xml for SXD instrument defintion (used if workspace not loaded for a given run)
        :param **kwargs: key word arguments for SCDCalibratePanels
        :return: xml_path: path to detector calibration xml file which can be applied using apply_calibration_xml or in the class init
        """
        # overwrite default kwargs for SCDCalibrate panels
        default_kwargs = {
            "RecalculateUB": False,
            "CalibrateL1": False,
            "CalibrateBanks": True,
            "TuneSamplePosition": False,
            "SearchRadiusTransBank": 0.015,
            "SearchradiusRotXBank": 2,
            "SearchradiusRotYBank": 2,
            "SearchradiusRotZBank": 2,
        }
        kwargs = {**default_kwargs, **kwargs}
        tol_to_report = np.round(tol / 2, 3)  # use smaller tolerance to track improvement in calibration
        npeaks = SXD.retrieve(peaks).getNumberPeaks()
        if ws is not None:
            wsname = SXD.retrieve(ws).name()
        else:
            wsname = "empty"
            mantid.LoadEmptyInstrument(Filename=idf_filename, OutputWorkspace=ws, EnableLogging=False)
        peaks_name = SXD.retrieve(peaks).name()
        # report inital indexing
        nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False)
        logger.notice(f"Indexed initially = {nindexed}/{npeaks} peaks within {tol_to_report}")
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol, CommonUBForAll=False, EnableLogging=False)

        # optimize global shift in detector (equivalent to sample placement)
        initialUB = np.copy(SXD.retrieve(peaks_name).sample().getOrientedLattice().getUB())
        _, pos_table, *_ = mantid.OptimizeCrystalPlacement(
            PeaksWorkspace=peaks_name,
            ModifiedPeaksWorkspace="temp",
            AdjustSampleOffsets=True,
            OptimizeGoniometerTilt=True,
            MaxIndexingError=tol,
            MaxSamplePositionChangeMeters=kwargs["SearchRadiusTransBank"],
            EnableLogging=False,
        )
        x, y, z = pos_table.cell(0, 1), pos_table.cell(1, 1), pos_table.cell(2, 1)
        rotx, roty, rotz = pos_table.cell(3, 1), pos_table.cell(4, 1), pos_table.cell(5, 1)
        for ibank in range(1, 12):
            mantid.MoveInstrumentComponent(
                Workspace=wsname, ComponentName="bank" + str(ibank), RelativePosition=True, X=-x, Y=-y, Z=-z, EnableLogging=False
            )
            mantid.ApplyInstrumentToPeaks(
                InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name, EnableLogging=False
            )
        newUB = getR(rotx, [1, 0, 0]) @ getR(roty, [0, 1, 0]) @ getR(rotz, [0, 0, 1]) @ initialUB
        mantid.SetUB(peaks_name, UB=newUB, EnableLogging=False)
        # report progress
        nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False)
        logger.notice(f"Indexed after global instrument translation and rotation = {nindexed}/{npeaks} peaks within {tol_to_report}")
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol, CommonUBForAll=False, EnableLogging=False)
        # optimize position of each bank independently
        for ibank in range(1, 12):
            logger.notice(f"Optimizing bank position {ibank}")
            peaks_bank = peaks_name + "_bank"
            mantid.FilterPeaks(
                InputWorkspace=peaks_name,
                OutputWorkspace=peaks_bank,
                FilterVariable="RunNumber",
                BankName="bank" + str(ibank),
                EnableLogging=False,
            )
            # report inital indexing
            npeaks_bank = SXD.retrieve(peaks_bank).getNumberPeaks()
            nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks_bank, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False)
            logger.notice(f"Indexed before optimization = {nindexed}/{npeaks_bank} peaks within {tol_to_report}")
            mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol, CommonUBForAll=False, EnableLogging=False)
            peaks_bank_mod, pos_table, *_ = mantid.OptimizeCrystalPlacement(
                PeaksWorkspace=peaks_bank,
                ModifiedPeaksWorkspace="temp",
                AdjustSampleOffsets=True,
                OptimizeGoniometerTilt=False,
                MaxIndexingError=tol,
                MaxSamplePositionChangeMeters=kwargs["SearchRadiusTransBank"],
                EnableLogging=False,
            )
            nindexed, *_ = mantid.IndexPeaks(
                PeaksWorkspace=peaks_bank_mod, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False
            )
            # report progress
            logger.notice(f"Indexed after optimization = {nindexed}/{npeaks_bank} peaks within {tol_to_report}")
            x, y, z = pos_table.cell(0, 1), pos_table.cell(1, 1), pos_table.cell(2, 1)
            mantid.MoveInstrumentComponent(
                Workspace=wsname, ComponentName="bank" + str(ibank), RelativePosition=True, X=-x, Y=-y, Z=-z, EnableLogging=False
            )
            mantid.ApplyInstrumentToPeaks(
                InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name, EnableLogging=False
            )
        mantid.DeleteWorkspaces(WorkspaceList=[peaks_bank, "temp"], EnableLogging=False)

        # use SCD calibrate to refine detector orientation and save detcal
        # report initial indexing
        logger.notice("Optimizing bank positions and rotations")
        nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False)
        logger.notice(f"Indexed before optimization = {nindexed}/{npeaks} peaks within {tol_to_report}")
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol, CommonUBForAll=False, EnableLogging=False)
        # Optimize all panels independently
        detcal_path = path.join(save_dir, "_".join([peaks_name, "detcal"]))
        xml_path = detcal_path + ".xml"
        mantid.SCDCalibratePanels(
            PeakWorkspace=peaks_name,
            OutputWorkspace="sxd_calibration_ws",
            DetCalFilename=detcal_path + ".detcal",
            XmlFilename=xml_path,
            CSVFilename=detcal_path + ".csv",
            EnableLogging=False,
            **kwargs,
        )
        SXD.undo_calibration(wsname)
        mantid.LoadParameterFile(Workspace=wsname, Filename=xml_path, EnableLogging=False)
        mantid.ApplyInstrumentToPeaks(
            InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name, EnableLogging=False
        )
        # report final progress
        nindexed, *_ = mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol_to_report, CommonUBForAll=False, EnableLogging=False)
        logger.notice(f"Indexed after optimization = {nindexed}/{npeaks} peaks within {tol_to_report}")
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol, CommonUBForAll=False, EnableLogging=False)
        # cleanup
        mantid.DeleteWorkspaces(WorkspaceList=["CovarianceInfo", "FitInfoTable"], EnableLogging=False)
        if ws is None:
            mantid.DeleteWorkspace(wsname, EnableLogging=False)
        return xml_path

    @BaseSX.default_apply_to_all_runs
    def apply_calibration_xml(self, xml_path, idf_filename="SXD_Definition_19_1.xml", run=None):
        """
        Apply .xml calibration file to workspaces loaded (and peak workspaces if present) - subsequent runs loaded
        will automatically have the new calibration
        :param xml_path: path to xml cal file
        :param idf_filename:  name of .xml for SXD instrument defintion (used if workspace not loaded for a given run)
        """
        use_empty = False
        ws = self.get_ws_name(run)
        if ws is None:
            # need instrument ws to apply to peaks
            ws = "empty"
            mantid.LoadEmptyInstrument(Filename=idf_filename, OutputWorkspace=ws, EnableLogging=False)
            use_empty = True
        else:
            SXD.undo_calibration(ws)
        mantid.LoadParameterFile(Workspace=ws, Filename=xml_path, EnableLogging=False)
        for pk_type in PEAK_TYPE:
            pks = self.get_peaks_name(run, peak_type=pk_type, integration_type=None)
            if pks is not None:
                mantid.ApplyInstrumentToPeaks(InputWorkspace=pks, InstrumentWorkspace=ws, OutputWorkspace=pks, EnableLogging=False)
                for int_type in INTEGRATION_TYPE:
                    pks_int = self.get_peaks_name(run, peak_type=pk_type, integration_type=int_type)
                    if pks_int is not None:
                        mantid.ApplyInstrumentToPeaks(
                            InputWorkspace=pks_int, InstrumentWorkspace=ws, OutputWorkspace=pks_int, EnableLogging=False
                        )
        if use_empty:
            mantid.DeleteWorkspace(ws)

    @staticmethod
    def undo_calibration(ws, peaks_ws=None):
        mantid.ClearInstrumentParameters(Workspace=ws)  # reset workspace calibration
        mantid.LoadParameterFile(Workspace=ws, Filename="SXD_Parameters.xml", EnableLogging=False)
        if peaks_ws is not None:
            mantid.ApplyInstrumentToPeaks(
                InputWorkspace=peaks_ws, InstrumentWorkspace=ws, OutputWorkspace=SXD.retrieve(peaks_ws).name(), EnableLogging=False
            )

    @staticmethod
    def remove_peaks_on_detector_edge(peaks, nedge):
        # filter peaks on bank edge
        for col_name in ["Row", "Col"]:
            col = np.array(SXD.retrieve(peaks).column(col_name))
            iremove = np.where(np.logical_or(col < nedge, col > 63 - nedge))[0]
            mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove, EnableLogging=False)

    @staticmethod
    def find_sx_peaks(ws, out_peaks=None, **kwargs):
        wsname = SXD.retrieve(ws).name()
        if out_peaks is None:
            out_peaks = wsname + "_peaks"
        default_kwargs = {
            "NRows": 7,
            "NCols": 7,
            "GetNBinsFromBackToBackParams": True,
            "NFWHM": 6,
            "PeakFindingStrategy": "VarianceOverMean",
            "ThresholdVarianceOverMean": 2,
        }
        kwargs = {**default_kwargs, **kwargs}  # will overwrite default with provided if duplicate keys
        mantid.FindSXPeaksConvolve(InputWorkspace=wsname, PeaksWorkspace=out_peaks, **kwargs)
        return out_peaks

    @staticmethod
    def get_radius(pk, ws, ispec, scale=12):
        func = BaseSX.get_back_to_back_exponential_func(pk, ws, ispec)
        dtof = scale * func.fwhm()
        return BaseSX.convert_dTOF_to_dQ_for_peak(dtof, pk)

    @staticmethod
    def mask_detector_edges(ws):
        mantid.MaskBTP(Workspace=ws, Tube="Edges", EnableLogging=False)
        mantid.MaskBTP(Workspace=ws, Pixel="Edges", EnableLogging=False)

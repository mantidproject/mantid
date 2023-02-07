from typing import Sequence
import numpy as np
import mantid.simpleapi as mantid
from Diffraction.single_crystal.base_sx import BaseSX, PEAK_TYPE, INTEGRATION_TYPE
from mantid.api import AnalysisDataService as ADS
from FindGoniometerFromUB import getR
from os import path


class SXD(BaseSX):
    def __init__(self, vanadium_runno=None, empty_runno=None, detcal_path=None):
        self.empty_runno = empty_runno
        self.detcal_path = detcal_path
        super().__init__(vanadium_runno)
        self.grp_ws = None  # banks grouping workspace
        self.ngrp = None  # no. groups (i.e. number of banks)
        self.sphere_shape = """<sphere id="sphere">
                               <centre x="0.0"  y="0.0" z="0.0" />
                               <radius val="0.003"/>
                               </sphere>"""  # sphere radius 3mm  - used for vanadium and NaCl

    def process_data(self, runs: Sequence[str], gonio_angles=None):
        for irun, run in enumerate(runs):
            wsname = self.load_run(run)
            self._minus_workspaces(wsname, self.empty_ws)
            self._divide_workspaces(wsname, self.van_ws)
            # set goniometer
            if self.gonio_axes is not None:
                # gonio_angles are a list of motor strings (same for all runs)
                if isinstance(gonio_angles[0], str):
                    self._set_goniometer_on_ws(wsname, gonio_angles)
                else:
                    # gonio_angles is a list of individual or tuple motor angles for each run
                    self._set_goniometer_on_ws(wsname, gonio_angles[irun])
            # correct for empty counts and normalise by vanadium
            self._minus_workspaces(wsname, self.empty_ws)
            self._divide_workspaces(wsname, self.van_ws)
            # set sample (must be done after gonio to rotate shape) and correct for attenuation
            if self.sample_dict is not None:
                mantid.SetSample(wsname, **self.sample_dict)
                mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength")
                if "<sphere" in ADS.retrieve(wsname).sample().getShape().getShapeXML():
                    transmission = mantid.SphericalAbsorption(InputWorkspace=wsname, OutputWorkspace="transmission")
                else:
                    transmission = mantid.MonteCarloAbsorption(
                        InputWorkspace=wsname, OutputWorkspace="transmission", EventsPerPoint=self.n_mcevents
                    )
                self._divide_workspaces(wsname, transmission)
                mantid.DeleteWorkspace(transmission)
            # save results in dictionary
            self.runs[str(run)] = {"ws": wsname}

    def load_run(self, runno):
        wsname = "SXD" + str(runno)
        mantid.Load(Filename=wsname + ".raw", OutputWorkspace=wsname)
        if self.detcal_path is not None:
            mantid.LoadParameterFile(Workspace=wsname, Filename=self.detcal_path)
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=700, XMax=18800)
        mantid.NormaliseByCurrent(InputWorkspace=wsname, OutputWorkspace=wsname)
        return wsname

    def process_vanadium(self):
        # load empty and vanadium
        self.empty_ws = self.load_run(self.empty_runno)
        self.van_ws = self.load_run(self.van_runno)
        # create grouping file per bank
        self.grp_ws, _, self.ngrp = mantid.CreateGroupingWorkspace(
            InputWorkspace=self.van_ws, GroupDetectorsBy="bank", OutputWorkspace="bank_groups"
        )
        for wsname in [self.empty_ws, self.van_ws]:
            # important to focus in TOF not d-spacing!
            banks_foc = mantid.GroupDetectors(
                InputWorkspace=wsname, OutputWorkspace="grouped", IgnoreGroupNumber=False, CopyGroupingFromWorkspace=self.grp_ws
            )
            mantid.SmoothData(InputWorkspace=wsname, OutputWorkspace=wsname, NPoints=15)
            ws = SXD.retrieve(wsname)
            for igrp in range(self.ngrp):
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
        # subtract empty from vanadium
        self._minus_workspaces(self.van_ws, self.empty_ws)
        # correct vanadium for absorption
        mantid.SetSample(
            self.van_ws,
            Geometry={"Shape": "CSG", "Value": self.sphere_shape},
            Material={"ChemicalFormula": "V0.95-Nb0.05", "SampleNumberDensity": 0.0722},
        )
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target="Wavelength")
        vanadium_transmission = mantid.SphericalAbsorption(InputWorkspace=self.van_ws, OutputWorkspace="vanadium_transmission")
        self._divide_workspaces(self.van_ws, vanadium_transmission)
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target="TOF")

    @staticmethod
    def flip_bank_1(ws):
        wsname = SXD.retrieve(ws).name()
        mantid.MoveInstrumentComponent(Workspace=wsname, ComponentName="bank1", RelativePosition=False)  # move to origin
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", Y=1, Angle=37.5)
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", X=1, Y=1, Angle=180)
        mantid.RotateInstrumentComponent(Workspace=wsname, ComponentName="bank1", X=1, Angle=-37.5)
        mantid.MoveInstrumentComponent(Workspace=wsname, ComponentName="bank1", X=0.136971, Z=-0.178505, RelativePosition=False)

    @staticmethod
    def calibrate_sxd_panels(ws, peaks, save_dir, tol=0.15, maxShiftInMeters=0.025):
        """
        Calibrate SXD panels and apply the calibration to th
        :param ws: workspace or name (or None) - if None then empty workspace is loaded
        :param peaks: peak table with a UB
        :param save_dir: for xml file containing calibration
        :param tol: for indexing
        :param maxShiftInMeters: for panel optimisation
        :return: xml_path
        """
        if ws is not None:
            wsname = SXD.retrieve(ws).name()
        else:
            wsname = "empty"
            mantid.LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace=wsname)
        peaks_name = SXD.retrieve(peaks).name()
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol)
        initialUB = np.copy(SXD.retrieve(peaks_name).sample().getOrientedLattice().getUB())
        # optimize global shift in detector (equivalent to sample placement)
        _, pos_table, *_ = mantid.OptimizeCrystalPlacement(
            PeaksWorkspace=peaks_name,
            ModifiedPeaksWorkspace="temp",
            AdjustSampleOffsets=True,
            OptimizeGoniometerTilt=True,
            MaxIndexingError=tol,
            MaxSamplePositionChangeMeters=maxShiftInMeters,
            EnableLogging=False,
        )
        x, y, z = pos_table.cell(0, 1), pos_table.cell(1, 1), pos_table.cell(2, 1)
        rotx, roty, rotz = pos_table.cell(3, 1), pos_table.cell(4, 1), pos_table.cell(5, 1)
        for ibank in range(1, 12):
            mantid.MoveInstrumentComponent(
                Workspace=wsname, ComponentName="bank" + str(ibank), RelativePosition=True, X=-x, Y=-y, Z=-z, EnableLogging=False
            )
            mantid.ApplyInstrumentToPeaks(InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name)
        newUB = getR(rotx, [1, 0, 0]) @ getR(roty, [0, 1, 0]) @ getR(rotz, [0, 0, 1]) @ initialUB
        mantid.SetUB(peaks_name, UB=newUB)
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol)
        # optimize position of each bank independently
        for ibank in range(1, 12):
            mantid.FilterPeaks(
                InputWorkspace=peaks_name, OutputWorkspace=peaks + "_bank", FilterVariable="RunNumber", BankName="bank" + str(ibank)
            )
            _, pos_table, *_ = mantid.OptimizeCrystalPlacement(
                PeaksWorkspace=peaks_name,
                ModifiedPeaksWorkspace="temp",
                AdjustSampleOffsets=True,
                OptimizeGoniometerTilt=False,
                MaxIndexingError=tol,
                MaxSamplePositionChangeMeters=maxShiftInMeters,
                EnableLogging=False,
            )
            x, y, z = pos_table.cell(0, 1), pos_table.cell(1, 1), pos_table.cell(2, 1)
            mantid.MoveInstrumentComponent(
                Workspace=wsname, ComponentName="bank" + str(ibank), RelativePosition=True, X=-x, Y=-y, Z=-z, EnableLogging=False
            )
            mantid.ApplyInstrumentToPeaks(InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name)
        mantid.DeleteWorkspace(peaks + "_bank")
        mantid.DeleteWorkspace("temp")
        # use SCD calibrate to refine detector orientation and save detcal
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol)
        detcal_path = path.join(save_dir, "_".join([peaks, "detcal"]))
        xml_path = detcal_path + ".xml"
        mantid.SCDCalibratePanels(
            PeakWorkspace=peaks_name,
            OutputWorkspace="sxd_calibration_ws",
            RecalculateUB=False,
            CalibrateL1=False,
            CalibrateBanks=True,
            TuneSamplePosition=False,
            SearchRadiusTransBank=maxShiftInMeters / 2,  # should be much reduced
            SearchradiusRotXBank=2,
            SearchradiusRotYBank=2,
            SearchradiusRotZBank=2,
            DetCalFilename=detcal_path + ".detcal",
            XmlFilename=xml_path,
            CSVFilename=detcal_path + ".csv",
            EnableLogging=False,
        )
        mantid.LoadInstrument(Workspace=wsname, InstrumentName="SXD", RewriteSpectraMap=False)  # reset instrument if already calibrated
        mantid.LoadParameterFile(Workspace=wsname, Filename=xml_path)
        mantid.ApplyInstrumentToPeaks(InputWorkspace=peaks_name, InstrumentWorkspace=wsname, OutputWorkspace=peaks_name)
        mantid.IndexPeaks(PeaksWorkspace=peaks_name, Tolerance=tol)
        if ws is None:
            mantid.DeleteWorkspace(wsname)

        return xml_path

    def apply_calibration_xml(self, xml_path):
        """
        Apply .xml calibration file to workspaces loaded (and peak workspaces if present) - subsequent runs loaded
        will automatically have the new calibration
        :param xml_path: path to xml cal file
        """
        use_empty = False
        for run in self.runs.keys():
            ws = self.get_ws(run)
            if ws is None:
                # need instrument ws to apply to peaks
                ws = "empty"
                mantid.LoadEmptyInstrument(InstrumentName="SXD", OutputWorkspace=ws)
                use_empty = True
            else:
                mantid.LoadInstrument(Workspace=ws, InstrumentName="SXD", RewriteSpectraMap=False)  # reset instrument if already calibrated
            mantid.LoadParameterFile(Workspace=ws, Filename=xml_path)
            for pk_type in PEAK_TYPE:
                pks = self.get_peaks(run, peak_type=pk_type, integration_type=None)
                if pks is not None:
                    mantid.ApplyInstrumentToPeaks(InputWorkspace=pks, InstrumentWorkspace=ws, OutputWorkspace=pks)
                    for int_type in INTEGRATION_TYPE:
                        pks_int = self.get_peaks(run, peak_type=pk_type, integration_type=int_type)
                        if pks_int is not None:
                            mantid.ApplyInstrumentToPeaks(InputWorkspace=pks_int, InstrumentWorkspace=ws, OutputWorkspace=pks_int)
        if use_empty:
            mantid.DeleteWorkspace(ws)

    @staticmethod
    def filter_peaks_on_detector_edge(peaks, nedge):
        # filter peaks on bank edge
        for col_name in ["Row", "Col"]:
            col = np.array(SXD.retrieve(peaks).column(col_name))
            iremove = np.where(np.logical_or(col < nedge, col > 63 - nedge))[0]
            mantid.DeleteTableRows(TableWorkspace=peaks, Rows=iremove)

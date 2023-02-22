from typing import Sequence
import numpy as np
import mantid.simpleapi as mantid
from Diffraction.single_crystal.base_sx import BaseSX
from mantid.api import AnalysisDataService as ADS


class WishSX(BaseSX):
    def __init__(self, vanadium_runno=None):
        super().__init__(vanadium_runno)
        self.grp_ws = None  # banks grouping workspace
        self.ngrp = None  # no. groups (i.e. number of banks)
        self.sphere_shape = """<sphere id="sphere">
                               <centre x="0.0"  y="0.0" z="0.0" />
                               <radius val="0.0025"/>
                               </sphere>"""  # sphere radius 2.5mm  - used for vanadium and NaCl

    def process_data(self, runs: Sequence[str], *args):
        gonio_angles = args
        if len(gonio_angles) != len(self.gonio_axes):
            return
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
                mantid.SetSample(wsname, EnableLogging=False, **self.sample_dict)
                mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength", EnableLogging=False)
                if "<sphere" in ADS.retrieve(wsname).sample().getShape().getShapeXML():
                    transmission = mantid.SphericalAbsorption(InputWorkspace=wsname, OutputWorkspace="transmission", EnableLogging=False)
                else:
                    transmission = mantid.MonteCarloAbsorption(
                        InputWorkspace=wsname, OutputWorkspace="transmission", EventsPerPoint=self.n_mcevents, EnableLogging=False
                    )
                self._divide_workspaces(wsname, transmission)
                mantid.DeleteWorkspace(transmission)
            # save results in dictionary
            self.set_ws(run, wsname)
        return wsname

    @staticmethod
    def mask_detector_edges(ws):
        # mask pixels on ends of tubes
        mantid.MaskBTP(Workspace=ws, Pixel="1-16,496-512")
        # only mask tubes on panels with edge facing beam in/out
        mantid.MaskBTP(Workspace=ws, Bank="5-6", Tube="152")
        mantid.MaskBTP(Workspace=ws, Bank="1,10", Tube="1")

    def process_vanadium(self, npoints=301):
        # vanadium
        self.van_ws = self.load_run(self.van_runno)
        mantid.ConvertUnits(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Target="Wavelength")
        mantid.SmoothNeighbours(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, Radius=3)
        mantid.SmoothData(InputWorkspace=self.van_ws, OutputWorkspace=self.van_ws, NPoints=npoints)
        # correct vanadium for absorption
        mantid.SetSample(
            self.van_ws,
            Geometry={"Shape": "CSG", "Value": self.sphere_shape},
            Material={"ChemicalFormula": "V0.95-Nb0.05", "SampleNumberDensity": 0.0722},
        )
        vanadium_transmission = mantid.SphericalAbsorption(InputWorkspace=self.van_ws, OutputWorkspace="vanadium_transmission")
        self._divide_workspaces(self.van_ws, vanadium_transmission)
        mantid.DeleteWorkspace(vanadium_transmission)

    @staticmethod
    def load_run(runno):
        wsname = "WISH000" + str(runno)
        ws, mon = mantid.LoadRaw(Filename=wsname + ".raw", OutputWorkspace=wsname, LoadMonitors="Separate")
        mon_name = mon.name()
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=6000, XMax=99000)
        # replace empty bin errors with 1 count
        mantid.SetUncertainties(InputWorkspace=wsname, OutputWorkspace=wsname, SetError="oneIfZero")
        mantid.ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength")
        mantid.ConvertUnits(InputWorkspace=mon_name, OutputWorkspace=mon_name, Target="Wavelength")
        mantid.NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspaceIndex=3, MonitorWorkspace=mon_name)
        mantid.ReplaceSpecialValues(InputWorkspace=wsname, OutputWorkspace=wsname, NaNValue=0, InfinityValue=0)
        mantid.CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=0.8)
        mantid.NormaliseByCurrent(InputWorkspace=wsname, OutputWorkspace=wsname)
        return wsname

    @staticmethod
    def find_sx_peaks(ws, bg, out_pk_wsname="peaks"):
        ws = WishSX.retrieve(ws)
        # get unit to convert back to after peaks found
        xunit = WishSX.get_xunit(ws)
        if not ws.getXDimension().name == "TOF":
            ws = mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target="TOF")  # FindSXPeaks requires TOF
        # extract y data (to use to determine to detemrine threshold)
        mantid.FindSXPeaks(
            InputWorkspace=ws,
            PeakFindingStrategy="AllPeaks",
            AbsoluteBackground=bg,
            ResolutionStrategy="AbsoluteResolution",
            XResolution=500,
            PhiResolution=3,
            TwoThetaResolution=2,
            OutputWorkspace=out_pk_wsname,
        )
        WishSX.remove_peaks_on_edge(out_pk_wsname)
        if not ws.getXDimension().name == "TOF":
            mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws.name(), Target=xunit)
        return out_pk_wsname

    @staticmethod
    def find_sx_peaks_bankwise(wsname, out_pk_wsname=None, nstd=8, nbunch=3, lambda_min=1.25, lambda_max=7):
        ws = mantid.Rebunch(InputWorkspace=wsname, OutputWorkspace=wsname + "_rb", NBunch=nbunch)
        WishSX.mask_detector_edges(ws)
        # mask additional tubes at detector edge
        mantid.MaskBTP(Workspace=ws, Bank="5-6", Tube="76,151-152")
        mantid.MaskBTP(Workspace=ws, Bank="1,10", Tube="1-2,77")
        # Crop in wavelength
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="Wavelength")
        mantid.CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=lambda_min, XMax=lambda_max)
        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="TOF")  # FindSXPeaks requries TOF
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
            pks_bank = mantid.FindSXPeaks(
                InputWorkspace=ws,
                PeakFindingStrategy="AllPeaks",
                AbsoluteBackground=cutoff,
                ResolutionStrategy="AbsoluteResolution",
                XResolution=500,
                PhiResolution=3,
                TwoThetaResolution=2,
                StartWorkspaceIndex=int(ispec_min),
                EndWorkspaceIndex=int(ispec_max) - 1,
            )
            peaks = mantid.CombinePeaksWorkspaces(
                LHSWorkspace=pks_bank, RHSWorkspace=peaks, OutputWorkspace=out_pk_wsname, CombineMatchingPeaks=True, Tolerance=0.05
            )
        mantid.DeleteWorkspace(pks_bank)
        mantid.DeleteWorkspace(ws)
        return out_pk_wsname

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

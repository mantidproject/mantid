# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import systemtesting
import tempfile
import shutil
import os
import numpy as np
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (
    AddPeak,
    CalculateUMatrix,
    CreatePeaksWorkspace,
    FindUBUsingFFT,
    IndexPeaks,
    Load,
    LoadEmptyInstrument,
    LoadNexus,
    OptimizeLatticeForCellType,
    SelectCellOfType,
)
from Diffraction.single_crystal.sxd import SXD
from Diffraction.single_crystal.base_sx import PEAK_TYPE, INTEGRATION_TYPE


class SXDPeakSearchAndFindUBUsingFFT(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        ws = Load(Filename="SXD23767.raw", LoadMonitors="Exclude")
        self.peaks = SXD.find_sx_peaks(ws, ThresholdVarianceOverMean=1.5)
        FindUBUsingFFT(PeaksWorkspace=self.peaks, MinD=1, MaxD=10, Tolerance=0.15)
        SelectCellOfType(PeaksWorkspace=self.peaks, CellType="Cubic", Centering="F", Apply=True)
        OptimizeLatticeForCellType(PeaksWorkspace=self.peaks, CellType="Cubic", Apply=True)
        self.nindexed, *_ = IndexPeaks(PeaksWorkspace=self.peaks, Tolerance=0.1, CommonUBForAll=True)

    def validate(self):
        self.assertEqual(171, self.nindexed)
        latt = SXD.retrieve(self.peaks).sample().getOrientedLattice()
        a, alpha = 5.6533, 90  # published value for NaCl is a=6.6402 but the detector positions haven't been calibrated
        self.assertAlmostEqual(a, latt.a(), delta=1e-5)
        self.assertAlmostEqual(a, latt.b(), delta=1e-5)
        self.assertAlmostEqual(a, latt.c(), delta=1e-5)
        self.assertAlmostEqual(alpha, latt.alpha(), delta=1e-10)
        self.assertAlmostEqual(alpha, latt.beta(), delta=1e-10)
        self.assertAlmostEqual(alpha, latt.gamma(), delta=1e-10)


class SXDDetectorCalibration(systemtesting.MantidSystemTest):
    def setUp(self):
        self._temp_dir = tempfile.mkdtemp()

    def cleanup(self):
        ADS.clear()
        shutil.rmtree(self._temp_dir)

    def runTest(self):
        self.peaks = LoadNexus(Filename="SXD23767_found_peaks.nxs", OutputWorkspace="peaks")
        SXD.remove_peaks_on_detector_edge(self.peaks, 2)

        # force lattice parameters to equal published values
        a, alpha = 5.6402, 90
        CalculateUMatrix(PeaksWorkspace=self.peaks, a=a, b=a, c=a, alpha=alpha, beta=alpha, gamma=alpha)
        # load an empty workspace as MoveCOmpoennt etc. only work on Matrix workspaces
        self.ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty")

        self.xml_path = SXD.calibrate_sxd_panels(self.ws, self.peaks, self._temp_dir, tol=0.25, SearchRadiusTransBank=0.025)

        self.nindexed, *_ = IndexPeaks(PeaksWorkspace=self.peaks, Tolerance=0.1, CommonUBForAll=True)

    def validate(self):
        # test seems to vary on OS - so just check more peaks indexed and components have been moved
        self.assertGreaterThan(self.nindexed, 232)
        # check xml file exists
        self.assertTrue(os.path.exists(self.xml_path))
        # check calibration has been applied to both MatrixWorkspace and peaks
        for ws in [self.peaks, self.ws]:
            self.assertNotEqual(ws.getInstrument().getComponentByName("bank1").getPos()[1], 0)
            self.assertNotEqual(ws.getInstrument().getComponentByName("bank2").getPos()[2], 0)
        return True


class SXDProcessVanadium(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        sxd = SXD(vanadium_runno=23779, empty_runno=23768)
        sxd.process_vanadium()
        self.van = sxd.van_ws

    def validate(self):
        self.checkInstrument = False
        return self.van, "SXD23779_processed_vanadium.nxs"


class SXDProcessSampleData(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        sxd = SXD(vanadium_runno=23769, empty_runno=23768, scale_integrated=False)
        sxd.van_ws = LoadNexus(Filename="SXD23779_processed_vanadium.nxs", OutputWorkspace="SXD23779_vanadium")
        sxd.set_sample(
            Geometry={"Shape": "CSG", "Value": sxd.sphere_shape},
            Material={"ChemicalFormula": "Na Cl", "SampleNumberDensity": 0.0223, "NumberDensityUnit": "Atoms"},
        )
        sxd.set_goniometer_axes([0, 1, 0, 1])  # ccw rotation around vertical
        runno = 23767
        sxd.process_data([runno], [0])
        self.ws = sxd.get_ws_name(runno)

    def validate(self):
        self.checkInstrument = False
        return self.ws, "SXD23767_processed.nxs"


class SXDIntegrateData(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        sxd = SXD(vanadium_runno=23769, empty_runno=23768)
        # load data and convert to Qlab
        ws = LoadNexus(Filename="SXD23767_processed.nxs", OutputWorkspace="SXD23767_processed")
        runno = 23767
        sxd.set_ws(runno, ws)
        sxd.convert_to_MD(run=runno)
        # load peaks to integrate
        peaks = LoadNexus(Filename="SXD23767_found_peaks.nxs", OutputWorkspace="SXD23767_found")
        sxd.set_peaks(runno, peaks, PEAK_TYPE.FOUND)
        sxd.set_sample(
            Geometry={"Shape": "CSG", "Value": sxd.sphere_shape}, Material={"ChemicalFormula": "Na Cl", "SampleNumberDensity": 0.0223}
        )
        sxd.set_goniometer_axes([0, 1, 0, 1])  # ccw rotation around vertical
        sxd.integrate_data(INTEGRATION_TYPE.MD_OPTIMAL_RADIUS, PEAK_TYPE.FOUND, scale=12)
        self.integrated_peaks = sxd.get_peaks_name(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.MD_OPTIMAL_RADIUS)

    def validate(self):
        self.tolerance = 1e-8
        self.tolerance_is_rel_err = True
        return self.integrated_peaks, "SXD23767_found_peaks_integrated.nxs"


class SXDIntegrateDataShoebox(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        sxd = SXD(vanadium_runno=23769, empty_runno=23768)
        runno = 23767
        _load_data_and_add_peaks(sxd, runno)

        # integrate
        sxd.integrate_data(
            INTEGRATION_TYPE.SHOEBOX,
            PEAK_TYPE.FOUND,
            GetNBinsFromBackToBackParams=True,
            WeakPeakThreshold=20.0,
            LorentzCorrection=False,
            WeakPeakStrategy="NearestStrongPeak",
        )
        self.integrated_peaks = sxd.get_peaks(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.SHOEBOX)

    def validate(self):
        intens_over_sigma = [0.0, 12.749, 11.941, 136.535, 0.0]
        self.assertTrue(np.allclose(self.integrated_peaks.column("Intens/SigInt"), intens_over_sigma, atol=1e-2))


class SXDIntegrateData1DProfile(systemtesting.MantidSystemTest):
    def cleanup(self):
        ADS.clear()

    def runTest(self):
        sxd = SXD(vanadium_runno=23769, empty_runno=23768)
        runno = 23767
        _load_data_and_add_peaks(sxd, runno)

        # integrate
        sxd.integrate_data(
            INTEGRATION_TYPE.PROFILE,
            PEAK_TYPE.FOUND,
            GetNBinsFromBackToBackParams=True,
            NFWHM=6,
            CostFunction="RSq",
            FixPeakParameters="A",
            FractionalChangeDSpacing=0.025,
            IntegrateIfOnEdge=False,
        )
        self.integrated_peaks = sxd.get_peaks(runno, PEAK_TYPE.FOUND, INTEGRATION_TYPE.PROFILE)

    def validate(self):
        intens_over_sigma = [0.0, 17.863, 13.395, 135.109, 0.0]
        self.assertTrue(np.allclose(self.integrated_peaks.column("Intens/SigInt"), intens_over_sigma, atol=1e-2))


def _load_data_and_add_peaks(sxd, runno):
    ws = LoadNexus(Filename="SXD23767_processed.nxs", OutputWorkspace="SXD23767_processed")
    sxd.set_ws(runno, ws)
    # create peak table
    peaks = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace="peaks")
    AddPeak(PeaksWorkspace="peaks", RunWorkspace=ws, TOF=19166.422281671705, DetectorID=20009)
    AddPeak(PeaksWorkspace="peaks", RunWorkspace=ws, TOF=1582.4728457415549, DetectorID=25111)
    AddPeak(PeaksWorkspace="peaks", RunWorkspace=ws, TOF=1734.9661945986509, DetectorID=28271)
    AddPeak(PeaksWorkspace="peaks", RunWorkspace=ws, TOF=5241.7309709929505, DetectorID=1380)
    AddPeak(PeaksWorkspace="peaks", RunWorkspace=ws, TOF=1665.2648421691604, DetectorID=8176)
    sxd.set_peaks(runno, peaks, PEAK_TYPE.FOUND)

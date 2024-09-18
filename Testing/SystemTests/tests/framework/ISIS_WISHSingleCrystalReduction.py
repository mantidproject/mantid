# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from systemtesting import MantidSystemTest

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (
    ConvertUnits,
    LoadRaw,
    FilterPeaks,
    PredictPeaks,
    SetUB,
    SaveIsawPeaks,
    MaskBTP,
    LoadEmptyInstrument,
    AddPeak,
    CreatePeaksWorkspace,
    CreateMDWorkspace,
    IntegratePeaksMD,
    CropWorkspace,
    NormaliseByCurrent,
    ReplaceSpecialValues,
    NormaliseToMonitor,
    LoadIsawUB,
    Divide,
    LoadNexus,
    RebinToWorkspace,
    ConvertToDiffractionMDWorkspace,
    LoadMD,
    PredictFractionalPeaks,
    CombinePeaksWorkspaces,
    SaveReflections,
    BinMD,
    ConvertMDHistoToMatrixWorkspace,
    ExtractSpectra,
    SortPeaksWorkspace,
)
from Diffraction.wish.wishSX import WishSX
from mantid import config
import numpy as np
import os
from SaveReflections import num_modulation_vectors


class WISHSingleCrystalPeakPredictionTest(MantidSystemTest):
    """
    At the time of writing WISH users rely quite heavily on the PredictPeaks
    algorithm. As WISH has tubes rather than rectangular detectors sometimes
    peaks fall between the gaps in the tubes.

    Here we check that PredictPeaks works on a real WISH dataset & UB. This also
    includes an example of a peak whose center is predicted to fall between two
    tubes.
    """

    def requiredFiles(self):
        return ["WISHPredictedSingleCrystalPeaks.nxs"]

    def cleanup(self):
        ADS.clear()
        try:
            os.remove(self._peaks_file)
        except:
            pass

    def runTest(self):
        ws = LoadEmptyInstrument(InstrumentName="WISH")
        UB = np.array([[-0.00601763, 0.07397297, 0.05865706], [0.05373321, 0.050198, -0.05651455], [-0.07822144, 0.0295911, -0.04489172]])

        SetUB(ws, UB=UB)

        self._peaks = PredictPeaks(ws, WavelengthMin=0.1, WavelengthMax=100, OutputWorkspace="peaks")
        # We specifically want to check peak -5 -1 -7 exists, so filter for it
        self._filtered = FilterPeaks(self._peaks, "h^2+k^2+l^2", 75, "=", OutputWorkspace="filtered")

        SaveIsawPeaks(self._peaks, Filename="WISHSXReductionPeaksTest.peaks")

    def validate(self):
        self.assertEqual(self._peaks.rowCount(), 536)
        self.assertEqual(self._filtered.rowCount(), 7)

        # The peak at [-5 -1 -7] is known to fall between the gaps of WISH's tubes
        # Specifically check this one is predicted to exist because past bugs have
        # been found in the ray tracing.
        BasicPeak = namedtuple("Peak", ("DetID", "BankName", "h", "k", "l"))
        expected = BasicPeak(DetID=9202086, BankName="WISHpanel09", h=-5.0, k=-1.0, l=-7.0)
        expected_peak_found = False
        peak_count = self._filtered.rowCount()
        for i in range(peak_count):  # iterate of the table representation of the PeaksWorkspace
            peak_row = self._filtered.row(i)
            peak = BasicPeak(**{k: peak_row[k] for k in BasicPeak._fields})
            if peak == expected:
                expected_peak_found = True
                break
        self.assertTrue(expected_peak_found, msg="Peak at {} expected but it was not found".format(expected))
        self._peaks_file = os.path.join(config["defaultsave.directory"], "WISHSXReductionPeaksTest.peaks")
        self.assertTrue(os.path.isfile(self._peaks_file))

        return self._peaks.name(), "WISHPredictedSingleCrystalPeaks.nxs"


class WISHPeakIntegrationRespectsMaskingTest(MantidSystemTest):
    """
    This tests that IntegratePeaksMD correctly ignores peaks at tube ends and that a custom masking for tubes
    adjacent to the beam in and out is respected by IntegratePeaksMD
    """

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        # Load Empty Instrument
        ws = LoadEmptyInstrument(InstrumentName="WISH", OutputWorkspace="WISH")
        axis = ws.getAxis(0)
        axis.setUnit("TOF")  # need this to add peak to table
        # CreatePeaksWorkspace with peaks in specific detectors
        peaks = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace="peaks")
        AddPeak(
            PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000, DetectorID=1707204, Height=521, BinCount=0
        )  # pixel in first tube in panel 1
        AddPeak(
            PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000, DetectorID=1400510, Height=1, BinCount=0
        )  # pixel at top of a central tube in panel 1
        AddPeak(
            PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000, DetectorID=1408202, Height=598, BinCount=0
        )  # pixel in middle of bank 1 (not near edge)
        AddPeak(
            PeaksWorkspace=peaks, RunWorkspace=ws, TOF=20000, DetectorID=1100173, Height=640, BinCount=0
        )  # pixel in last tube of panel 1 (next to panel 2)
        # create dummy MD workspace for integration (don't need data as checking peak shape)
        MD = CreateMDWorkspace(
            Dimensions="3",
            Extents="-1,1,-1,1,-1,1",
            Names="Q_lab_x,Q_lab_y,Q_lab_z",
            Units="U,U,U",
            Frames="QLab,QLab,QLab",
            SplitInto="2",
            SplitThreshold="50",
        )
        # Integrate peaks masking all pixels at tube end (built into IntegratePeaksMD)
        self._peaks_pixels = IntegratePeaksMD(
            InputWorkspace=MD,
            PeakRadius="0.02",
            PeaksWorkspace=peaks,
            IntegrateIfOnEdge=False,
            OutputWorkspace="peaks_pixels",
            MaskEdgeTubes=False,
        )
        # Apply masking to specific tubes next to beam in/out (subset of all edge tubes) and integrate again
        MaskBTP(Workspace="peaks", Bank="5-6", Tube="152")
        MaskBTP(Workspace="peaks", Bank="1,10", Tube="1")
        self._peaks_pixels_beamTubes = IntegratePeaksMD(
            InputWorkspace="MD",
            PeakRadius="0.02",
            PeaksWorkspace=peaks,
            IntegrateIfOnEdge=False,
            OutputWorkspace="peaks_pixels_beamTubes",
            MaskEdgeTubes=False,
        )
        # Integrate masking all edge tubes
        self._peaks_pixels_edgeTubes = IntegratePeaksMD(
            InputWorkspace="MD",
            PeakRadius="0.02",
            PeaksWorkspace="peaks",
            IntegrateIfOnEdge=False,
            OutputWorkspace="peaks_pixels_edgeTubes",
            MaskEdgeTubes=True,
        )

    def validate(self):
        # test which peaks were not integrated due to being on edge (i.e. shape = 'none')

        self.assertListEqual([pk.getPeakShape().shapeName() for pk in self._peaks_pixels], ["spherical", "none", "spherical", "spherical"])
        self.assertListEqual(
            [pk.getPeakShape().shapeName() for pk in self._peaks_pixels_beamTubes], ["none", "none", "spherical", "spherical"]
        )
        self.assertListEqual([pk.getPeakShape().shapeName() for pk in self._peaks_pixels_edgeTubes], ["none", "none", "spherical", "none"])


class WISHProcessVanadiumForNormalisationTest(MantidSystemTest):
    """
    This tests the processing of the vanadium run used to correct for detector efficiency
    """

    @classmethod
    def setUp(cls):
        cls._data_dirs = config.getDataSearchDirs()
        config.appendDataSearchSubDir(os.path.join("WISH", "input", "11_4", ""))

    @classmethod
    def tearDown(cls):
        config.setDataSearchDirs(cls._data_dirs)

    def requiredMemoryMB(self):
        return 6000

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        wish = WishSX(vanadium_runno=19612, file_ext=".nxs")  # load .nxs that only includes spectra nums 1-19461
        wish.process_vanadium()
        # select subset of spectra for comparision
        ExtractSpectra(InputWorkspace="WISH00019612", OutputWorkspace="WISH00019612_spec", WorkspaceIndexList="500,2500,10000")

    def validate(self):
        return "WISH00019612_spec", "WISH00019612_processed_van_spectra.nxs"


class WISHFindPeaksAndIntegrateUsingClassMethodsTest(MantidSystemTest):
    """
    This tests the processing of the vanadium run used to correct for detector efficiency
    """

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        ws = load_data_and_normalise("WISH00038237.raw", spectrumMax=38661)  # bank 1 & 2
        # find peaks
        peaks = WishSX.find_sx_peaks(ws, nstd=8)
        WishSX.remove_peaks_on_detector_edge(peaks, nedge=16, ntubes=2)
        # convert data to Q for integration
        WishSX.mask_detector_edges(ws, nedge=16, ntubes=2)
        wsMD = WishSX.convert_ws_to_MD(ws, frame="Q (lab frame)")
        # integrate - needs instance
        intPeaksMDArgs = {"ellipsoid": True, "fixQAxis": True, "fixMajorAxisLength": True, "useCentroid": True, "IntegrateIfOnEdge": True}
        self.peaks_int = WishSX().integrate_peaks_MD_optimal_radius(wsMD, peaks, peaks + "_int", ws=ws, **intPeaksMDArgs)
        self.peaks_int = SortPeaksWorkspace(
            InputWorkspace=self.peaks_int, OutputWorkspace=self.peaks_int.name(), ColumnNameToSortBy="DetID", SortAscending=False
        )

    def validate(self):
        self.assertEqual(self.peaks_int.getNumberPeaks(), 3)
        pk = self.peaks_int.getPeak(0)
        self.assertAlmostEqual(pk.getIntensity() / pk.getSigmaIntensity(), 8.3611, delta=1e-4)


class WISHNormaliseDataAndCreateMDWorkspaceTest(MantidSystemTest):
    """
    This tests the loading and normalisation of data, incl. correction for detector efficiency using vanadium and
    creation of MD workspace
    """

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        # Load processed vanadium for normalisation (bank 1)
        van = LoadNexus(Filename="WISH19612_vana_bank1_SXProcessed.nxs")
        # Load raw data (bank 1)
        ws = load_data_and_normalise("WISH00038237.raw")  # default so doesn't get overwrite van
        # normalise to vanadium
        RebinToWorkspace(WorkspaceToRebin=van, WorkspaceToMatch=ws, OutputWorkspace=van)
        Divide(LHSWorkspace=ws, RHSWorkspace=van, OutputWorkspace=ws)
        ReplaceSpecialValues(
            InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, InfinityValue=0, BigNumberThreshold=1e15, SmallNumberThreshold=-1e15
        )
        # Convert to Diffraction MD and Lorentz Correction
        wsMD = ConvertToDiffractionMDWorkspace(InputWorkspace=ws, LorentzCorrection=True, OneEventPerBin=False)
        # BinMD to 2D object and convert to histo so can compare saved workspace
        wsMD_2Dcut = BinMD(
            InputWorkspace=wsMD,
            AxisAligned=False,
            BasisVector0="Q_lab_x,Angstrom^-1,1.0,0.0,0.0",
            BasisVector1="Q_lab_y,Angstrom^-1,0.0,1.0,0.0",
            BasisVector2="Q_lab_z,Angstrom^-1,0.0,0.0,1.0",
            OutputExtents="0.2,0.8,-0.4,0.4,0.05,0.1",
            OutputBins="50,50,1",
        )
        ConvertMDHistoToMatrixWorkspace(InputWorkspace=wsMD_2Dcut, outputWorkspace="wsHisto_2Dcut")

    def validate(self):
        return "wsHisto_2Dcut", "WISH38237_MD_2Dcut.nxs"


class WISHIntegrateSatellitePeaksTest(MantidSystemTest):
    """
    This tests the integration of predicted satellite peaks. One could predict two modulations at once but here we
    capture the use of CombinePeaksWorkspace which is used in the WISH workflow in other instances (e.g. for combining
    peaks from different runs)
    """

    def cleanup(self):
        ADS.clear()
        try:
            os.remove(self._filepath)
        except:
            pass

    def runTest(self):
        # Load raw data (bank 1)
        wsMD = LoadMD("WISH38237_MD.nxs")  # default so doesn't get overwrite van
        # For each mod vec, predict and integrate peaks and combine
        qs = [(0.15, 0, 0.3), (-0.15, 0, 0.3)]
        all_pks = CreatePeaksWorkspace(InstrumentWorkspace=wsMD, NumberOfPeaks=0, OutputWorkspace="all_pks")
        LoadIsawUB(InputWorkspace=all_pks, Filename="Wish_Diffuse_Scattering_ISAW_UB.mat")
        # PredictPeaks
        parent = PredictPeaks(
            InputWorkspace=all_pks, WavelengthMin=0.8, WavelengthMax=9.3, MinDSpacing=0.5, ReflectionCondition="Primitive"
        )
        self._pfps = []
        self._saved_files = []
        for iq, q in enumerate(qs):
            wsname = f"pfp_{iq}"
            PredictFractionalPeaks(
                Peaks=parent,
                IncludeAllPeaksInRange=True,
                Hmin=0,
                Hmax=0,
                Kmin=1,
                Kmax=1,
                Lmin=0,
                Lmax=1,
                ReflectionCondition="Primitive",
                MaxOrder=1,
                ModVector1=",".join([str(qi) for qi in q]),
                FracPeaks=wsname,
            )
            FilterPeaks(
                InputWorkspace=wsname, OutputWorkspace=wsname, FilterVariable="Wavelength", FilterValue=9.3, Operator="<"
            )  # should get rid of one peak in q1 table
            FilterPeaks(InputWorkspace=wsname, OutputWorkspace=wsname, FilterVariable="Wavelength", FilterValue=0.8, Operator=">")
            IntegratePeaksMD(
                InputWorkspace=wsMD,
                PeakRadius="0.1",
                BackgroundInnerRadius="0.1",
                BackgroundOuterRadius="0.15",
                PeaksWorkspace=wsname,
                OutputWorkspace=wsname,
                IntegrateIfOnEdge=False,
                UseOnePercentBackgroundCorrection=False,
            )
            all_pks = CombinePeaksWorkspaces(LHSWorkspace=all_pks, RHSWorkspace=wsname)
            self._pfps.append(ADS.retrieve(wsname))
        self._filepath = os.path.join(config["defaultsave.directory"], "WISH_IntegratedSatellite.int")
        SaveReflections(InputWorkspace=all_pks, Filename=self._filepath, Format="Jana")
        self._all_pks = all_pks

    def validate(self):
        # check number of peaks and modulation vectors is as expected
        for ip, pfp in enumerate(self._pfps):
            self.assertEqual(1, pfp.getNumberPeaks())
            self.assertEqual(1, num_modulation_vectors(pfp))
        self.assertEqual(2, self._all_pks.getNumberPeaks())
        self.assertEqual(2, num_modulation_vectors(self._all_pks))
        # check files produced (don't need to check files as that should be covered by SaveReflections unit tests)
        self.assertTrue(os.path.isfile(self._filepath))


def load_data_and_normalise(filename, spectrumMin=1, spectrumMax=19461, outputWorkspace="sample"):
    """
    Function to load in raw data, crop and normalise
    :param filename: file path to .raw
    :param spectrumMin: min spec to load (default incl. all monitors)
    :param spectrumMax: max spec to load (default includes only bank 1)
    :param outputWorkspace: name of output workspace (can be specified to stop workspaces being overwritten)
    :return: normalised and cropped data with xunit wavelength (excl. monitors)
    """
    sample, mon = LoadRaw(
        Filename=filename, SpectrumMin=spectrumMin, SpectrumMax=spectrumMax, LoadMonitors="Separate", OutputWorkspace=outputWorkspace
    )
    for ws in [sample, mon]:
        CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=6000, XMax=99000)
        NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace=ws)
        ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target="Wavelength")
    NormaliseToMonitor(InputWorkspace=sample, OutputWorkspace=sample, MonitorWorkspaceIndex=3, MonitorWorkspace=mon)
    ReplaceSpecialValues(InputWorkspace=sample, OutputWorkspace=sample, NaNValue=0, InfinityValue=0)
    CropWorkspace(InputWorkspace=sample, OutputWorkspace=sample, XMin=0.8, XMax=9.3)
    return sample

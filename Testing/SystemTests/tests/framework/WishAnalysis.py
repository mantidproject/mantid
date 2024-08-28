# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import (
    AlignDetectors,
    ConvertFromDistribution,
    ConvertToDistribution,
    ConvertUnits,
    CropWorkspace,
    DeleteWorkspace,
    DiffractionFocussing,
    Divide,
    LoadNexusProcessed,
    LoadRaw,
    MaskBins,
    Minus,
    NormaliseToMonitor,
    SmoothData,
    SplineBackground,
    RebinToWorkspace,
    ReplaceSpecialValues,
)


class WishAnalysis(systemtesting.MantidSystemTest):
    """
    Runs the WISH analysis chain on one bank of data
    """

    def runTest(self):
        # MG: 5/5/2010: The test machine only has 1 Gb of RAM and can't handle a whole bank of WISH
        # load Data
        LoadRaw(Filename="WISH00016748.raw", OutputWorkspace="w16748-1", LoadLogFiles="0", SpectrumMin="6", SpectrumMax="5000")
        ConvertUnits(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", Target="Wavelength")
        # load monitors
        LoadRaw(Filename="WISH00016748.raw", OutputWorkspace="monitor16748", LoadLogFiles="0", SpectrumMin="4", SpectrumMax="4")
        ConvertUnits(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", Target="Wavelength")
        # etract integral section of monitor
        CropWorkspace(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", XMin="0.6", XMax="9.8")
        ConvertToDistribution(Workspace="monitor16748")
        # mask out vanadium peaks
        MaskBins(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", XMin="4.57", XMax="4.76")
        MaskBins(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", XMin="3.87", XMax="4.12")
        MaskBins(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", XMin="2.75", XMax="2.91")
        MaskBins(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", XMin="2.24", XMax="2.5")
        # generate sspline and smooth
        SplineBackground(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", NCoeff="30")
        SmoothData(InputWorkspace="monitor16748", OutputWorkspace="monitor16748", NPoints="50")
        ConvertFromDistribution(Workspace="monitor16748")
        # normalise data to the monitor in wavelength
        NormaliseToMonitor(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", MonitorWorkspace="monitor16748")
        NormaliseToMonitor(
            InputWorkspace="w16748-1",
            OutputWorkspace="w16748-1",
            MonitorWorkspace="monitor16748",
            IntegrationRangeMin="0.6",
            IntegrationRangeMax="9.8",
        )
        # align detectors
        ConvertUnits(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", Target="TOF")
        ReplaceSpecialValues(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", NaNValue="0", InfinityValue="0")
        ApplyDiffCal(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", CalibrationFile="wish_grouping_noends2_no_offsets_nov2009.cal")
        AConvertUnits(InputWorkspace="w16748-1", OutputWorkspace="w16748-1", Target="d-spacing")
        # focus data
        DiffractionFocussing(
            InputWorkspace="w16748-1", OutputWorkspace="w16748-1foc", GroupingFileName="wish_grouping_noends2_no_offsets_nov2009.cal"
        )
        DeleteWorkspace(Workspace="w16748-1")
        CropWorkspace(InputWorkspace="w16748-1foc", OutputWorkspace="w16748-1foc", XMin="0.83", XMax="45")
        # load pre-processed empty and subtract
        LoadNexusProcessed(Filename="emptycryo3307-1foc.nx5", OutputWorkspace="empty")
        RebinToWorkspace(WorkspaceToRebin="empty", WorkspaceToMatch="w16748-1foc", OutputWorkspace="empty")
        Minus(LHSWorkspace="w16748-1foc", RHSWorkspace="empty", OutputWorkspace="w16748-1foc")
        DeleteWorkspace(Workspace="empty")
        # Load preprocessed Vanadium and divide
        LoadNexusProcessed(Filename="vana3123-1foc-SS.nx5", OutputWorkspace="vana")
        RebinToWorkspace(WorkspaceToRebin="vana", WorkspaceToMatch="w16748-1foc", OutputWorkspace="vana")
        Divide(LHSWorkspace="w16748-1foc", RHSWorkspace="vana", OutputWorkspace="w16748-1foc")
        DeleteWorkspace(Workspace="vana")
        # convert back to TOF for output to GSAS/Fullprof
        ConvertUnits(InputWorkspace="w16748-1foc", OutputWorkspace="w16748-1foc", Target="TOF")

    def validate(self):
        return "w16748-1foc", "WishAnalysis.nxs"

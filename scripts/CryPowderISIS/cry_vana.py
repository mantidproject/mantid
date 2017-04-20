#pylint: disable=unused-variable

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
import cry_utils
import cry_sample
import cry_load


def create_vana(EXPR_FILE, NoAbs=False, write_existingv=True):
    # ==== Vana loading
    (dum, uampstotal) = cry_sample.get_data_sum(EXPR_FILE.VanFile, "Vanadium", EXPR_FILE)
    # Subtract the empty instrument ===
    (dum, uampstotal) = cry_sample.get_data_sum(EXPR_FILE.VEmptyFile, "Empty", EXPR_FILE)
    if uampstotal > 1e-6:
        print(" => Substract the Empty to the Vana")
        Minus(LHSWorkspace="Vanadium", RHSWorkspace="Empty", OutputWorkspace="Vanadium_align")
        mtd.remove("Empty")
    cry_load.align_fnc("Vanadium_align", EXPR_FILE)
    Divide(LHSWorkspace="Vanadium_align", RHSWorkspace="Corr", OutputWorkspace="Vanadium_corr")
    if not NoAbs:
        print(" => Van Absortption correction")
        cry_utils.correct_abs(InputWkspc="Vanadium_corr", outputWkspc="Transmission",
                              TheCylinderSampleHeight=EXPR_FILE.VHeight,
                              TheCylinderSampleRadius=EXPR_FILE.VRadius,
                              TheAttenuationXSection=EXPR_FILE.VAttenuationXSection,
                              TheScatteringXSection=EXPR_FILE.VScatteringXSection,
                              TheSampleNumberDensity=EXPR_FILE.VanaNumberDensity,
                              TheNumberOfSlices=EXPR_FILE.VNumberOfSlices,
                              TheNumberOfAnnuli=EXPR_FILE.VNumberOfAnnuli,
                              TheNumberOfWavelengthPoints=EXPR_FILE.VNumberOfWavelengthPoints,
                              TheExpMethod=EXPR_FILE.VExpMethod)
    # --- Alternative way.
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="Wavelength")
    # CylinderAbsorption(InputWorkspace="Vanadium", OutputWorkspace="Vanadium",
    #	CylinderSampleHeight=      EXPR_FILE.VHeight,
    #	CylinderSampleRadius=      EXPR_FILE.VRadius,
    #	AttenuationXSection=       EXPR_FILE.VAttenuationXSection,
    #	ScatteringXSection=        EXPR_FILE.VScatteringXSection,
    #	SampleNumberDensity=       EXPR_FILE.VanaNumberDensity,
    #	NumberOfSlices =           EXPR_FILE.VNumberOfSlices,
    #	NumberOfAnnuli=            EXPR_FILE.VNumberOfAnnuli,
    #	NumberOfWavelengthPoints = EXPR_FILE.VNumberOfWavelengthPoints,
    #	ExpMethod=                 EXPR_FILE.VExpMethod               )
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="dSpacing")
    # (dum,uampstotal)=CRY_sample.get_data_sum(EXPR_FILE.VanFile,"Vanadium2",EXPR_FILE)
    # Divide("Vanadium2", "Vanadium", "Vanadium")
    ##ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="dSpacing")
    # mtd.remove("Vanadium2")
    ##
    print(" => Focus type : " + EXPR_FILE.VGrpfocus)
    if EXPR_FILE.VGrpfocus == "sam":
        GrpFile = EXPR_FILE.Path2DatGrpFile
    else:
        GrpFile = EXPR_FILE.Path2VanGrpFile
    print(" => Van Focused with the Cal file :" + GrpFile)
    DiffractionFocussing(InputWorkspace="Vanadium_corr", OutputWorkspace="Vanadium_foc", GroupingFileName=GrpFile,
                         PreserveEvents=False)
    print(" => VANADIUM FOCUSED")
    ReplaceSpecialValues(InputWorkspace="Vanadium_foc", OutputWorkspace="Vanadium", NaNValue="0", InfinityValue="0",
                         BigNumberThreshold="99999999.99999999")
    SaveNexusProcessed(Filename=EXPR_FILE.CorrVanFile + "_unstripped.nxs", InputWorkspace="Vanadium")
    SaveFocusedXYE(Filename=EXPR_FILE.CorrVanFile + "_unstripped.dat", InputWorkspace="Vanadium", SplitFiles=True)
    strip_the_vana(EXPR_FILE)
    if EXPR_FILE.ExistV == 'no' and EXPR_FILE.VGrpfocus == 'van' and write_existingv:
        EXPR_FILE.write_prefline("ExistingV", "yes")
        EXPR_FILE.ExistV = "yes"
    if not EXPR_FILE.debugMode:
        mtd.remove("Vanadium_foc")
        mtd.remove("Transmission")
        mtd.remove("Vanadium_corr")
        mtd.remove("Vanadium_align")
    return False


def strip_the_vana(EXPR_FILE, LoadUnstrip=""):
    if LoadUnstrip:
        LoadNexusProcessed(Filename=LoadUnstrip, OutputWorkspace="Vanadium", EntryNumber=1)
    print(EXPR_FILE.bankList)
    cry_load.split_bank("Vanadium", bankList=EXPR_FILE.bankList, Del=True)
    if EXPR_FILE.VanPeakRemove == "interpol":
        print(" => Van Bragg-peak stripping")
        print("Smmoth Vana data with " + EXPR_FILE.VanSmooth + " points")
        remove_bins(EXPR_FILE)
    elif EXPR_FILE.VanPeakRemove == "strip":
        print(" => Van Bragg-peak stripping")
        van_strip(EXPR_FILE)
    elif EXPR_FILE.VanPeakRemove == "spline":
        van_spline(EXPR_FILE)
    elif EXPR_FILE.VanPeakRemove == "splineonly":
        van_spline_only(EXPR_FILE)
    else:
        return
    save_vana(EXPR_FILE)


def save_vana(EXPR_FILE):
    for i in EXPR_FILE.bankList:
        spec = i - 1
        vanfil = EXPR_FILE.CorrVanFile + "-" + str(spec) + ".nxs"
        SaveNexusProcessed(Filename=vanfil, InputWorkspace="Vanadium-" + str(i))


def remove_bins(EXPR_FILE):
    for i in EXPR_FILE.bankList:
        spec = i - 1
        SmoothData(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                   NPoints=int(EXPR_FILE.VanSmooth))
        print("Strip Vanapeak in bank=" + str(i))
        for peak in EXPR_FILE.VanPeakList[spec]:
            RemoveBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                       XMax=peak[1], RangeUnit="AsInput", Interpolation="Linear", WorkspaceIndex=0)
        SaveFocusedXYE(Filename=EXPR_FILE.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_strip(EXPR_FILE):
    for i in EXPR_FILE.bankList:
        spec = i - 1
        if EXPR_FILE.VanPeakWdt[spec] != 0:
            print("Strip Vanapeaks with params : bank=" + str(i) + " FWHM=" + str(
                EXPR_FILE.VanPeakWdt[spec]) + " Tol=" + str(EXPR_FILE.VanPeakTol[spec]))
            StripPeaks(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                       FWHM=EXPR_FILE.VanPeakWdt[spec], Tolerance=EXPR_FILE.VanPeakTol[spec], WorkspaceIndex=0)
            SaveFocusedXYE(Filename=EXPR_FILE.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                           SplitFiles=False)


def van_spline(EXPR_FILE):
    for i in EXPR_FILE.bankList:
        spec = i - 1
        print("Strip Vanapeak in bank=" + str(i))
        for peak in EXPR_FILE.VanPeakList[spec]:
            MaskBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                     XMax=peak[1])
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(EXPR_FILE.VanSplineCoef))
        SaveFocusedXYE(Filename=EXPR_FILE.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_spline_only(EXPR_FILE):
    for i in EXPR_FILE.bankList:
        spec = i - 1
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(EXPR_FILE.VanSplineCoef))
        SaveFocusedXYE(Filename=EXPR_FILE.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)

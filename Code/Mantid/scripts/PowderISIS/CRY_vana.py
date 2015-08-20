from mantid.simpleapi import *
import CRY_utils
import CRY_sample
import CRY_load


def create_vana(expt_files, NoAbs=False):
    # ==== Vana loading
    (dum, uampstotal) = CRY_sample.get_data_sum(expt_files.VanFile, "Vanadium", expt_files)
    # Subtract the empty instrument ===
    (dum, uampstotal) = CRY_sample.get_data_sum(expt_files.VEmptyFile, "Empty", expt_files)
    if uampstotal > 1e-6:
        print " => Substract the Empty to the Vana"
        Minus(LHSWorkspace="Vanadium", RHSWorkspace="Empty", OutputWorkspace="Vanadium_align")
        mtd.remove("Empty")
    CRY_load.align_fnc("Vanadium_align", expt_files)
    Divide(LHSWorkspace="Vanadium_align", RHSWorkspace="Corr", OutputWorkspace="Vanadium_corr")
    if not NoAbs:
        print " => Van Absortption correction"
        CRY_utils.correct_abs(InputWkspc="Vanadium_corr", outputWkspc="Transmission", \
                             TheCylinderSampleHeight=expt_files.VHeight, \
                             TheCylinderSampleRadius=expt_files.VRadius, \
                             TheAttenuationXSection=expt_files.VAttenuationXSection, \
                             TheScatteringXSection=expt_files.VScatteringXSection, \
                             TheSampleNumberDensity=expt_files.VanaNumberDensity, \
                             TheNumberOfSlices=expt_files.VNumberOfSlices, \
                             TheNumberOfAnnuli=expt_files.VNumberOfAnnuli, \
                             TheNumberOfWavelengthPoints=expt_files.VNumberOfWavelengthPoints, \
                             TheExpMethod=expt_files.VExpMethod)
    # --- Alternative way
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="Wavelength")
    # CylinderAbsorption(InputWorkspace="Vanadium", OutputWorkspace="Vanadium",
    #	CylinderSampleHeight=      expt_files.VHeight,
    #	CylinderSampleRadius=      expt_files.VRadius,
    #	AttenuationXSection=       expt_files.VAttenuationXSection,
    #	ScatteringXSection=        expt_files.VScatteringXSection,
    #	SampleNumberDensity=       expt_files.VanaNumberDensity,
    #	NumberOfSlices =           expt_files.VNumberOfSlices,
    #	NumberOfAnnuli=            expt_files.VNumberOfAnnuli,
    #	NumberOfWavelengthPoints = expt_files.VNumberOfWavelengthPoints,
    #	ExpMethod=                 expt_files.VExpMethod               )
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="dSpacing")
    # (dum,uampstotal)=CRY_sample.get_data_sum(expt_files.VanFile,"Vanadium2",expt_files)
    # Divide("Vanadium2", "Vanadium", "Vanadium")
    ##ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="dSpacing")
    # mtd.remove("Vanadium2")
    ##
    print " => Focus type : " + expt_files.VGrpfocus
    if expt_files.VGrpfocus == "sam":
        GrpFile = expt_files.Path2DatGrpFile
    else:
        GrpFile = expt_files.Path2VanGrpFile
    print " => Van Focused with the Cal file :" + GrpFile
    DiffractionFocussing(InputWorkspace="Vanadium_corr", OutputWorkspace="Vanadium_foc", GroupingFileName=GrpFile,
                         PreserveEvents=False)
    print " => VANADIUM FOCUSED"
    ReplaceSpecialValues(InputWorkspace="Vanadium_foc", OutputWorkspace="Vanadium", NaNValue="0", InfinityValue="0",
                         BigNumberThreshold="99999999.99999999")
    SaveNexusProcessed(Filename=expt_files.CorrVanFile + "_unstripped.nxs", InputWorkspace="Vanadium")
    SaveFocusedXYE(Filename=expt_files.CorrVanFile + "_unstripped.dat", InputWorkspace="Vanadium", SplitFiles=True)
    strip_the_vana(expt_files)
    if expt_files.ExistV == 'no' and expt_files.VGrpfocus == 'van':
        expt_files.write_prefline("ExistingV", "yes")
        expt_files.ExistV = "yes"
    if not expt_files.debugMode:
        mtd.remove("Vanadium_foc")
        mtd.remove("Transmission")
        mtd.remove("Vanadium_corr")
        mtd.remove("Vanadium_align")
    return False


def strip_the_vana(expt_files, LoadUnstrip=""):
    if not LoadUnstrip:
        LoadNexusProcessed(Filename=LoadUnstrip, OutputWorkspace="Vanadium", EntryNumber=1)
    print expt_files.bankList
    CRY_load.split_bank("Vanadium", bankList=expt_files.bankList, Del=True)
    if expt_files.VanPeakRemove == "interpol":
        print " => Van Bragg-peak stripping"
        print "Smmoth Vana data with " + expt_files.VanSmooth + " points"
        remove_bins(expt_files)
    elif expt_files.VanPeakRemove == "strip":
        print " => Van Bragg-peak stripping"
        van_strip(expt_files)
    elif expt_files.VanPeakRemove == "spline":
        van_spline(expt_files)
    elif expt_files.VanPeakRemove == "splineonly":
        van_spline_only(expt_files)
    else:
        return
    save_vana(expt_files)


def save_vana(expt_files):
    for i in expt_files.bankList:
        spec = i - 1
        vanfil = expt_files.CorrVanFile + "-" + str(spec) + ".nxs"
        SaveNexusProcessed(Filename=vanfil, InputWorkspace="Vanadium-" + str(i))


def remove_bins(expt_files):
    for i in expt_files.bankList:
        spec = i - 1
        SmoothData(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                   NPoints=int(expt_files.VanSmooth))
        print "Strip Vanapeak in bank=" + str(i)
        for peak in expt_files.VanPeakList[spec]:
            RemoveBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                       XMax=peak[1], RangeUnit="AsInput", Interpolation="Linear", WorkspaceIndex=0)
        SaveFocusedXYE(Filename=expt_files.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_strip(expt_files):
    for i in expt_files.bankList:
        spec = i - 1
        if expt_files.VanPeakWdt[spec] != 0:
            print "Strip Vanapeaks with params : bank=" + str(i) + " FWHM=" + str(
                expt_files.VanPeakWdt[spec]) + " Tol=" + str(expt_files.VanPeakTol[spec])
            StripPeaks(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                       FWHM=expt_files.VanPeakWdt[spec], Tolerance=expt_files.VanPeakTol[spec], WorkspaceIndex=0)
            SaveFocusedXYE(Filename=expt_files.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                           SplitFiles=False)


def van_spline(expt_files):
    for i in expt_files.bankList:
        spec = i - 1
        print "Strip Vanapeak in bank=" + str(i)
        for peak in expt_files.VanPeakList[spec]:
            MaskBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                     XMax=peak[1])
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(expt_files.VanSplineCoef))
        SaveFocusedXYE(Filename=expt_files.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_spline_only(expt_files):
    for i in expt_files.bankList:
        spec = i - 1
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(expt_files.VanSplineCoef))
        SaveFocusedXYE(Filename=expt_files.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


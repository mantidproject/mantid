from mantid.simpleapi import *
import CRY_utils
import CRY_sample
import CRY_load


def create_vana(experimentf, NoAbs=False):
    # ==== Vana loading
    (dum, uampstotal) = CRY_sample.get_data_sum(experimentf.VanFile, "Vanadium", experimentf)
    # Subtract the empty instrument ===
    (dum, uampstotal) = CRY_sample.get_data_sum(experimentf.VEmptyFile, "Empty", experimentf)
    if uampstotal > 1e-6:
        print " => Substract the Empty to the Vana"
        Minus(LHSWorkspace="Vanadium", RHSWorkspace="Empty", OutputWorkspace="Vanadium_align")
        mtd.remove("Empty")
    CRY_load.align_fnc("Vanadium_align", experimentf)
    Divide(LHSWorkspace="Vanadium_align", RHSWorkspace="Corr", OutputWorkspace="Vanadium_corr")
    if not NoAbs:
        print " => Van Absortption correction"
        CRY_utils.correct_abs(InputWkspc="Vanadium_corr", outputWkspc="Transmission", \
                             TheCylinderSampleHeight=experimentf.VHeight, \
                             TheCylinderSampleRadius=experimentf.VRadius, \
                             TheAttenuationXSection=experimentf.VAttenuationXSection, \
                             TheScatteringXSection=experimentf.VScatteringXSection, \
                             TheSampleNumberDensity=experimentf.VanaNumberDensity, \
                             TheNumberOfSlices=experimentf.VNumberOfSlices, \
                             TheNumberOfAnnuli=experimentf.VNumberOfAnnuli, \
                             TheNumberOfWavelengthPoints=experimentf.VNumberOfWavelengthPoints, \
                             TheExpMethod=experimentf.VExpMethod)
    # --- Alternative way
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="Wavelength")
    # CylinderAbsorption(InputWorkspace="Vanadium", OutputWorkspace="Vanadium",
    #	CylinderSampleHeight=      experimentf.VHeight,
    #	CylinderSampleRadius=      experimentf.VRadius,
    #	AttenuationXSection=       experimentf.VAttenuationXSection,
    #	ScatteringXSection=        experimentf.VScatteringXSection,
    #	SampleNumberDensity=       experimentf.VanaNumberDensity,
    #	NumberOfSlices =           experimentf.VNumberOfSlices,
    #	NumberOfAnnuli=            experimentf.VNumberOfAnnuli,
    #	NumberOfWavelengthPoints = experimentf.VNumberOfWavelengthPoints,
    #	ExpMethod=                 experimentf.VExpMethod               )
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="dSpacing")
    # (dum,uampstotal)=CRY_sample.get_data_sum(experimentf.VanFile,"Vanadium2",experimentf)
    # Divide("Vanadium2", "Vanadium", "Vanadium")
    ##ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="dSpacing")
    # mtd.remove("Vanadium2")
    ##
    print " => Focus type : " + experimentf.VGrpfocus
    if experimentf.VGrpfocus == "sam":
        GrpFile = experimentf.Path2DatGrpFile
    else:
        GrpFile = experimentf.Path2VanGrpFile
    print " => Van Focused with the Cal file :" + GrpFile
    DiffractionFocussing(InputWorkspace="Vanadium_corr", OutputWorkspace="Vanadium_foc", GroupingFileName=GrpFile,
                         PreserveEvents=False)
    print " => VANADIUM FOCUSED"
    ReplaceSpecialValues(InputWorkspace="Vanadium_foc", OutputWorkspace="Vanadium", NaNValue="0", InfinityValue="0",
                         BigNumberThreshold="99999999.99999999")
    SaveNexusProcessed(Filename=experimentf.CorrVanFile + "_unstripped.nxs", InputWorkspace="Vanadium")
    SaveFocusedXYE(Filename=experimentf.CorrVanFile + "_unstripped.dat", InputWorkspace="Vanadium", SplitFiles=True)
    strip_the_vana(experimentf)
    if experimentf.ExistV == 'no' and experimentf.VGrpfocus == 'van':
        experimentf.write_prefline("ExistingV", "yes")
        experimentf.ExistV = "yes"
    if not experimentf.debugMode:
        mtd.remove("Vanadium_foc")
        mtd.remove("Transmission")
        mtd.remove("Vanadium_corr")
        mtd.remove("Vanadium_align")
    return False


def strip_the_vana(experimentf, LoadUnstrip=""):
    if not LoadUnstrip:
        LoadNexusProcessed(Filename=LoadUnstrip, OutputWorkspace="Vanadium", EntryNumber=1)
    print experimentf.bankList
    CRY_load.split_bank("Vanadium", bankList=experimentf.bankList, Del=True)
    if experimentf.VanPeakRemove == "interpol":
        print " => Van Bragg-peak stripping"
        print "Smmoth Vana data with " + experimentf.VanSmooth + " points"
        remove_bins(experimentf)
    elif experimentf.VanPeakRemove == "strip":
        print " => Van Bragg-peak stripping"
        van_strip(experimentf)
    elif experimentf.VanPeakRemove == "spline":
        van_spline(experimentf)
    elif experimentf.VanPeakRemove == "splineonly":
        van_spline_only(experimentf)
    else:
        return
    save_vana(experimentf)


def save_vana(experimentf):
    for i in experimentf.bankList:
        spec = i - 1
        vanfil = experimentf.CorrVanFile + "-" + str(spec) + ".nxs"
        SaveNexusProcessed(Filename=vanfil, InputWorkspace="Vanadium-" + str(i))


def remove_bins(experimentf):
    for i in experimentf.bankList:
        spec = i - 1
        SmoothData(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                   NPoints=int(experimentf.VanSmooth))
        print "Strip Vanapeak in bank=" + str(i)
        for peak in experimentf.VanPeakList[spec]:
            RemoveBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                       XMax=peak[1], RangeUnit="AsInput", Interpolation="Linear", WorkspaceIndex=0)
        SaveFocusedXYE(Filename=experimentf.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_strip(experimentf):
    for i in experimentf.bankList:
        spec = i - 1
        if experimentf.VanPeakWdt[spec] != 0:
            print "Strip Vanapeaks with params : bank=" + str(i) + " FWHM=" + str(
                experimentf.VanPeakWdt[spec]) + " Tol=" + str(experimentf.VanPeakTol[spec])
            StripPeaks(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                       FWHM=experimentf.VanPeakWdt[spec], Tolerance=experimentf.VanPeakTol[spec], WorkspaceIndex=0)
            SaveFocusedXYE(Filename=experimentf.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                           SplitFiles=False)


def van_spline(experimentf):
    for i in experimentf.bankList:
        spec = i - 1
        print "Strip Vanapeak in bank=" + str(i)
        for peak in experimentf.VanPeakList[spec]:
            MaskBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                     XMax=peak[1])
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(experimentf.VanSplineCoef))
        SaveFocusedXYE(Filename=experimentf.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def van_spline_only(experimentf):
    for i in experimentf.bankList:
        spec = i - 1
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(experimentf.VanSplineCoef))
        SaveFocusedXYE(Filename=experimentf.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


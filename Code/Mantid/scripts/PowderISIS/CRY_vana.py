from mantid.simpleapi import *
import CRY_utils
import CRY_sample
import CRY_load


def CreateVana(expt, NoAbs=False):
    # ==== Vana loading
    (dum, uampstotal) = CRY_sample.getDataSum(expt.VanFile, "Vanadium", expt)
    # Subtract the empty instrument ===
    (dum, uampstotal) = CRY_sample.getDataSum(expt.VEmptyFile, "Empty", expt)
    if uampstotal > 1e-6:
        print " => Substract the Empty to the Vana"
        Minus(LHSWorkspace="Vanadium", RHSWorkspace="Empty", OutputWorkspace="Vanadium_align")
        mtd.remove("Empty")
    CRY_load.Align("Vanadium_align", expt)
    Divide(LHSWorkspace="Vanadium_align", RHSWorkspace="Corr", OutputWorkspace="Vanadium_corr")
    if not NoAbs:
        print " => Van Absortption correction"
        CRY_utils.CorrectAbs(InputWkspc="Vanadium_corr", outputWkspc="Transmission", \
                             TheCylinderSampleHeight=expt.VHeight, \
                             TheCylinderSampleRadius=expt.VRadius, \
                             TheAttenuationXSection=expt.VAttenuationXSection, \
                             TheScatteringXSection=expt.VScatteringXSection, \
                             TheSampleNumberDensity=expt.VanaNumberDensity, \
                             TheNumberOfSlices=expt.VNumberOfSlices, \
                             TheNumberOfAnnuli=expt.VNumberOfAnnuli, \
                             TheNumberOfWavelengthPoints=expt.VNumberOfWavelengthPoints, \
                             TheExpMethod=expt.VExpMethod)
    # --- Alternative way
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="Wavelength")
    # CylinderAbsorption(InputWorkspace="Vanadium", OutputWorkspace="Vanadium",
    #	CylinderSampleHeight=      expt.VHeight,
    #	CylinderSampleRadius=      expt.VRadius,
    #	AttenuationXSection=       expt.VAttenuationXSection,
    #	ScatteringXSection=        expt.VScatteringXSection,
    #	SampleNumberDensity=       expt.VanaNumberDensity,
    #	NumberOfSlices =           expt.VNumberOfSlices,
    #	NumberOfAnnuli=            expt.VNumberOfAnnuli,
    #	NumberOfWavelengthPoints = expt.VNumberOfWavelengthPoints,
    #	ExpMethod=                 expt.VExpMethod               )
    # ConvertUnits(InputWorkspace="Vanadium", OutputWorkspace="Vanadium", Target="dSpacing")
    # (dum,uampstotal)=CRY_sample.getDataSum(expt.VanFile,"Vanadium2",expt)
    # Divide("Vanadium2", "Vanadium", "Vanadium")
    ##ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="dSpacing")
    # mtd.remove("Vanadium2")
    ##
    print " => Focus type : " + expt.VGrpfocus
    if expt.VGrpfocus == "sam":
        GrpFile = expt.Path2DatGrpFile
    else:
        GrpFile = expt.Path2VanGrpFile
    print " => Van Focused with the Cal file :" + GrpFile
    DiffractionFocussing(InputWorkspace="Vanadium_corr", OutputWorkspace="Vanadium_foc", GroupingFileName=GrpFile,
                         PreserveEvents=False)
    print " => VANADIUM FOCUSED"
    ReplaceSpecialValues(InputWorkspace="Vanadium_foc", OutputWorkspace="Vanadium", NaNValue="0", InfinityValue="0",
                         BigNumberThreshold="99999999.99999999")
    SaveNexusProcessed(Filename=expt.CorrVanFile + "_unstripped.nxs", InputWorkspace="Vanadium")
    SaveFocusedXYE(Filename=expt.CorrVanFile + "_unstripped.dat", InputWorkspace="Vanadium", SplitFiles=True)
    stripTheVana(expt)
    if expt.ExistV == 'no' and expt.VGrpfocus == 'van':
        expt.write_prefline("ExistingV", "yes")
        expt.ExistV = "yes"
    if not expt.debugMode:
        mtd.remove("Vanadium_foc")
        mtd.remove("Transmission")
        mtd.remove("Vanadium_corr")
        mtd.remove("Vanadium_align")
    return False


def stripTheVana(expt, LoadUnstrip=""):
    if LoadUnstrip <> "":
        LoadNexusProcessed(Filename=LoadUnstrip, OutputWorkspace="Vanadium", EntryNumber=1)
    print expt.bankList
    CRY_load.SplitBank("Vanadium", bankList=expt.bankList, Del=True)
    if expt.VanPeakRemove == "interpol":
        print " => Van Bragg-peak stripping"
        print "Smmoth Vana data with " + expt.VanSmooth + " points"
        Removebins(expt)
    elif expt.VanPeakRemove == "strip":
        print " => Van Bragg-peak stripping"
        vanStrip(expt)
    elif expt.VanPeakRemove == "spline":
        vanSpline(expt)
    elif expt.VanPeakRemove == "splineonly":
        vanSplineOnly(expt)
    else:
        return
    SaveVana(expt)


def SaveVana(expt):
    for i in expt.bankList:
        spec = i - 1
        vanfil = expt.CorrVanFile + "-" + str(spec) + ".nxs"
        SaveNexusProcessed(Filename=vanfil, InputWorkspace="Vanadium-" + str(i))


def Removebins(expt):
    for i in expt.bankList:
        spec = i - 1
        SmoothData(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                   NPoints=int(expt.VanSmooth))
        print "Strip Vanapeak in bank=" + str(i)
        for peak in expt.VanPeakList[spec]:
            RemoveBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                       XMax=peak[1], RangeUnit="AsInput", Interpolation="Linear", WorkspaceIndex=0)
        SaveFocusedXYE(Filename=expt.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def vanStrip(expt):
    for i in expt.bankList:
        spec = i - 1
        if expt.VanPeakWdt[spec] <> 0:
            print "Strip Vanapeaks with params : bank=" + str(i) + " FWHM=" + str(
                expt.VanPeakWdt[spec]) + " Tol=" + str(expt.VanPeakTol[spec])
            StripPeaks(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i),
                       FWHM=expt.VanPeakWdt[spec], Tolerance=expt.VanPeakTol[spec], WorkspaceIndex=0)
            SaveFocusedXYE(Filename=expt.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                           SplitFiles=False)


def vanSpline(expt):
    for i in expt.bankList:
        spec = i - 1
        print "Strip Vanapeak in bank=" + str(i)
        for peak in expt.VanPeakList[spec]:
            MaskBins(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), XMin=peak[0],
                     XMax=peak[1])
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(expt.VanSplineCoef))
        SaveFocusedXYE(Filename=expt.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)


def vanSplineOnly(expt):
    for i in expt.bankList:
        spec = i - 1
        SplineBackground(InputWorkspace="Vanadium-" + str(i), OutputWorkspace="Vanadium-" + str(i), WorkspaceIndex=0,
                         NCoeff=int(expt.VanSplineCoef))
        SaveFocusedXYE(Filename=expt.CorrVanFile + "-" + str(spec) + "_.dat", InputWorkspace="Vanadium-" + str(i),
                       SplitFiles=False)

